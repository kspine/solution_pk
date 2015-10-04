#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include <poseidon/http/utilities.hpp>
#include "global_status.hpp"
#include "account_map.hpp"
#include "item_map.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"
#include "../mysql/income_balance_history.hpp"
#include "../mysql/outcome_balance_history.hpp"

namespace EmperyPromotion {

MODULE_RAII_PRIORITY(handles, 9000){
	auto listener = Poseidon::EventDispatcher::registerListener<Events::ItemChanged>(
		[](const boost::shared_ptr<Events::ItemChanged> &event){
			PROFILE_ME;

			if(event->itemId != ItemIds::ID_ACCOUNT_BALANCE){
				return;
			}
			if(event->oldCount == event->newCount){
				return;
			}
			LOG_EMPERY_PROMOTION_INFO("Writing account balance records...");

			const auto localNow = Poseidon::getLocalTime();

			std::ostringstream oss;
			AccountMap::AccountInfo info;
			switch(event->reason){
			case Events::ItemChanged::R_ADMIN_OPERATION:
				break;
			case Events::ItemChanged::R_TRANSFER:
				info = AccountMap::get(AccountId(event->param1));
				oss <<Poseidon::Http::base64Encode(info.loginName) <<',' <<Poseidon::Http::base64Encode(info.nick) <<',';
				info = AccountMap::get(AccountId(event->param2));
				oss <<Poseidon::Http::base64Encode(info.loginName) <<',' <<Poseidon::Http::base64Encode(info.nick) <<',';
				oss <<Poseidon::Http::base64Encode(event->remarks) <<',';
				break;
			case Events::ItemChanged::R_UPGRADE_ACCOUNT:
			case Events::ItemChanged::R_CREATE_ACCOUNT:
				info = AccountMap::get(AccountId(event->param1));
				oss <<Poseidon::Http::base64Encode(info.loginName) <<',' <<Poseidon::Http::base64Encode(info.nick) <<',';
				info = AccountMap::get(AccountId(event->param2));
				oss <<Poseidon::Http::base64Encode(info.loginName) <<',' <<Poseidon::Http::base64Encode(info.nick) <<',';
				oss <<event->param3 <<',';
				oss <<Poseidon::Http::base64Encode(event->remarks) <<',';
				break;
			case Events::ItemChanged::R_BALANCE_BONUS:
			case Events::ItemChanged::R_BALANCE_BONUS_EXTRA:
				info = AccountMap::get(AccountId(event->param1));
				oss <<Poseidon::Http::base64Encode(info.loginName) <<',' <<Poseidon::Http::base64Encode(info.nick) <<',';
				info = AccountMap::get(AccountId(event->param2));
				oss <<Poseidon::Http::base64Encode(info.loginName) <<',' <<Poseidon::Http::base64Encode(info.nick) <<',';
				oss <<event->param3 <<',';
				break;
			case Events::ItemChanged::R_RECHARGE:
			case Events::ItemChanged::R_WITHDRAW:
				info = AccountMap::get(AccountId(event->param1));
				oss <<Poseidon::Http::base64Encode(info.loginName) <<',' <<Poseidon::Http::base64Encode(info.nick) <<',';
				break;
			case Events::ItemChanged::R_COMMIT_WITHDRAWAL:
				break;
			case Events::ItemChanged::R_INCOME_TAX:
				oss <<event->param1 <<',';
				break;
			default:
				LOG_EMPERY_PROMOTION_WARNING("Unknown reason: ", (unsigned)event->reason);
				break;
			}
			if(event->oldCount < event->newCount){
				const auto obj = boost::make_shared<MySql::Promotion_IncomeBalanceHistory>(
					event->accountId.get(), localNow, event->newCount - event->oldCount,
					event->reason, event->param1, event->param2, event->param3, oss.str());
				obj->asyncSave(true);
			} else {
				const auto obj = boost::make_shared<MySql::Promotion_OutcomeBalanceHistory>(
					event->accountId.get(), localNow, event->oldCount - event->newCount,
					event->reason, event->param1, event->param2, event->param3, oss.str());
				obj->asyncSave(true);
			}
		});
	handles.push(listener);
}

}
