#include "../precompiled.hpp"
#include "mail_box_map.hpp"
#include <poseidon/json.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "account_map.hpp"
#include "../mail_box.hpp"
#include "../mail_data.hpp"
#include "../mysql/mail.hpp"

namespace EmperyCenter {

namespace {
	struct MailBoxElement {
		AccountUuid account_uuid;
		boost::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>> sink;

		mutable boost::shared_ptr<MailBox> mail_box;
		mutable boost::shared_ptr<Poseidon::TimerItem> timer;

		MailBoxElement(AccountUuid account_uuid_, boost::uint64_t unload_time_)
			: account_uuid(account_uuid_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(MailBoxMapDelegator, MailBoxElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<MailBoxMapDelegator> g_mail_box_map;

	struct MailDataElement {
		std::pair<MailUuid, LanguageId> pkey;
		boost::uint64_t unload_time;

		mutable boost::shared_ptr<const Poseidon::JobPromise> promise;
		mutable boost::shared_ptr<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>> sink;

		mutable boost::shared_ptr<MySql::Center_MailData> mail_data_obj;
		mutable boost::shared_ptr<MailData> mail_data;

		MailDataElement(MailUuid mail_uuid_, LanguageId language_id_, boost::uint64_t unload_time_)
			: pkey(mail_uuid_, language_id_), unload_time(unload_time_)
		{
		}
	};

	MULTI_INDEX_MAP(MailDataMapDelegator, MailDataElement,
		UNIQUE_MEMBER_INDEX(pkey)
		MULTI_MEMBER_INDEX(unload_time)
	)

	boost::weak_ptr<MailDataMapDelegator> g_mail_data_map;

	void gc_timer_proc(boost::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Mail box gc timer: now = ", now);

		const auto mail_box_map = g_mail_box_map.lock();
		if(mail_box_map){
			for(;;){
				const auto it = mail_box_map->begin<1>();
				if(it == mail_box_map->end<1>()){
					break;
				}
				if(now < it->unload_time){
					break;
				}

				// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
				if((it->promise.use_count() <= 1) && it->mail_box && it->mail_box.unique()){
					LOG_EMPERY_CENTER_INFO("Reclaiming mail box: account_uuid = ", it->account_uuid);
					mail_box_map->erase<1>(it);
				} else {
					mail_box_map->set_key<1, 1>(it, now + 1000);
				}
			}
		}

		const auto mail_data_map = g_mail_data_map.lock();
		if(mail_data_map){
			for(;;){
				const auto it = mail_data_map->begin<1>();
				if(it == mail_data_map->end<1>()){
					break;
				}
				if(now < it->unload_time){
					break;
				}

				// 判定 use_count() 为 0 或 1 的情况。参看 require() 中的注释。
				if((it->promise.use_count() <= 1) && it->mail_data && it->mail_data.unique()){
					LOG_EMPERY_CENTER_INFO("Reclaiming mail data: mail_uuid = ", it->pkey.first);
					mail_data_map->erase<1>(it);
				} else {
					mail_data_map->set_key<1, 1>(it, now + 1000);
				}
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto mail_box_map = boost::make_shared<MailBoxMapDelegator>();
		g_mail_box_map = mail_box_map;
		handles.push(mail_box_map);

		const auto mail_data_map = boost::make_shared<MailDataMapDelegator>();
		g_mail_data_map = mail_data_map;
		handles.push(mail_data_map);

		const auto gc_interval = get_config<boost::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}

	const auto GLOBAL_MAIL_ACCCOUNT_UUID = AccountUuid("15104D25-DC80-0008-F3E5-6B1D016998D5");
}

boost::shared_ptr<MailBox> MailBoxMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto mail_box_map = g_mail_box_map.lock();
	if(!mail_box_map){
		LOG_EMPERY_CENTER_WARNING("MailBoxMap is not loaded.");
		return { };
	}

	auto it = mail_box_map->find<0>(account_uuid);
	if(it == mail_box_map->end<0>()){
		it = mail_box_map->insert<0>(it, MailBoxElement(account_uuid, 0));
	}
	if(!it->mail_box){
		LOG_EMPERY_CENTER_INFO("Loading mail box: account_uuid = ", account_uuid);

		if(!it->promise){
			auto sink = boost::make_shared<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>>();
			std::ostringstream oss;
			const auto utc_now = Poseidon::get_utc_time();
			oss <<"SELECT * FROM `Center_Mail` WHERE `expiry_time` > " <<utc_now <<" AND `account_uuid` = '" <<account_uuid <<"'";
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(sink,
				&MySql::Center_Mail::create, "Center_Mail", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}
		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise);
		promise->check_and_rethrow();

		if(it->sink){
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed: account_uuid = ", account_uuid, ", rows = ", it->sink->size());

			std::vector<boost::shared_ptr<MySql::Center_Mail>> objs;
			objs.reserve(it->sink->size());
			for(auto sit = it->sink->begin(); sit != it->sink->end(); ++sit){
				const auto &base = *sit;
				auto obj = boost::dynamic_pointer_cast<MySql::Center_Mail>(base);
				if(!obj){
					LOG_EMPERY_CENTER_ERROR("Unexpected dynamic MySQL object type: type = ", typeid(*base).name());
					DEBUG_THROW(Exception, sslit("Unexpected dynamic MySQL object type"));
				}
				objs.emplace_back(std::move(obj));
			}
			auto mail_box = boost::make_shared<MailBox>(account_uuid, objs);

			const auto mail_box_refresh_interval = get_config<boost::uint64_t>("mail_box_refresh_interval", 60000);
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

			it->promise  = { };
			it->sink     = { };
			it->mail_box = std::move(mail_box);
			it->timer    = std::move(timer);
		}
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<boost::uint64_t>("object_gc_interval", 300000);
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

boost::shared_ptr<MailBox> MailBoxMap::get_global(){
	PROFILE_ME;

	return get(GLOBAL_MAIL_ACCCOUNT_UUID);
}
boost::shared_ptr<MailBox> MailBoxMap::require_global(){
	PROFILE_ME;

	return require(GLOBAL_MAIL_ACCCOUNT_UUID);
}

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
		LOG_EMPERY_CENTER_INFO("Loading mail data: mail_uuid = ", mail_uuid, ", language_id = ", language_id);

		if(!it->promise){
			auto sink = boost::make_shared<std::deque<boost::shared_ptr<Poseidon::MySql::ObjectBase>>>();
			std::ostringstream oss;
			oss <<"SELECT * FROM `Center_MailData` WHERE `mail_uuid` = '" <<mail_uuid <<"' AND `language_id` = " <<language_id;
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(sink,
				&MySql::Center_MailData::create, "Center_MailData", oss.str());
			it->promise = std::move(promise);
			it->sink    = std::move(sink);
		}
		// 复制一个智能指针，并且导致 use_count() 增加。
		// 在 GC 定时器中我们用 use_count() 判定是否有异步操作进行中。
		const auto promise = it->promise;
		Poseidon::JobDispatcher::yield(promise);
		promise->check_and_rethrow();

		if(it->sink){
			LOG_EMPERY_CENTER_DEBUG("Async MySQL query completed:  mail_uuid = ", mail_uuid, ", language_id = ", language_id,
				", rows = ", it->sink->size());

			boost::shared_ptr<MySql::Center_MailData> obj;
			if(it->sink->empty()){
				// 创建一个空的。
				obj = boost::make_shared<MySql::Center_MailData>(mail_uuid.get(), language_id.get(),
					0, 0, Poseidon::Uuid(), std::string(), std::string(), std::string());
				// obj->async_save(true); // 不需要。
				obj->enable_auto_saving();
			} else {
				const auto &base = it->sink->front();
				obj = boost::dynamic_pointer_cast<MySql::Center_MailData>(base);
				if(!obj){
					LOG_EMPERY_CENTER_ERROR("Unexpected dynamic MySQL object type: type = ", typeid(*base).name());
					DEBUG_THROW(Exception, sslit("Unexpected dynamic MySQL object type"));
				}
			}
			auto mail_data = boost::make_shared<MailData>(std::move(obj));

			it->promise   = { };
			it->sink      = { };
			it->mail_data = std::move(mail_data);
		}
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto gc_interval = get_config<boost::uint64_t>("object_gc_interval", 300000);
	mail_data_map->set_key<0, 1>(it, saturated_add(now, gc_interval));

	return it->mail_data;
}
boost::shared_ptr<MailData> MailBoxMap::require_mail_data(MailUuid mail_uuid, LanguageId language_id){
	PROFILE_ME;

	auto ret = get_mail_data(mail_uuid, language_id);
	if(!ret){
		LOG_EMPERY_CENTER_WARNING("MailData not found: mail_uuid = ", mail_uuid, ", language_id = ", language_id);
		DEBUG_THROW(Exception, sslit("MailData not found"));
	}
	return ret;
}
void MailBoxMap::insert_mail_data(boost::shared_ptr<MailData> mail_data){
	PROFILE_ME;

	const auto mail_data_map = g_mail_data_map.lock();
	if(!mail_data_map){
		LOG_EMPERY_CENTER_WARNING("MailDataMap is not loaded.");
		DEBUG_THROW(Exception, sslit("MailDataMap is not loaded"));
	}

	const auto mail_uuid = mail_data->get_mail_uuid();
	const auto language_id = mail_data->get_language_id();

	auto it = mail_data_map->find<0>(std::make_pair(mail_uuid, language_id));
	if(it != mail_data_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Mail data already exists: mail_uuid = ", mail_uuid, ", language_id = ", language_id);
		DEBUG_THROW(Exception, sslit("Mail data already exists"));
	}
	it = mail_data_map->insert<0>(it, MailDataElement(mail_uuid, language_id, 0));

	it->mail_data = std::move(mail_data);
}

}
