#include "../precompiled.hpp"
#include "account_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../account.hpp"
#include "../controller_session.hpp"
#include "../../../empery_center/src/mysql/account.hpp"
#include "../string_utilities.hpp"

namespace EmperyController {

namespace {
	struct AccountElement {
		boost::shared_ptr<Account> account;

		AccountUuid account_uuid;
		boost::weak_ptr<ControllerSession> weak_controller;
		std::pair<PlatformId, std::size_t> platform_id_login_name_hash;

		explicit AccountElement(boost::shared_ptr<Account> account_)
			: account(std::move(account_))
			, account_uuid(account->get_account_uuid()), weak_controller(account->get_weak_controller())
			, platform_id_login_name_hash(account->get_platform_id(), hash_string_nocase(account->get_login_name()))
		{
		}
	};

	MULTI_INDEX_MAP(AccountContainer, AccountElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(weak_controller)
		MULTI_MEMBER_INDEX(platform_id_login_name_hash)
	)

	boost::shared_ptr<AccountContainer> g_account_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto account_map = boost::make_shared<AccountContainer>();
		LOG_EMPERY_CONTROLLER_INFO("Loading accounts...");
		conn->execute_sql("SELECT * FROM `Center_Account`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<EmperyCenter::MySql::Center_Account>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			auto account = boost::make_shared<Account>(std::move(obj));
			account_map->insert(AccountElement(std::move(account)));
		}
		LOG_EMPERY_CONTROLLER_INFO("Loaded ", account_map->size(), " account(s).");
		g_account_map = account_map;
		handles.push(account_map);
	}
}

boost::shared_ptr<Account> AccountMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_account_map;
	if(!account_map){
		LOG_EMPERY_CONTROLLER_WARNING("Account map not loaded.");
		return { };
	}

	const auto it = account_map->find<0>(account_uuid);
	if(it == account_map->end<0>()){
		LOG_EMPERY_CONTROLLER_TRACE("Account not found: account_uuid = ", account_uuid);
		return { };
	}
	return it->account;
}
boost::shared_ptr<Account> AccountMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CONTROLLER_WARNING("Account not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	return account;
}
boost::shared_ptr<Account> AccountMap::get_or_reload(AccountUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		account = forced_reload(account_uuid);
	}
	return account;
}
boost::shared_ptr<Account> AccountMap::forced_reload(AccountUuid account_uuid){
	PROFILE_ME;
	LOG_EMPERY_CONTROLLER_DEBUG("Reloading account: account_uuid = ", account_uuid);

	const auto &account_map = g_account_map;
	if(!account_map){
		LOG_EMPERY_CONTROLLER_WARNING("Account map not loaded.");
		return { };
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<EmperyCenter::MySql::Center_Account>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_Account` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_Account>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_Account", oss.str());
		Poseidon::JobDispatcher::yield(promise, false);
	}
	if(sink->empty()){
		account_map->erase<0>(account_uuid);
		LOG_EMPERY_CONTROLLER_DEBUG("Account not found in database: account_uuid = ", account_uuid);
		return { };
	}

	auto account = boost::make_shared<Account>(std::move(sink->front()));

	const auto old_it = account_map->find<0>(account_uuid);
	if(old_it != account_map->end<0>()){
		account->set_controller(old_it->weak_controller.lock());
	}

	const auto elem = AccountElement(account);
	const auto result = account_map->insert(elem);
	if(!result.second){
		account_map->replace(result.first, elem);
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Successfully reloaded account: account_uuid = ", account_uuid);
	return std::move(account);
}
boost::shared_ptr<Account> AccountMap::get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	const auto &account_map = g_account_map;
	if(!account_map){
		LOG_EMPERY_CONTROLLER_WARNING("Account map not loaded.");
		return { };
	}

	const auto key = std::make_pair(platform_id, hash_string_nocase(login_name));
	const auto range = account_map->equal_range<2>(key);
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->account->get_login_name(), login_name)){
			return it->account;
		}
	}
	LOG_EMPERY_CONTROLLER_DEBUG("Login name not found. Reloading: platform_id = ", platform_id, ", login_name = ", login_name);

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<EmperyCenter::MySql::Center_Account>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_Account` WHERE `platform_id` = " <<platform_id
		    <<" AND `login_name` = " <<Poseidon::MySql::StringEscaper(login_name);
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_Account>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_Account", oss.str());
		Poseidon::JobDispatcher::yield(promise, false);
	}
	if(sink->empty()){
		// account_map->erase<0>(account_uuid);
		LOG_EMPERY_CONTROLLER_DEBUG("Account not found in database: platform_id = ", platform_id, ", login_name = ", login_name);
		return { };
	}

	auto account = boost::make_shared<Account>(std::move(sink->front()));
	const auto account_uuid = account->get_account_uuid();

	const auto old_it = account_map->find<0>(account_uuid);
	if(old_it != account_map->end<0>()){
		account->set_controller(old_it->weak_controller.lock());
	}

	const auto elem = AccountElement(account);
	const auto result = account_map->insert(elem);
	if(!result.second){
		account_map->replace(result.first, elem);
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Successfully reloaded account: account_uuid = ", account_uuid,
		", platform_id = ", platform_id, ", login_name = ", login_name);
	return std::move(account);
}

void AccountMap::update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists){
	PROFILE_ME;

	const auto &account_map = g_account_map;
	if(!account_map){
		LOG_EMPERY_CONTROLLER_WARNING("Account map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Account map not loaded"));
		}
		return;
	}

	const auto account_uuid = account->get_account_uuid();

	const auto it = account_map->find<0>(account_uuid);
	if(it == account_map->end<0>()){
		LOG_EMPERY_CONTROLLER_WARNING("Account not found: account_uuid = ", account_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Account not found"));
		}
		return;
	}
	if(it->account != account){
		LOG_EMPERY_CONTROLLER_DEBUG("Account expired: account_uuid = ", account_uuid);
		return;
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Updating account: account_uuid = ", account_uuid);
	account_map->replace<0>(it, AccountElement(account));
}

void AccountMap::get_all_controllers(std::vector<boost::shared_ptr<ControllerSession>> &ret){
	PROFILE_ME;

	const auto &account_map = g_account_map;
	if(!account_map){
		LOG_EMPERY_CONTROLLER_WARNING("Account map not loaded.");
		return;
	}

	ret.reserve(64);
	for(auto it = account_map->begin<1>(); it != account_map->end<1>(); it = account_map->upper_bound<1>(it->weak_controller)){
		auto controller = it->weak_controller.lock();
		if(!controller){
			continue;
		}
		ret.emplace_back(std::move(controller));
	}
}

}
