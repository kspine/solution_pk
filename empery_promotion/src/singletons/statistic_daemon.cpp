#include "../precompiled.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include <poseidon/http/utilities.hpp>
#include <poseidon/atomic.hpp>
#include "global_status.hpp"
#include "account_map.hpp"
#include "item_map.hpp"
#include "../item_ids.hpp"
#include "../events/item.hpp"
#include "../mysql/income_balance_history.hpp"
#include "../mysql/outcome_balance_history.hpp"

namespace EmperyPromotion {

namespace {
	volatile boost::uint32_t g_autoId = 0;
}

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

			AccountMap::AccountInfo info;

#define PUT_NUMBER(param_)      oss <<event->param_ <<',';
#define PUT_ACCOUNT(param_)     info = AccountMap::get(AccountId(event->param_));	\
                                oss <<Poseidon::Http::base64Encode(info.loginName) <<','	\
                                    <<Poseidon::Http::base64Encode(info.nick) <<',';
#define PUT_STRING(param_)      oss <<Poseidon::Http::base64Encode(event->param_) <<',';

			std::ostringstream oss;
			switch(event->reason){
			case Events::ItemChanged::R_ADMIN_OPERATION:
				PUT_STRING(remarks)
				break;
			case Events::ItemChanged::R_TRANSFER:
				PUT_ACCOUNT(param1)
				PUT_ACCOUNT(param2)
				PUT_STRING(remarks)
				break;
			case Events::ItemChanged::R_UPGRADE_ACCOUNT:
			case Events::ItemChanged::R_CREATE_ACCOUNT:
			case Events::ItemChanged::R_BALANCE_BONUS:
			case Events::ItemChanged::R_BALANCE_BONUS_EXTRA:
				PUT_ACCOUNT(param1)
				PUT_ACCOUNT(param2)
				PUT_NUMBER(param3)
				PUT_STRING(remarks)
				break;
			case Events::ItemChanged::R_RECHARGE:
			case Events::ItemChanged::R_WITHDRAW:
			case Events::ItemChanged::R_COMMIT_WITHDRAWAL:
			case Events::ItemChanged::R_DEACTIVATE_ACCOUNT:
				PUT_ACCOUNT(param1)
				PUT_STRING(remarks)
				break;
			case Events::ItemChanged::R_INCOME_TAX:
				PUT_NUMBER(param1)
				PUT_ACCOUNT(param2)
				break;
			case Events::ItemChanged::R_BUY_MORE_CARDS:
				PUT_ACCOUNT(param1)
				PUT_NUMBER(param2)
				PUT_STRING(remarks)
				break;
			default:
				LOG_EMPERY_PROMOTION_WARNING("Unknown reason: ", (unsigned)event->reason, ", param1 = ", event->param1,
					", param2 = ", event->param2, ", param3 = ", event->param3, ", remarks = ", event->remarks);
				break;
			}
			if(event->oldCount < event->newCount){
				const auto obj = boost::make_shared<MySql::Promotion_IncomeBalanceHistory>(
					event->accountId.get(), localNow, Poseidon::atomicAdd(g_autoId, 1, Poseidon::ATOMIC_RELAXED),
					event->newCount - event->oldCount, event->reason, event->param1, event->param2, event->param3, oss.str());
				obj->asyncSave(true);
			} else if(event->oldCount > event->newCount){
				const auto obj = boost::make_shared<MySql::Promotion_OutcomeBalanceHistory>(
					event->accountId.get(), localNow, Poseidon::atomicAdd(g_autoId, 1, Poseidon::ATOMIC_RELAXED),
					event->oldCount - event->newCount, event->reason, event->param1, event->param2, event->param3, oss.str());
				obj->asyncSave(true);
			}
		});
	handles.push(listener);
}

}
