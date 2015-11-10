#ifndef EMPERY_PROMOTION_EVENTS_ITEM_HPP_
#define EMPERY_PROMOTION_EVENTS_ITEM_HPP_

#include <boost/cstdint.hpp>
#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyPromotion {

namespace Events {
	struct ItemChanged : public Poseidon::EventBase<330201> {
		enum Reason {                         // param1         param2          param3
			R_ADMIN_OPERATION       = 201001, // 自定义         自定义          自定义
			R_TRANSFER              = 201002, // 源账号         目标账号        0
			R_UPGRADE_ACCOUNT       = 201003, // 升级账号       付款人账号      升级到等级
			R_CREATE_ACCOUNT        = 201004, // 新账号         付款人账号      升级到等级
			R_BALANCE_BONUS         = 201005, // 升级账号       付款人账号      升级到等级
			R_RECHARGE              = 201006, // 充值账号       0               0
			R_WITHDRAW              = 201007, // 提款账号       0               0
			R_COMMIT_WITHDRAWAL     = 201008, // 提款账号       0               0
			R_INCOME_TAX            = 201009, // 原始收入       收取谁的        0
			R_BALANCE_BONUS_EXTRA   = 201010, // 升级账号       付款人账号      升级到等级
			R_DEACTIVATE_ACCOUNT    = 201011, // 删除账号       0               0
			R_BUY_MORE_CARDS        = 201012, // 自己账号       付款时卡单价    0
			R_GOLD_SCRAMBLE_BID     = 201013, // 起始时间       奖池内金币      奖池内余额
			R_GOLD_SCRAMBLE_REWARD  = 201014, // 起始时间       奖池内金币      奖池内余额
		};

		AccountId accountId;
		ItemId itemId;
		boost::uint64_t oldCount;
		boost::uint64_t newCount;

		Reason reason;
		boost::uint64_t param1;
		boost::uint64_t param2;
		boost::uint64_t param3;
		std::string remarks;

		ItemChanged(AccountId accountId_, ItemId itemId_, boost::uint64_t oldCount_, boost::uint64_t newCount_,
			Reason reason_, boost::uint64_t param1_, boost::uint64_t param2_, boost::uint64_t param3_, std::string remarks_)
			: accountId(accountId_), itemId(itemId_), oldCount(oldCount_), newCount(newCount_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_), remarks(std::move(remarks_))
		{
		}
	};
}

}

#endif
