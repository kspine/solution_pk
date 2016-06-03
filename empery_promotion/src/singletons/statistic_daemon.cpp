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
#include "../mysql/gold_coin_history.hpp"

namespace EmperyPromotion {

namespace {
	volatile std::uint32_t g_auto_id = 0;
}

MODULE_RAII_PRIORITY(handles, 9000){
	auto listener = Poseidon::EventDispatcher::register_listener<Events::ItemChanged>(
		[](const boost::shared_ptr<Events::ItemChanged> &event){
			PROFILE_ME;

			if(event->old_count == event->new_count){
				return;
			}

			const auto utc_now = Poseidon::get_utc_time();

			if(event->item_id == ItemIds::ID_ACCOUNT_BALANCE){
				LOG_EMPERY_PROMOTION_INFO("Writing account balance records...");

				AccountMap::AccountInfo info;

#define PUT_NUMBER(param_)      oss <<event->param_ <<',';
#define PUT_ACCOUNT(param_)     info = AccountMap::get(AccountId(event->param_));	\
                                oss <<Poseidon::Http::base64_encode(info.login_name) <<','	\
                                    <<Poseidon::Http::base64_encode(info.nick) <<',';
#define PUT_STRING(param_)      oss <<Poseidon::Http::base64_encode(event->param_) <<',';

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
				case Events::ItemChanged::R_ROLLBACK_WITHDRAWAL:
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
				case Events::ItemChanged::R_GOLD_SCRAMBLE_BID:
				case Events::ItemChanged::R_GOLD_SCRAMBLE_REWARD:
					PUT_NUMBER(param1)
					PUT_NUMBER(param2)
					PUT_NUMBER(param3)
					break;
				case Events::ItemChanged::R_SELL_CARDS:
				case Events::ItemChanged::R_SELL_CARDS_ALT:
					PUT_ACCOUNT(param1)
					break;
				case Events::ItemChanged::R_BUY_DIAMONDS:
				case Events::ItemChanged::R_BUY_GIFT_BOX:
				case Events::ItemChanged::R_BUY_GOLD_COINS:
				case Events::ItemChanged::R_BUY_LARGE_GIFT_BOX:
					PUT_NUMBER(param1)
					PUT_STRING(remarks)
					break;
				case Events::ItemChanged::R_AUCTION_TRANSFER_IN:
				case Events::ItemChanged::R_AUCTION_TRANSFER_OUT:
				case Events::ItemChanged::R_ENABLE_AUCTION_CENTER:
					break;
				case Events::ItemChanged::R_SHARED_RECHARGE_LOCK:
				case Events::ItemChanged::R_SHARED_RECHARGE_UNLOCK:
				case Events::ItemChanged::R_SHARED_RECHARGE_COMMIT:
				case Events::ItemChanged::R_SHARED_RECHARGE_REWARD:
				case Events::ItemChanged::R_SHARED_RECHARGE_ROLLBACK:
					PUT_ACCOUNT(param1)
					PUT_ACCOUNT(param2)
					break;
				default:
					LOG_EMPERY_PROMOTION_WARNING("Unknown reason: ", (unsigned)event->reason, ", param1 = ", event->param1,
						", param2 = ", event->param2, ", param3 = ", event->param3, ", remarks = ", event->remarks);
					break;
				}
				if(event->old_count < event->new_count){
					const auto obj = boost::make_shared<MySql::Promotion_IncomeBalanceHistory>(
						event->account_id.get(), utc_now, Poseidon::atomic_add(g_auto_id, 1, Poseidon::ATOMIC_RELAXED),
						event->new_count - event->old_count, event->reason, event->param1, event->param2, event->param3, oss.str());
					obj->async_save(true);
				} else if(event->old_count > event->new_count){
					const auto obj = boost::make_shared<MySql::Promotion_OutcomeBalanceHistory>(
						event->account_id.get(), utc_now, Poseidon::atomic_add(g_auto_id, 1, Poseidon::ATOMIC_RELAXED),
						event->old_count - event->new_count, event->reason, event->param1, event->param2, event->param3, oss.str());
					obj->async_save(true);
				}
			} else if(event->item_id == ItemIds::ID_GOLD_COINS){
				const auto obj = boost::make_shared<MySql::Promotion_GoldCoinHistory>(
					event->account_id.get(), utc_now, Poseidon::atomic_add(g_auto_id, 1, Poseidon::ATOMIC_RELAXED),
					event->old_count, event->new_count, event->reason, event->param1, event->param2, event->param3);
				obj->async_save(true);
			}
		});
	handles.push(listener);
}

}
