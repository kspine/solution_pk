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
			CAT_UNKNOWN               =  0, //
			CAT_UPGRADE_TURBO         =  1, // 建造加速，训练加速，研究加速
			CAT_SIGNING_IN            =  2, // 签到专用
			// = 3, // ???
			CAT_LEVEL_GIFT_BOX        =  4, // 充值礼包
			CAT_LAND_PURCHASE_TICKET  =  5, // 土地购买券专用
			CAT_LAND_UPGRADE_TICKET   =  6, // 土地升级券专用
			// = 7, // ???
			CAT_CURRENCY              =  8, // 黄金，钻石
			CAT_ACCELERATION_CARD     =  9, // 效率卡
			CAT_RESOURCE_BOX          = 10, // 资源木箱
			// = 11, // ???
			CAT_GIFT_BOX              = 12, // 加速礼包
			CAT_RESOURCE_TOKEN        = 13, // 资源代币
			CAT_LARGE_LEVEL_GIFT_BOX  = 14, // 大充值礼包
			CAT_SOLDIER_BOX           = 15, // 长枪兵，轻骑兵
			CAT_RELOCATION_CARD       = 16, // 迁城卡
			CAT_CASTLE_BUFF           = 17, // 城堡增益
			CAT_ITEM_BOX              = 18, // 道具木箱
			CAT_HORN                  = 19, // 小喇叭
			CAT_PERSONAL_DONATE_BOX   = 21, // 个人贡献箱
		};

	public:
		static boost::shared_ptr<const Item> get(ItemId item_id);
		static boost::shared_ptr<const Item> require(ItemId item_id);

		static boost::shared_ptr<const Item> get_by_type(Category category, std::uint32_t type);
		static boost::shared_ptr<const Item> require_by_type(Category category, std::uint32_t type);

		static void get_init(std::vector<boost::shared_ptr<const Item>> &ret);
		static void get_auto_inc(std::vector<boost::shared_ptr<const Item>> &ret);

		static void get_public(std::vector<boost::shared_ptr<const Item>> &ret);

	public:
		ItemId item_id;
		std::pair<Category, std::uint32_t> type;
		std::uint64_t value;

		TradeId use_as_trade_id;
		std::uint64_t init_count;

		AutoIncType auto_inc_type;
		std::uint64_t auto_inc_offset;
		std::int64_t auto_inc_step;
		std::uint64_t auto_inc_bound;

		bool is_public;
	};

	class ItemTrade {
	public:
		static boost::shared_ptr<const ItemTrade> get(TradeId trade_id);
		static boost::shared_ptr<const ItemTrade> require(TradeId trade_id);

	public:
		TradeId trade_id;
		boost::container::flat_map<ItemId, std::uint64_t> items_consumed;
		boost::container::flat_map<ItemId, std::uint64_t> items_produced;
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

	extern void unpack_item_trade(std::vector<ItemTransactionElement> &transaction,
		const boost::shared_ptr<const ItemTrade> &trade_data, std::uint64_t repeat_count,
		std::int64_t param1);
}

}

#endif
