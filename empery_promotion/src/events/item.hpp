#ifndef EMPERY_PROMOTION_EVENTS_ITEM_HPP_
#define EMPERY_PROMOTION_EVENTS_ITEM_HPP_

#include <cstdint>
#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyPromotion {

namespace Events {
	struct ItemChanged : public Poseidon::EventBase<330201> {
		enum Reason {                             // param1          param2          param3
			R_ADMIN_OPERATION           = 201001, // 自定义          自定义          自定义
			R_TRANSFER                  = 201002, // 源账号          目标账号        0
			R_UPGRADE_ACCOUNT           = 201003, // 升级账号        付款人账号      升级到等级
			R_CREATE_ACCOUNT            = 201004, // 新账号          付款人账号      升级到等级
			R_BALANCE_BONUS             = 201005, // 升级账号        付款人账号      升级到等级
			R_RECHARGE                  = 201006, // 充值账号        0               0
			R_WITHDRAW                  = 201007, // 提款账号        0               0
			R_COMMIT_WITHDRAWAL         = 201008, // 提款账号        0               0
			R_INCOME_TAX                = 201009, // 原始收入        收取谁的        0
			R_BALANCE_BONUS_EXTRA       = 201010, // 升级账号        付款人账号      升级到等级
			R_DEACTIVATE_ACCOUNT        = 201011, // 删除账号        0               0
			R_BUY_MORE_CARDS            = 201012, // 自己账号        付款时卡单价    0
			R_GOLD_SCRAMBLE_BID         = 201013, // 起始时间        奖池内金币      奖池内余额
			R_GOLD_SCRAMBLE_REWARD      = 201014, // 起始时间        奖池内金币      奖池内余额
			R_ROLLBACK_WITHDRAWAL       = 201015, // 提款账号        0               0
			R_SELL_CARDS                = 201016, // 买卡账号        0               0
			R_BUY_DIAMONDS              = 201017, // 买钻石数量      0               0
			R_BUY_GIFT_BOX              = 201018, // 买礼包等级      0               0
			R_BUY_GOLD_COINS            = 201019, // 买金币数量      0               0
			R_AUCTION_TRANSFER_IN       = 201020, // 0               0               0
			R_AUCTION_TRANSFER_OUT      = 201021, // 0               0               0
			R_ENABLE_AUCTION_CENTER     = 201022, // 0               0               0
			R_BUY_LARGE_GIFT_BOX        = 201023, // 买礼包等级      0               0
			R_SHARED_RECHARGE_LOCK      = 201024, // 发起人账号      共享人账号      0
			R_SHARED_RECHARGE_UNLOCK    = 201025, // 发起人账号      共享人账号      0
			R_SHARED_RECHARGE_COMMIT    = 201026, // 发起人账号      共享人账号      0
			R_SHARED_RECHARGE_REWARD    = 201027, // 发起人账号      共享人账号      0
			R_SHARED_RECHARGE_ROLLBACK  = 201028, // 发起人账号      共享人账号      0
			R_SELL_CARDS_ALT            = 201029, // 买卡账号        0               0
			R_SYNUSER_PAYMENT           = 201030, // 0               0               0
			R_SELL_CARDS_MOJINPAI       = 201031, // 买卡账号        0               0
		};

		AccountId account_id;
		ItemId item_id;
		std::uint64_t old_count;
		std::uint64_t new_count;

		Reason reason;
		std::uint64_t param1;
		std::uint64_t param2;
		std::uint64_t param3;
		std::string remarks;

		ItemChanged(AccountId account_id_, ItemId item_id_, std::uint64_t old_count_, std::uint64_t new_count_,
			Reason reason_, std::uint64_t param1_, std::uint64_t param2_, std::uint64_t param3_, std::string remarks_)
			: account_id(account_id_), item_id(item_id_), old_count(old_count_), new_count(new_count_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_), remarks(std::move(remarks_))
		{
		}
	};
}

}

#endif
