#include "../precompiled.hpp"
#include "legion_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <tuple>
#include "player_session_map.hpp"
#include "../mysql/legion.hpp"
#include "../msg/cs_legion.hpp"
#include "../player_session.hpp"
#include "../events/account.hpp"
#include "../account.hpp"
#include "../legion.hpp"
#include "../legion_attribute_ids.hpp"
#include "item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../string_utilities.hpp"
#include "controller_client.hpp"
#include "../msg/st_account.hpp"
#include "../msg/err_legion.hpp"

namespace EmperyCenter {

namespace {
	struct LegionElement {
		boost::shared_ptr<Legion> account;

		LegionUuid legion_uuid;
		std::size_t nick_hash;
		AccountUuid account_uuid;

		explicit LegionElement(boost::shared_ptr<Legion> account_)
			: account(std::move(account_))
			, legion_uuid(account->get_legion_uuid())
			, nick_hash(hash_string_nocase(account->get_nick())), account_uuid(account->get_account_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(LegionContainer, LegionElement,
		UNIQUE_MEMBER_INDEX(legion_uuid)
		MULTI_MEMBER_INDEX(nick_hash)
		MULTI_MEMBER_INDEX(account_uuid)
	)

	boost::shared_ptr<LegionContainer> g_legion_map;

	enum CacheType : std::uint64_t {
		CT_NONE       = 0x0000, // Silent the warning.
		CT_NICK       = 0x0001,
		CT_ATTRS      = 0x0002,
		CT_PRIV_ATTRS = 0x0004,
		CT_ITEMS      = 0x0008,
	};

	struct InfoCacheElement {
		std::tuple<LegionUuid, boost::weak_ptr<PlayerSession>, CacheType> key;
		std::uint64_t expiry_time;

		InfoCacheElement(LegionUuid account_uuid_, const boost::shared_ptr<PlayerSession> &session_, CacheType type_,
			std::uint64_t expiry_time_)
			: key(account_uuid_, session_, type_), expiry_time(expiry_time_)
		{
		}
	};

	MULTI_INDEX_MAP(InfoCacheContainer, InfoCacheElement,
		UNIQUE_MEMBER_INDEX(key)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<InfoCacheContainer> g_info_cache_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// Legion
		struct TempLegionElement {
			boost::shared_ptr<MySql::Center_Legion> obj;
			std::vector<boost::shared_ptr<MySql::Center_LegionAttribute>> attributes;
			std::vector<boost::shared_ptr<MySql::Center_Legion_Member>> members;
		};
		std::map<LegionUuid, TempLegionElement> temp_account_map;

		LOG_EMPERY_CENTER_INFO("Loading Legions...");
		conn->execute_sql("SELECT * FROM `Center_Legion`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Legion>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto legion_uuid = LegionUuid(obj->get_legion_uuid());
			temp_account_map[legion_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_account_map.size(), " Legion(s).");

		LOG_EMPERY_CENTER_INFO("Loading Legion attributes...");
		conn->execute_sql("SELECT * FROM `Center_LegionAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_LegionAttribute>();
			obj->fetch(conn);
			const auto legion_uuid = LegionUuid(obj->unlocked_get_legion_uuid());
			const auto it = temp_account_map.find(legion_uuid);
			if(it == temp_account_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.attributes.emplace_back(std::move(obj));
		}

		LOG_EMPERY_CENTER_INFO("Done loading Legion attributes.");

		/*
		LOG_EMPERY_CENTER_INFO("Loading Legion members...");
		conn->execute_sql("SELECT * FROM `Center_Legion_Member`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Legion_Member>();
			obj->fetch(conn);
			const auto legion_uuid = LegionUuid(obj->unlocked_get_legion_uuid());
			const auto it = temp_account_map.find(legion_uuid);
			if(it == temp_account_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.members.emplace_back(std::move(obj));
		}

		LOG_EMPERY_CENTER_INFO("Done loading account attributes.");
		*/
		const auto legion_map = boost::make_shared<LegionContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			auto legion = boost::make_shared<Legion>(std::move(it->second.obj), it->second.attributes);

		//	LOG_EMPERY_CENTER_INFO("legion attributes size==============================================",it->second.attributes.size());

		//	LOG_EMPERY_CENTER_INFO("legion members size==============================================",it->second.members.size());

			legion_map->insert(LegionElement(std::move(legion)));

		//	sleep(5000);
		}
		g_legion_map = legion_map;
		handles.push(legion_map);

		const auto info_cache_map = boost::make_shared<InfoCacheContainer>();
		g_info_cache_map = info_cache_map;
		handles.push(info_cache_map);
	}

	boost::shared_ptr<Legion> reload_account_aux(boost::shared_ptr<MySql::Center_Legion> obj){
		PROFILE_ME;

		std::deque<boost::shared_ptr<const Poseidon::JobPromise>> promises;

		const auto attributes = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_LegionAttribute>>>();

#define RELOAD_PART_(sink_, table_)	\
		{	\
			std::ostringstream oss;	\
			const auto account_uuid = obj->unlocked_get_account_uuid();	\
			oss <<"SELECT * FROM `" #table_ "` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid);	\
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(	\
				[sink_](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){	\
					auto obj = boost::make_shared<MySql:: table_ >();	\
					obj->fetch(conn);	\
					obj->enable_auto_saving();	\
					(sink_)->emplace_back(std::move(obj));	\
				}, #table_, oss.str());	\
			promises.emplace_back(std::move(promise));	\
		}
//=============================================================================
	//	RELOAD_PART_(attributes,         Center_LegionAttribute)
//=============================================================================
		for(const auto &promise : promises){
			Poseidon::JobDispatcher::yield(promise, true);
		}
#undef RELOAD_PART_

		return boost::make_shared<Legion>(std::move(obj), *attributes);
	}

	template<typename IteratorT>
	void really_append_account(std::vector<boost::shared_ptr<Legion>> &ret,
		const std::pair<IteratorT, IteratorT> &range, std::uint64_t begin, std::uint64_t count)
	{
		PROFILE_ME;

		auto lower = range.first;
		for(std::uint64_t i = 0; i < begin; ++i){
			if(lower == range.second){
				break;
			}
			++lower;
		}

		auto upper = lower;
		for(std::uint64_t i = 0; i < count; ++i){
			if(upper == range.second){
				break;
			}
			++upper;
		}

		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(lower, upper)));
		for(auto it = lower; it != upper; ++it){
			ret.emplace_back(it->account);
		}
	}

	void synchronize_account_and_update_cache(std::uint64_t now, std::uint64_t cache_timeout,
		const boost::shared_ptr<Legion> &account, const boost::shared_ptr<ItemBox> &item_box,
		const boost::shared_ptr<PlayerSession> &session, std::uint64_t flags)
	try {
		PROFILE_ME;

		if(flags == 0){
			return;
		}

/*
		const auto account_uuid = account->get_account_uuid();


		Msg::SC_LegionAttributes msg;
		msg.account_uuid = account_uuid.str();

		// nick
		if(flags & CT_NICK){
			msg.nick = account->get_nick();
		}

		// attributes
		boost::container::flat_map<LegionAttributeId, std::string> attributes;
		auto public_end = attributes.end();

		const auto fetch_attributes = [&]{
			if(attributes.empty()){
				account->get_attributes(attributes);
				msg.attributes.reserve(attributes.size());
				public_end = attributes.upper_bound(LegionAttributeIds::ID_PUBLIC_END);
			}
		};
		const auto copy_attributes  = [&](decltype(attributes.cbegin()) begin, decltype(attributes.cbegin()) end){
			for(auto it = begin; it != end; ++it){
				auto &attribute = *msg.attributes.emplace(msg.attributes.end());
				attribute.account_attribute_id = it->first.get();
				attribute.value                = std::move(it->second);
			}
		};

		if(flags & CT_ATTRS){
			fetch_attributes();
			copy_attributes(attributes.begin(), public_end);
		}
		if(flags & CT_PRIV_ATTRS){
			fetch_attributes();
			copy_attributes(public_end, attributes.upper_bound(LegionAttributeIds::ID_END));
		}

		// items
		if(flags & CT_ITEMS){
			std::vector<boost::shared_ptr<const Data::Item>> items_to_check;
			Data::Item::get_public(items_to_check);
			msg.public_items.reserve(items_to_check.size());
			for(auto it = items_to_check.begin(); it != items_to_check.end(); ++it){
				const auto &item_data = *it;
				auto info = item_box->get(item_data->item_id);

				auto &item = *msg.public_items.emplace(msg.public_items.end());
				item.item_id    = info.item_id.get();
				item.item_count = info.count;
			}
		}

		// 其他。
		msg.promotion_level = account->get_promotion_level();
		msg.activated       = account->has_been_activated();

		session->send(msg);

		const auto info_cache_map = g_info_cache_map.lock();
		if(info_cache_map){
			const auto update_cache = [&](CacheType type){
				if(cache_timeout != 0){
					auto elem = InfoCacheElement(account_uuid, session, type, saturated_add(now, cache_timeout));
					auto it = info_cache_map->find<0>(elem.key);
					if(it == info_cache_map->end<0>()){
						info_cache_map->insert<0>(std::move(elem));
					} else {
						info_cache_map->replace<0>(it, std::move(elem));
					}
				}
			};

			if(flags & CT_NICK){
				update_cache(CT_NICK);
			}
			if(flags & CT_ATTRS){
				update_cache(CT_ATTRS);
			}
			if(flags & CT_PRIV_ATTRS){
				update_cache(CT_PRIV_ATTRS);
			}
			if(flags & CT_ITEMS){
				update_cache(CT_ITEMS);
			}
		}

		*/
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		session->shutdown(e.what());
	}

}

bool LegionMap::is_holding_controller_token(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return false;
	}

