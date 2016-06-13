#include "../precompiled.hpp"
#include "account_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../account.hpp"
#include "../controller_session.hpp"
#include "../../../empery_center/src/mysql/account.hpp"
#include "../../../empery_center/src/string_utilities.hpp"

namespace EmperyController {

using EmperyCenter::hash_string_nocase;
using EmperyCenter::are_strings_equal_nocase;

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

	const auto it = g_account_map->find<0>(account_uuid);
	if(it == g_account_map->end<0>()){
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

	std::vector<boost::shared_ptr<EmperyCenter::MySql::Center_Account>> sink;
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_Account` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[&](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<EmperyCenter::MySql::Center_Account>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink.emplace_back(std::move(obj));
			}, "Center_Account", oss.str());
		Poseidon::JobDispatcher::yield(promise, false);
	}
	if(sink.empty()){
		LOG_EMPERY_CONTROLLER_DEBUG("Account not found in database: account_uuid = ", account_uuid);
		return { };
	}

	auto account = boost::make_shared<Account>(std::move(sink.front()));

	auto it = g_account_map->find<0>(account_uuid);
	if(it == g_account_map->end<0>()){
		// account->set_controller({ });
		it = g_account_map->insert<0>(AccountElement(account)).first;
	} else {
		account->set_controller(it->weak_controller.lock());
		g_account_map->replace<0>(it, AccountElement(account));
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Successfully reloaded account: account_uuid = ", account_uuid);
	return std::move(account);
}
boost::shared_ptr<Account> AccountMap::get_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	const auto key = std::make_pair(platform_id, hash_string_nocase(login_name));
	const auto range = g_account_map->equal_range<2>(key);
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->account->get_login_name(), login_name)){
			return it->account;
		}
	}
	LOG_EMPERY_CONTROLLER_DEBUG("Login name not found: platform_id = ", platform_id, ", login_name = ", login_name);
	return { };
}
boost::shared_ptr<Account> AccountMap::require_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	auto account = get_by_login_name(platform_id, login_name);
	if(!account){
		LOG_EMPERY_CONTROLLER_WARNING("Login name not found: platform_id = ", platform_id, ", login_name = ", login_name);
		DEBUG_THROW(Exception, sslit("Login name not found"));
	}
	return account;
}

void AccountMap::update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists){
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	const auto it = g_account_map->find<0>(account_uuid);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_CONTROLLER_WARNING("Account not found: account_uuid = ", account_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Account not found"));
		}
		return;
	}

	LOG_EMPERY_CONTROLLER_DEBUG("Updating account: account_uuid = ", account_uuid);
	g_account_map->replace<0>(it, AccountElement(account));
}

void AccountMap::get_all_controllers(std::vector<boost::shared_ptr<ControllerSession>> &ret){
	PROFILE_ME;

	ret.reserve(64);
	for(auto it = g_account_map->begin<1>(); it != g_account_map->end<1>(); it = g_account_map->upper_bound<1>(it->weak_controller)){
		auto controller = it->weak_controller.lock();
		if(!controller){
			continue;
		}
		ret.emplace_back(std::move(controller));
	}
}

}
