#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/global_status.hpp"
#include "../item_ids.hpp"
#include "../msg/err_account.hpp"
#include <poseidon/mysql/object_base.hpp>

namespace EmperyPromotion {

namespace MySql {
	namespace {

#define MYSQL_OBJECT_NAME	EconomySummary
#define MYSQL_OBJECT_FIELDS	\
	FIELD_BIGINT_UNSIGNED	(itemId)	\
	FIELD_BIGINT_UNSIGNED	(count)
#include <poseidon/mysql/object_generator.hpp>

	}
}

ACCOUNT_SERVLET("getEconomySummary", /* session */, /* params */){
	Poseidon::JsonObject ret;

	std::vector<boost::shared_ptr<MySql::EconomySummary>> objs;
	MySql::EconomySummary::batchLoad(objs, "SELECT `itemId`, SUM(`count`) AS `count` FROM `Promotion_Item` GROUP BY `itemId`");

	const auto getTotalItemCount = [&](ItemId itemId) -> boost::uint64_t {
		for(auto it = objs.begin(); it != objs.end(); ++it){
			const auto &obj = *it;
			if(obj->get_itemId() == itemId.get()){
				return obj->get_count();
			}
		}
		return 0;
	};

	ret[sslit("balanceRechargedHistorical")] = getTotalItemCount(ItemIds::ID_BALANCE_RECHARGED_HISTORICAL);
	ret[sslit("balanceWithdrawnHistorical")] = getTotalItemCount(ItemIds::ID_BALANCE_WITHDRAWN_HISTORICAL);
	ret[sslit("withdrawnBalance")]           = getTotalItemCount(ItemIds::ID_WITHDRAWN_BALANCE);

	ret[sslit("accCardUnitPrice")]           = GlobalStatus::get(GlobalStatus::SLOT_ACC_CARD_UNIT_PRICE);
	ret[sslit("accelerationCards")]          = getTotalItemCount(ItemIds::ID_ACCELERATION_CARDS);

	ret[sslit("goldCoins")]                  = getTotalItemCount(ItemIds::ID_GOLD_COINS);

	ret[sslit("errorCode")] = (int)Msg::ST_OK;
	ret[sslit("errorMessage")] = "No error";
	return ret;
}

}