	/*
	const auto controller = ControllerClient::require();

	Msg::ST_LegionQueryToken treq;
	treq.account_uuid = account_uuid.str();
	auto tresult = controller->send_and_wait(treq);
	LOG_EMPERY_CENTER_DEBUG("Controller response: code = ", tresult.first, ", msg = ", tresult.second);
	if(tresult.first != 0){
		return false;
	}
	*/
	return true;
}
void LegionMap::require_controller_token(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		DEBUG_THROW(Exception, sslit("legion map not loaded"));
	}

	const auto controller = ControllerClient::require();

	/*
	Msg::ST_LegionAcquireToken treq;
	treq.account_uuid = account_uuid.str();
	auto tresult = controller->send_and_wait(treq);
	LOG_EMPERY_CENTER_DEBUG("Controller response: code = ", tresult.first, ", msg = ", tresult.second);
	if(tresult.first == Msg::ERR_INVALIDATION_IN_PROGRESS){
		const auto wait_delay = get_config<std::uint64_t>("account_invalidation_wait_delay", 15000);

		const auto promise = boost::make_shared<Poseidon::JobPromise>();
		const auto timer = Poseidon::TimerDaemon::register_timer(wait_delay, 0, std::bind([=]() mutable { promise->set_success(); }));
		LOG_EMPERY_CENTER_DEBUG("Waiting for account invalidation...");
		Poseidon::JobDispatcher::yield(promise, true);

		tresult = controller->send_and_wait(treq);
	}
	if(tresult.first != 0){
		LOG_EMPERY_CENTER_INFO("Failed to acquire controller token: code = ", tresult.first, ", msg = ", tresult.second);
		DEBUG_THROW(Exception, sslit("Failed to acquire controller token"));
	}

	*/
}

boost::shared_ptr<Legion> LegionMap::get(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto it = account_map->find<0>(account_uuid);
	if(it == account_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("legion not found: account_uuid = ", account_uuid);
		return { };
	}
	return it->account;
}
boost::shared_ptr<Legion> LegionMap::require(LegionUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Legion not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Legion not found"));
	}
	return account;
}
boost::shared_ptr<Legion> LegionMap::get_or_reload(LegionUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		account = forced_reload(account_uuid);
	}
	return account;
}
boost::shared_ptr<Legion> LegionMap::forced_reload(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_Legion>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_Legion` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_Legion>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_Legion", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_DEBUG("Legion not found in database: account_uuid = ", account_uuid);
		return { };
	}

	auto account = reload_account_aux(std::move(sink->front()));

	const auto elem = LegionElement(account);
	const auto result = account_map->insert(elem);
	if(!result.second){
		account_map->replace(result.first, elem);
	}

//	LOG_EMPERY_CENTER_DEBUG("Successfully reloaded account: account_uuid = ", account_uuid);
	return std::move(account);
}
boost::shared_ptr<Legion> LegionMap::get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Center_Legion map not loaded.");
		return { };
	}

	/*
	const auto key = std::make_pair(platform_id, hash_string_nocase(login_name));
	const auto range = account_map->equal_range<1>(key);
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->account->get_login_name(), login_name)){
			return it->account;
		}
	}
	*/
	LOG_EMPERY_CENTER_DEBUG("Login name not found. Reloading: platform_id = ", platform_id, ", login_name = ", login_name);

	return forced_reload_by_login_name(platform_id, login_name);

}
boost::shared_ptr<Legion> LegionMap::forced_reload_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Center_Legion map not loaded.");
		return { };
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_Legion>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_Legion` WHERE `platform_id` = " <<platform_id
		    <<" AND `login_name` = " <<Poseidon::MySql::StringEscaper(login_name);
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_Legion>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_Legion", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_DEBUG("Legion not found in database: platform_id = ", platform_id, ", login_name = ", login_name);
		return { };
	}

	auto account = reload_account_aux(std::move(sink->front()));
	//const auto account_uuid = account->get_account_uuid();

	const auto elem = LegionElement(account);
	const auto result = account_map->insert(elem);
	if(!result.second){
		account_map->replace(result.first, elem);
	}

