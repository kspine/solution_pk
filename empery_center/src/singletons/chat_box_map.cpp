#include "../precompiled.hpp"
#include "chat_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../events/account.hpp"
#include "../chat_box.hpp"
#include "account_map.hpp"
#include "../horn_message.hpp"
#include "../mysql/horn_message.hpp"
#include "controller_client.hpp"
#include "../msg/st_chat.hpp"

namespace EmperyCenter {

namespace {
	struct ChatBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<ChatBox> chat_box;

		ChatBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(ChatBoxContainer, ChatBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<ChatBoxContainer> g_chat_box_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Chat box gc timer: now = ", now);

		const auto chat_box_map = g_chat_box_map.lock();
		if(!chat_box_map){
			return;
		}

		for(;;){
			const auto it = chat_box_map->begin<1>();
			if(it == chat_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			if(it->chat_box.use_count() != 1){
				chat_box_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming chat box: account_uuid = ", it->account_uuid);
				chat_box_map->erase<1>(it);
			}
		}
	}

	struct HornElement {
		boost::shared_ptr<HornMessage> horn_message;

		HornMessageUuid horn_message_uuid;
		std::uint64_t expiry_time;
		LanguageId language_id;

		explicit HornElement(boost::shared_ptr<HornMessage> horn_message_)
			: horn_message(std::move(horn_message_))
			, horn_message_uuid(horn_message->get_horn_message_uuid())
			, expiry_time(horn_message->get_expiry_time())
			, language_id(horn_message->get_language_id())
		{
		}
	};

	MULTI_INDEX_MAP(HornContainer, HornElement,
		UNIQUE_MEMBER_INDEX(horn_message_uuid)
		MULTI_MEMBER_INDEX(expiry_time)
		MULTI_MEMBER_INDEX(language_id)
	)

	boost::weak_ptr<HornContainer> g_horn_map;

	void horn_gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Horn message gc timer: now = ", now);

		const auto horn_map = g_horn_map.lock();
		if(!horn_map){
			return;
		}

		const auto utc_now = Poseidon::get_utc_time();
		const auto range = std::make_pair(horn_map->begin<1>(), horn_map->upper_bound<1>(utc_now));

		std::vector<boost::shared_ptr<HornMessage>> horn_messages_to_delete;
		horn_messages_to_delete.reserve(static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			horn_messages_to_delete.emplace_back(it->horn_message);
		}
		for(auto it = horn_messages_to_delete.begin(); it != horn_messages_to_delete.end(); ++it){
			const auto &horn_message = *it;
			LOG_EMPERY_CENTER_DEBUG("Reclaiming horn message: horn_message_uuid = ", horn_message->get_horn_message_uuid());
			horn_message->delete_from_game();
		}

		Poseidon::MySqlDaemon::enqueue_for_deleting("Center_HornMessage",
			"DELETE QUICK `m`.* "
			"  FROM `Center_HornMessage` AS `m` "
			"  WHERE `m`.`expiry_time` = '0000-00-00 00:00:00'");
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto chat_box_map = boost::make_shared<ChatBoxContainer>();
		g_chat_box_map = chat_box_map;
		handles.push(chat_box_map);

		const auto horn_map = boost::make_shared<HornContainer>();
		LOG_EMPERY_CENTER_INFO("Loading horn messages...");
		conn->execute_sql("SELECT * FROM `Center_HornMessage`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_HornMessage>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto horn = boost::make_shared<HornMessage>(std::move(obj));
			horn_map->insert(HornElement(std::move(horn)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", horn_map->size(), " horn message(s).");
		g_horn_map = horn_map;
		handles.push(horn_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);

		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);

		timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&horn_gc_timer_proc, std::placeholders::_2));
		handles.push(timer);

		auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountInvalidate>(
			[](const boost::shared_ptr<Events::AccountInvalidate> &event){ ChatBoxMap::unload(event->account_uuid); });
		handles.push(listener);
	}

