#include "../precompiled.hpp"
#include "account_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../account.hpp"
#include "../controller_session.hpp"
#include "../../../empery_center/src/mysql/account.hpp"

namespace EmperyController {

namespace {
	struct AccountElement {
		boost::shared_ptr<Account> account;

		AccountUuid account_uuid;
		boost::weak_ptr<ControllerSession> weak_controller;

		explicit AccountElement(boost::shared_ptr<Account> account_)
			: account(std::move(account_))
			, account_uuid(account->get_account_uuid()), weak_controller(account->get_weak_controller())
		{
		}
	};

	MULTI_INDEX_MAP(AccountContainer, AccountElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(weak_controller)
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
boost::shared_ptr<Account> AccountMap::reload(AccountUuid account_uuid){
	PROFILE_ME;

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

}