//	LOG_EMPERY_CENTER_DEBUG("Successfully reloaded account: account_uuid = ", account_uuid,
//		", platform_id = ", platform_id, ", login_name = ", login_name);
	return std::move(account);
}

std::uint64_t LegionMap::get_count(){
	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Center_Legion map not loaded.");
		return 0;
	}

	return account_map->size();
}
void LegionMap::get_all(std::vector<boost::shared_ptr<Legion>> &ret, std::uint64_t begin, std::uint64_t count){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		return;
	}

	really_append_account(ret,
		std::make_pair(account_map->begin<0>(), account_map->end<0>()),
		begin, count);
}
void LegionMap::get_banned(std::vector<boost::shared_ptr<Legion>> &ret, std::uint64_t begin, std::uint64_t count){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		return;
	}
/*
	really_append_account(ret,
		std::make_pair(account_map->upper_bound<4>(0), account_map->end<4>()),
		begin, count);

		*/
}
void LegionMap::get_quieted(std::vector<boost::shared_ptr<Legion>> &ret, std::uint64_t begin, std::uint64_t count){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		return;
	}

	/*
	really_append_account(ret,
		std::make_pair(account_map->upper_bound<5>(0), account_map->end<5>()),
		begin, count);

		*/
}
void LegionMap::get_by_nick(std::vector<boost::shared_ptr<Legion>> &ret, const std::string &nick){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		return;
	}

	const auto key = hash_string_nocase(nick);
	const auto range = account_map->equal_range<1>(key);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(!are_strings_equal_nocase(it->account->get_nick(), nick)){
			continue;
		}
		ret.emplace_back(it->account);
	}

}
void LegionMap::get_by_referrer(std::vector<boost::shared_ptr<Legion>> &ret, AccountUuid referrer_uuid){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		return;
	}

	auto range = account_map->equal_range<2>(referrer_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->account);
	}

}

