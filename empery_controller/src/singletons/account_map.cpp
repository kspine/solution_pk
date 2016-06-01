#include "../precompiled.hpp"
#include "account_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
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

	boost::weak_ptr<AccountContainer> g_account_map;

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
/*
boost::shared_ptr<Account> get(AccountUuid account_uuid);
boost::shared_ptr<Account> require(AccountUuid account_uuid);
boost::shared_ptr<Account> reload(AccountUuid account_uuid);
void update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists = true);
*/
}
