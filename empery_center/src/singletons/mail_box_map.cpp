#include "../precompiled.hpp"
#include "mail_box_map.hpp"
#include "../mmain.hpp"
#include <poseidon/json.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../mail_box.hpp"
#include "../mail_data.hpp"
#include "../mysql/mail.hpp"
#include "account_map.hpp"
#include "../account.hpp"

namespace EmperyCenter {

namespace {
	struct MailBoxElement {
		AccountUuid account_uuid;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::vector<boost::shared_ptr<MySql::Center_Mail>>> sink;

		mutable boost::shared_ptr<MailBox> mail_box;
		mutable boost::shared_ptr<Poseidon::TimerItem> timer;

		MailBoxElement(AccountUuid account_uuid_, std::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(MailBoxContainer, MailBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<MailBoxContainer> g_mail_box_map;

	struct MailDataElement {
		std::pair<MailUuid, LanguageId> pkey;
		std::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<boost::container::flat_map<LanguageId, boost::shared_ptr<MySql::Center_MailData>>> sink;

		mutable boost::shared_ptr<MySql::Center_MailData> mail_data_obj;
		mutable boost::shared_ptr<MailData> mail_data;

		MailDataElement(MailUuid mail_uuid_, LanguageId language_id_, std::uint64_t unload_time_)
			: pkey(mail_uuid_, language_id_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(MailDataContainer, MailDataElement,
		UNIQUE_MEMBER_INDEX(pkey)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<MailDataContainer> g_mail_data_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Mail box gc timer: now = ", now);

		const auto mail_box_map = g_mail_box_map.lock();
		if(!mail_box_map){
			return;
		}

		for(;;){
			const auto it = mail_box_map->begin<1>();
			if(it == mail_box_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->mail_box.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				mail_box_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming mail box: account_uuid = ", it->account_uuid);
				mail_box_map->erase<1>(it);
			}
		}
	}

	void data_gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Mail data gc timer: now = ", now);

		const auto mail_data_map = g_mail_data_map.lock();
		if(!mail_data_map){
			return;
		}

		for(;;){
			const auto it = mail_data_map->begin<1>();
			if(it == mail_data_map->end<1>()){
				break;
			}
			if(now < it->unload_time){
				break;
			}

			// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
			if((it->promise.use_count() | it->mail_data.use_count()) != 1){ // (a > 1) || (b > 1) || ((a == 0) && b == 0))
				mail_data_map->set_key<1, 1>(it, now + 1000);
			} else {
				LOG_EMPERY_CENTER_DEBUG("Reclaiming mail data: mail_uuid = ", it->pkey.first);
				mail_data_map->erase<1>(it);
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto mail_box_map = boost::make_shared<MailBoxContainer>();
		g_mail_box_map = mail_box_map;
		handles.push(mail_box_map);

		const auto mail_data_map = boost::make_shared<MailDataContainer>();
		g_mail_data_map = mail_data_map;
		handles.push(mail_data_map);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);

		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);

		timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&data_gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}

//	const auto GLOBAL_MAIL_ACCCOUNT_UUID = AccountUuid("10000000-F660-0008-CEF2-0DDD8AD2585C");
}

boost::shared_ptr<MailBox> MailBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto mail_box_map = g_mail_box_map.lock();
	if(!mail_box_map){
		LOG_EMPERY_CENTER_WARNING("MailBoxMap is not loaded.");
		return { };
	}
/*
	if(account_uuid == GLOBAL_MAIL_ACCCOUNT_UUID){
		auto account = AccountMap::get(account_uuid);
		if(!account){
			const auto utc_now = Poseidon::get_utc_time();
			account = boost::make_shared<Account>(account_uuid,
				PlatformId(1), std::string(), AccountUuid(), 0, utc_now, "Global mail account");
			AccountMap::insert(account, std::string());
		}
	}
*/
	auto it = mail_box_map->find<0>(account_uuid);
	if(it == mail_box_map->end<0>()){
		it = mail_box_map->insert<0>(it, MailBoxElement(account_uuid, 0));
	}
	if(!it->mail_box){
		if(!AccountMap::is_holding_controller_token(account_uuid)){
			LOG_EMPERY_CENTER_DEBUG("Failed to acquire controller token: account_uuid = ", account_uuid);
			return { };
		}

		LOG_EMPERY_CENTER_DEBUG("Loading mail box: account_uuid = ", account_uuid);

		boost::shared_ptr<const Poseidon::JobPromise> promise_tack;
		do {
			if(!it->promise){
				auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_Mail>>>();
				std::ostringstream oss;
				const auto utc_now = Poseidon::get_utc_time();
				oss <<"SELECT * FROM `Center_Mail` WHERE `expiry_time` > " <<Poseidon::MySql::DateTimeFormatter(utc_now)
				    <<"  AND `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
				auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
					[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
						auto obj = boost::make_shared<MySql::Center_Mail>();
						obj->fetch(conn);
						obj->enable_auto_saving();
						sink->emplace_back(std::move(obj));
					}, "Center_Mail", oss.str());
				it->promise = std::move(promise);
				it->sink    = std::move(sink);
			}
			// 复制一个智能指针，并且导致 use_count() 增加。
			// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
			promise_tack = it->promise;
			Poseidon::JobDispatcher::yield(promise_tack, true);
		} while(promise_tack != it->promise);

		if(it->sink){
			mail_box_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			auto mail_box = boost::make_shared<MailBox>(account_uuid, *(it->sink));

			const auto mail_box_refresh_interval = get_config<std::uint64_t>("mail_box_refresh_interval", 60000);
			auto timer = Poseidon::TimerDaemon::register_timer(0, mail_box_refresh_interval,
				std::bind([](const boost::weak_ptr<MailBox> &weak){
					PROFILE_ME;
					const auto mail_box = weak.lock();
					if(!mail_box){
						return;
					}
					mail_box->pump_status();
				}, boost::weak_ptr<MailBox>(mail_box))
			);

			it->promise.reset();
			it->sink.reset();
			it->mail_box = std::move(mail_box);
			it->timer = std::move(timer);
		}

		assert(it->mail_box);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	mail_box_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->mail_box;
}
boost::shared_ptr<MailBox> MailBoxMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto ret = get(account_uuid);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("MailBox not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("MailBox not found"));
	}
	return ret;
}
void MailBoxMap::unload(AccountUuid account_uuid){
	PROFILE_ME;

	const auto mail_box_map = g_mail_box_map.lock();
	if(!mail_box_map){
		LOG_EMPERY_CENTER_WARNING("MailBoxMap is not loaded.");
		return;
	}

	const auto it = mail_box_map->find<0>(account_uuid);
	if(it == mail_box_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Mail box not loaded: account_uuid = ", account_uuid);
		return;
	}

	mail_box_map->set_key<0, 1>(it, 0);
	it->promise.reset();
	const auto now = Poseidon::get_fast_mono_clock();
	gc_timer_proc(now);
}
/*
boost::shared_ptr<MailBox> MailBoxMap::get_global(){
	PROFILE_ME;

	return get(GLOBAL_MAIL_ACCCOUNT_UUID);
}
boost::shared_ptr<MailBox> MailBoxMap::require_global(){
	PROFILE_ME;

	return require(GLOBAL_MAIL_ACCCOUNT_UUID);
}
*/
boost::shared_ptr<MailData> MailBoxMap::get_mail_data(MailUuid mail_uuid, LanguageId language_id){
	PROFILE_ME;

	const auto mail_data_map = g_mail_data_map.lock();
	if(!mail_data_map){
		LOG_EMPERY_CENTER_WARNING("MailDataMap is not loaded.");
		return { };
	}

	auto it = mail_data_map->find<0>(std::make_pair(mail_uuid, language_id));
	if(it == mail_data_map->end<0>()){
		it = mail_data_map->insert<0>(it, MailDataElement(mail_uuid, language_id, 0));
	}
	if(!it->mail_data){
		LOG_EMPERY_CENTER_DEBUG("Loading mail data: mail_uuid = ", mail_uuid, ", language_id = ", language_id);

		if(!it->promise){
			auto sink = boost::make_shared<boost::container::flat_map<LanguageId, boost::shared_ptr<MySql::Center_MailData>>>();
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_MailData` WHERE `mail_uuid` = " <<Poseidon::MySql::UuidFormatter(mail_uuid.get());
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
				[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
					auto obj = boost::make_shared<MySql::Center_MailData>();
					obj->fetch(conn);
					obj->enable_auto_saving();
					const auto language_id = LanguageId(obj->get_language_id());
					sink->emplace(language_id, std::move(obj));
				}, "Center_MailData", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}
		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise, true);

		if(it->sink){
			mail_data_map->set_key<0, 1>(it, 0);
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: mail_uuid = ", mail_uuid, ", rows = ", it->sink->size());

			boost::shared_ptr<MySql::Center_MailData> obj;
			decltype(it->sink->cbegin()) sit;
			if((sit = it->sink->find(language_id)) != it->sink->end()){
				obj = sit->second;
			} else if((sit = it->sink->find(LanguageId(0))) != it->sink->end()){
				obj = sit->second;
			} else if((sit = it->sink->begin()) != it->sink->end()){
				obj = sit->second;
			}
			if(!obj){
				LOG_EMPERY_CENTER_WARNING("Mail data not found: mail_uuid = ", mail_uuid, ", rows = ", it->sink->size());
				DEBUG_THROW(Exception, sslit("Mail data not found"));
			}

			auto mail_data = boost::make_shared<MailData>(std::move(obj));

			it->promise.reset();
			it->sink.reset();
			it->mail_data = std::move(mail_data);
		}

		assert(it->mail_data);
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
	mail_data_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->mail_data;
}
boost::shared_ptr<MailData> MailBoxMap::require_mail_data(MailUuid mail_uuid, LanguageId language_id){
	PROFILE_ME;

	auto ret = get_mail_data(mail_uuid, language_id);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("Mail data not found: mail_uuid = ", mail_uuid, ", language_id = ", language_id);
		DEBUG_THROW(Exception, sslit("Mail data not found"));
	}
	return ret;
}
void MailBoxMap::insert_mail_data(boost::shared_ptr<MailData> mail_data){
	PROFILE_ME;

	const auto mail_data_map = g_mail_data_map.lock();
	if(!mail_data_map){
		LOG_EMPERY_CENTER_WARNING("Mail data map is not loaded.");
		DEBUG_THROW(Exception, sslit("Mail data map is not loaded"));
	}

	const auto mail_uuid = mail_data->get_mail_uuid();
	const auto language_id = mail_data->get_language_id();

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);

	const auto result = mail_data_map->insert(MailDataElement(mail_uuid, language_id, saturated_add(now, gc_interval)));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Mail data already exists: mail_uuid = ", mail_uuid, ", language_id = ", language_id);
		DEBUG_THROW(Exception, sslit("Mail data already exists"));
	}

	result.first->mail_data = std::move(mail_data);
}

}