void LegionMap::insert(const boost::shared_ptr<Legion> &legion, const std::string &remote_ip){
	PROFILE_ME;

	const auto &legion_map = g_legion_map;
	if(!legion_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		DEBUG_THROW(Exception, sslit("Legion map not loaded"));
	}


	const auto legion_uuid = legion->get_account_uuid();

//	const auto withdrawn = boost::make_shared<bool>(true);
//	Poseidon::async_raise_event(boost::make_shared<Events::LegionCreated>(legion_uuid, remote_ip), withdrawn);

	LOG_EMPERY_CENTER_DEBUG("Inserting legion: account_uuid = ", legion_uuid);
	if(!legion_map->insert(LegionElement(legion)).second){
		LOG_EMPERY_CENTER_WARNING("Legion already exists: account_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("Legion already exists"));
	}

//	*withdrawn = false;
/*
	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			legion->synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	*/
}

void LegionMap::update(const boost::shared_ptr<Legion> &account, bool throws_if_not_exists){
	PROFILE_ME;

	const auto &account_map = g_legion_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Legion map not loaded"));
		}
		return;
	}

	/*
	const auto account_uuid = account->get_account_uuid();

	const auto it = account_map->find<0>(account_uuid);
	if(it == account_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Legion not found: account_uuid = ", account_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Legion not found"));
		}
		return;
	}
	if(it->account != account){
		LOG_EMPERY_CENTER_DEBUG("Legion expired: account_uuid = ", account_uuid);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating Legion: account_uuid = ", account_uuid);
	account_map->replace<0>(it, LegionElement(account));

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			account->synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	*/
}

