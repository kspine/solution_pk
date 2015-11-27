#ifndef EMPERY_CENTER_DATA_ITEM_HPP_
#define EMPERY_CENTER_DATA_ITEM_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <vector>
#include "../transaction_element_fwd.hpp"

namespace EmperyCenter {

namespace Data {
	class Item {
	public:
		enum AutoIncType {
			AIT_NONE     = 0,
			AIT_HOURLY   = 1,
			AIT_DAILY    = 2,
			AIT_WEEKLY   = 3,
			AIT_PERIODIC = 4,
		};

		enum Category {
			CAT_UNKNOWN               = 0, //
			CAT_UPGRADE_TURBO         = 1, // 建造加速，训练加速，研究加速
			CAT_SIGNING_IN            = 2, // 签到专用
			CAT_CURRENCY              = 3, // 黄金，钻石，铁矿，赤铜
			CAT_GIFT_BOX              = 4, // 礼包
			CAT_LAND_PURCHASE_TICKET  = 5, // 土地购买券专用
			CAT_LAND_UPGRADE_TICKET   = 6, // 土地升级券专用
		};

	public:
		static boost::shared_ptr<const Item> get(ItemId item_id);
		static boost::shared_ptr<const Item> require(ItemId item_id);

		static boost::shared_ptr<const Item> get_by_type(Category category, unsigned type);
		static boost::shared_ptr<const Item> require_by_type(Category category, unsigned type);

		static void get_init(std::vector<boost::shared_ptr<const Item>> &ret);
		static void get_auto_inc(std::vector<boost::shared_ptr<const Item>> &ret);
		static void get_public(std::vector<boost::shared_ptr<const Item>> &ret);

	public:
		ItemId item_id;
		std::pair<Category, unsigned> type;
		boost::uint64_t value;

		boost::uint64_t init_count;

		AutoIncType auto_inc_type;
		boost::uint64_t auto_inc_offset;
		boost::int64_t auto_inc_step;
		boost::uint64_t auto_inc_bound;

		bool is_public;
	};

	class ItemTrade {
	public:
		static boost::shared_ptr<const ItemTrade> get(TradeId trade_id);
		static boost::shared_ptr<const ItemTrade> require(TradeId trade_id);

		static void unpack(std::vector<ItemTransactionElement> &transaction,
			const boost::shared_ptr<const ItemTrade> &trade_data, boost::uint64_t repeat_count,
			boost::int64_t param1);

	public:
		TradeId trade_id;
		boost::container::flat_map<ItemId, boost::uint64_t> items_consumed;
		boost::container::flat_map<ItemId, boost::uint64_t> items_produced;
	};

	class ItemRecharge {
	public:
		static boost::shared_ptr<const ItemRecharge> get(RechargeId recharge_id);
		static boost::shared_ptr<const ItemRecharge> require(RechargeId recharge_id);

	public:
		RechargeId recharge_id;
		TradeId trade_id;
	};

	class ItemShop {
	public:
		static boost::shared_ptr<const ItemShop> get(ShopId shop_id);
		static boost::shared_ptr<const ItemShop> require(ShopId shop_id);

	public:
		ShopId shop_id;
		TradeId trade_id;
	};
}

}

#endif