	void synchronize_horn_message_with_cluster(const boost::shared_ptr<HornMessage> &horn_message) noexcept {
		PROFILE_ME;

		boost::shared_ptr<ControllerClient> controller;
		try {
			controller = ControllerClient::require();
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
		if(!controller){
			LOG_EMPERY_CENTER_FATAL("Could not connect to controller server!");
			return;
		}

		const auto horn_message_uuid = horn_message->get_horn_message_uuid();

		try {
			Poseidon::MySqlDaemon::enqueue_for_waiting_for_all_async_operations();

			Msg::ST_ChatBroadcastHornMessage msg;
			msg.horn_message_uuid = horn_message_uuid.str();
			controller->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}
}

boost::shared_ptr<ChatBox> ChatBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto chat_box_map = g_chat_box_map.lock();
	if(!chat_box_map){
		LOG_EMPERY_CENTER_WARNING("ChatBoxMap is not loaded.");
		return { };
	}

	auto it = chat_box_map->find<0>(account_uuid);
	if(it == chat_box_map->end<0>()){
		it = chat_box_map->insert<0>(it, ChatBoxElement(account_uuid, 0));
	}
	if(!it->chat_box){
		LOG_EMPERY_CENTER_DEBUG("Loading chat box: account_uuid = ", account_uuid);

		if(!AccountMap::is_holding_controller_token(account_uuid)){
			LOG_EMPERY_CENTER_DEBUG("Failed to acquire controller token: account_uuid = ", account_uuid);
			return { };
		}

		auto chat_box = boost::make_shared<ChatBox>(account_uuid);

		it->chat_box = std::move(chat_box);

		assert(it->chat_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	chat_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->chat_box;
}
boost::shared_ptr<ChatBox> ChatBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Chat box not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Chat box not found"));
	}
	return ret;
}
void ChatBoxMap::unload(AccountUuid account_uuid){
	PROFILE_ME;

	const auto chat_box_map = g_chat_box_map.lock();
	if(!chat_box_map){
		LOG_EMPERY_CENTER_WARNING("ChatBoxMap is not loaded.");
		return;
	}

	const auto it = chat_box_map->find<0>(account_uuid);
	if(it == chat_box_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Chat box not loaded: account_uuid = ", account_uuid);
		return;
	}

	chat_box_map->set_key<0, 1>(it, 0);
	const auto now = Poseidon::get_fast_mono_clock();
	gc_timer_proc(now);
}

boost::shared_ptr<HornMessage> ChatBoxMap::get_horn_message(HornMessageUuid horn_message_uuid){
	PROFILE_ME;

	const auto horn_map = g_horn_map.lock();
	if(!horn_map){
		LOG_EMPERY_CENTER_WARNING("Horn message map not loaded.");
		return { };
	}

	const auto it = horn_map->find<0>(horn_message_uuid);
	if(it == horn_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Horn message not found: horn_message_uuid = ", horn_message_uuid);
		return { };
	}
	return it->horn_message;
}
boost::shared_ptr<HornMessage> ChatBoxMap::require_horn_message(HornMessageUuid horn_message_uuid){
	PROFILE_ME;

	auto horn_message = get_horn_message(horn_message_uuid);
	if(!horn_message){
		LOG_EMPERY_CENTER_DEBUG("Horn message not found: horn_message_uuid = ", horn_message_uuid);
		DEBUG_THROW(Exception, sslit("Horn message not found"));
	}
	return horn_message;
}
boost::shared_ptr<HornMessage> ChatBoxMap::get_or_reload_horn_message(HornMessageUuid horn_message_uuid){
	PROFILE_ME;

	auto horn_message = get_horn_message(horn_message_uuid);
	if(!horn_message){
		horn_message = forced_reload_horn_message(horn_message_uuid);
	}
	return horn_message;
}
boost::shared_ptr<HornMessage> ChatBoxMap::forced_reload_horn_message(HornMessageUuid horn_message_uuid){
	PROFILE_ME;

	const auto horn_map = g_horn_map.lock();
	if(!horn_map){
		LOG_EMPERY_CENTER_WARNING("Horn message map not loaded.");
		return { };
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_HornMessage>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_HornMessage` "
		    <<"  WHERE `horn_message_uuid` = " <<Poseidon::MySql::UuidFormatter(horn_message_uuid.get())
		    <<"    AND `expiry_time` > 0";
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_HornMessage>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_HornMessage", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_DEBUG("Horn message not found: horn_message_uuid = ", horn_message_uuid);
		return { };
	}

	auto horn_message = boost::make_shared<HornMessage>(std::move(sink->front()));

	const auto elem = HornElement(horn_message);
	const auto result = horn_map->insert(elem);
	if(!result.second){
		horn_map->replace(result.first, elem);
	}

	LOG_EMPERY_CENTER_DEBUG("Successfully reloaded horn message not found: horn_message_uuid = ", horn_message_uuid);
	return std::move(horn_message);
}

void ChatBoxMap::get_horn_messages_all(std::vector<boost::shared_ptr<HornMessage>> &ret){
	PROFILE_ME;

	const auto horn_map = g_horn_map.lock();
	if(!horn_map){
		LOG_EMPERY_CENTER_WARNING("Horn message map not loaded.");
		return;
	}

	ret.reserve(ret.size() + horn_map->size());
	for(auto it = horn_map->begin<0>(); it != horn_map->end<0>(); ++it){
		ret.emplace_back(it->horn_message);
	}
}
void ChatBoxMap::get_horn_messages_by_language_id(std::vector<boost::shared_ptr<HornMessage>> &ret, LanguageId language_id){
	PROFILE_ME;

	const auto horn_map = g_horn_map.lock();
	if(!horn_map){
		LOG_EMPERY_CENTER_WARNING("Horn message map not loaded.");
		return;
	}

	const auto range = horn_map->equal_range<2>(language_id);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->horn_message);
	}
}

void ChatBoxMap::insert_horn_message(const boost::shared_ptr<HornMessage> &horn_message){
	PROFILE_ME;

	const auto horn_map = g_horn_map.lock();
	if(!horn_map){
		LOG_EMPERY_CENTER_WARNING("Horn message map not loaded.");
		DEBUG_THROW(Exception, sslit("Horn message map not loaded"));
	}

	const auto horn_message_uuid = horn_message->get_horn_message_uuid();

	if(horn_message->is_virtually_removed()){
		LOG_EMPERY_CENTER_WARNING("Horn message has been marked as deleted: horn_message_uuid = ", horn_message_uuid);
		DEBUG_THROW(Exception, sslit("Horn message has been marked as deleted"));
	}

	LOG_EMPERY_CENTER_DEBUG("Inserting horn message: horn_message_uuid = ", horn_message_uuid);
	const auto result = horn_map->insert(HornElement(horn_message));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Horn message already exists: horn_message_uuid = ", horn_message_uuid);
		DEBUG_THROW(Exception, sslit("Horn message already exists"));
	}

	synchronize_horn_message_with_cluster(horn_message);
}
void ChatBoxMap::update_horn_message(const boost::shared_ptr<HornMessage> &horn_message, bool throws_if_not_exists){
	PROFILE_ME;

	const auto horn_map = g_horn_map.lock();
	if(!horn_map){
		LOG_EMPERY_CENTER_WARNING("Horn message map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Horn message map not loaded"));
		}
		return;
	}

	const auto horn_message_uuid = horn_message->get_horn_message_uuid();

	const auto it = horn_map->find<0>(horn_message_uuid);
	if(it == horn_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("Horn message not found: horn_message_uuid = ", horn_message_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Horn message map not loaded"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating horn message: horn_message_uuid = ", horn_message_uuid);
	if(horn_message->is_virtually_removed()){
		horn_map->erase<0>(it);
	} else {
		horn_map->replace<0>(it, HornElement(horn_message));
	}

	synchronize_horn_message_with_cluster(horn_message);
}

}