void LegionMap::deletelegion(LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto &legion_map = g_legion_map;
	if(!legion_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		DEBUG_THROW(Exception, sslit("Legion map not loaded"));
	}

	const auto it = legion_map->find<0>(legion_uuid);
	if(it != legion_map->end<0>()){
		const auto legion = it->account;

		// 军团解散的善后操作
		legion->disband();

		// 先从内存中删除
		legion_map->erase<0>(it);
		LOG_EMPERY_CENTER_INFO("deletemember legion size==============================================",get_count());
		// 从数据库中删除该军团
		std::string strsql = "DELETE FROM Center_Legion WHERE legion_uuid='";
		strsql += legion_uuid.str();
		strsql += "';";

		LOG_EMPERY_CENTER_INFO("LegionMap::deletelegion==============================================",strsql);
		Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_Legion",strsql);
	}
}

void LegionMap::synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
	bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box)
{
	PROFILE_ME;

	const auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Legion not found: account_uuid = ", account_uuid);
		return;
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<std::uint64_t>("account_info_cache_timeout", 0);

	const auto info_cache_map = g_info_cache_map.lock();
	if(info_cache_map){
		info_cache_map->erase<1>(info_cache_map->begin<1>(), info_cache_map->upper_bound<1>(now));
	}

	synchronize_account_and_update_cache(now, cache_timeout, account, item_box, session,
		(wants_nick               ? CT_NICK       : CT_NONE) |
		(wants_attributes         ? CT_ATTRS      : CT_NONE) |
		(wants_private_attributes ? CT_PRIV_ATTRS : CT_NONE) |
		(item_box                 ? CT_ITEMS      : CT_NONE));
}

void LegionMap::cached_synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
	bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box)
{
	PROFILE_ME;

	const auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Legion not found: account_uuid = ", account_uuid);
		return;
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<std::uint64_t>("account_info_cache_timeout", 0);

	const auto info_cache_map = g_info_cache_map.lock();
	if(info_cache_map){
		info_cache_map->erase<1>(info_cache_map->begin<1>(), info_cache_map->upper_bound<1>(now));
	}

	const auto is_miss = [&](CacheType type){
		if(!info_cache_map){
			return true;
		}
		auto elem = InfoCacheElement(account_uuid, session, type, 0);
		const auto it = info_cache_map->find<0>(elem.key);
		if(it == info_cache_map->end<0>()){
			return true;
		}
		return false;
	};

	synchronize_account_and_update_cache(now, cache_timeout, account, item_box, session,
		(wants_nick               && is_miss(CT_NICK      ) ? CT_NICK       : CT_NONE) |
		(wants_attributes         && is_miss(CT_ATTRS     ) ? CT_ATTRS      : CT_NONE) |
		(wants_private_attributes && is_miss(CT_PRIV_ATTRS) ? CT_PRIV_ATTRS : CT_NONE) |
		(item_box                 && is_miss(CT_ITEMS     ) ? CT_ITEMS      : CT_NONE));
}
void LegionMap::cached_synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session){
	return cached_synchronize_account_with_player_all(account_uuid, session, true, true, false, { });
}

}
