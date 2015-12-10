#include "../precompiled.hpp"
#include "item.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(ItemMap, Data::Item,
		UNIQUE_MEMBER_INDEX(item_id)
		MULTI_MEMBER_INDEX(type)
		MULTI_MEMBER_INDEX(init_count)
		MULTI_MEMBER_INDEX(auto_inc_type)
		MULTI_MEMBER_INDEX(is_public)
	)
	boost::weak_ptr<const ItemMap> g_item_map;
	const char ITEM_FILE[] = "item";

	MULTI_INDEX_MAP(TradeMap, Data::ItemTrade,
		UNIQUE_MEMBER_INDEX(trade_id)
	)
	boost::weak_ptr<const TradeMap> g_trade_map;
	const char TRADE_FILE[] = "item_trading";

	MULTI_INDEX_MAP(RechargeMap, Data::ItemRecharge,
		UNIQUE_MEMBER_INDEX(recharge_id)
	)
	boost::weak_ptr<const RechargeMap> g_recharge_map;
	const char RECHARGE_FILE[] = "Pay";

	MULTI_INDEX_MAP(ShopMap, Data::ItemShop,
		UNIQUE_MEMBER_INDEX(shop_id)
	)
	boost::weak_ptr<const ShopMap> g_shop_map;
	const char SHOP_FILE[] = "shop";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(ITEM_FILE);
		const auto item_map = boost::make_shared<ItemMap>();
		while(csv.fetch_row()){
			Data::Item elem = { };

			csv.get(elem.item_id,    "itemid");

			unsigned category = Data::Item::CAT_UNKNOWN, type = 0;
			csv.get(category,        "class");
			csv.get(type,            "type");
			elem.type = std::make_pair(static_cast<Data::Item::Category>(category), type);
			csv.get(elem.value,      "value");

			csv.get(elem.init_count, "init_count");

			std::string str;
			csv.get(str,             "autoinc_type");
			if(::strcasecmp(str.c_str(), "none") == 0){
				elem.auto_inc_type = Data::Item::AIT_NONE;
			} else if(::strcasecmp(str.c_str(), "hourly") == 0){
				elem.auto_inc_type = Data::Item::AIT_HOURLY;
			} else if(::strcasecmp(str.c_str(), "daily") == 0){
				elem.auto_inc_type = Data::Item::AIT_DAILY;
			} else if(::strcasecmp(str.c_str(), "weekly") == 0){
				elem.auto_inc_type = Data::Item::AIT_WEEKLY;
			} else if(::strcasecmp(str.c_str(), "periodic") == 0){
				elem.auto_inc_type = Data::Item::AIT_PERIODIC;
			} else {
				LOG_EMPERY_CENTER_WARNING("Unknown item auto increment type: ", str);
				DEBUG_THROW(Exception, sslit("Unknown item auto increment type"));
			}
			csv.get(elem.auto_inc_offset, "autoinc_time");
			csv.get(elem.auto_inc_step,   "autoinc_step");
			csv.get(elem.auto_inc_bound,  "autoinc_bound");

			csv.get(elem.is_public,       "is_public");

			if(!item_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate item: item_id = ", elem.item_id);
				DEBUG_THROW(Exception, sslit("Duplicate item"));
			}
		}
		g_item_map = item_map;
		handles.push(item_map);
		auto servlet = DataSession::create_servlet(ITEM_FILE, Data::encode_csv_as_json(csv, "itemid"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TRADE_FILE);
		const auto trade_map = boost::make_shared<TradeMap>();
		while(csv.fetch_row()){
			Data::ItemTrade elem = { };

			csv.get(elem.trade_id, "trading_id");

			Poseidon::JsonObject object;
			csv.get(object, "consumption_item");
			elem.items_consumed.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
				if(!elem.items_consumed.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate item amount: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate item amount"));
				}
			}

			csv.get(object, "obtain_item");
			elem.items_produced.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
				if(!elem.items_produced.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate item amount: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate item amount"));
				}
			}

			if(!trade_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate trade element: trade_id = ", elem.trade_id);
				DEBUG_THROW(Exception, sslit("Duplicate trade element"));
			}
		}
		g_trade_map = trade_map;
		handles.push(trade_map);
		servlet = DataSession::create_servlet(TRADE_FILE, Data::encode_csv_as_json(csv, "trading_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(RECHARGE_FILE);
		const auto recharge_map = boost::make_shared<RechargeMap>();
		while(csv.fetch_row()){
			Data::ItemRecharge elem = { };

			csv.get(elem.recharge_id, "recharge_id");
			csv.get(elem.trade_id,    "trading_id");

			if(!recharge_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate recharge element: recharge_id = ", elem.recharge_id);
				DEBUG_THROW(Exception, sslit("Duplicate recharge element"));
			}
		}
		g_recharge_map = recharge_map;
		handles.push(recharge_map);
		servlet = DataSession::create_servlet(RECHARGE_FILE, Data::encode_csv_as_json(csv, "recharge_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(SHOP_FILE);
		const auto shop_map = boost::make_shared<ShopMap>();
		while(csv.fetch_row()){
			Data::ItemShop elem = { };

			csv.get(elem.shop_id,  "shop_id");
			csv.get(elem.trade_id, "trading_id");

			if(!shop_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate shop element: shop_id = ", elem.shop_id);
				DEBUG_THROW(Exception, sslit("Duplicate shop element"));
			}
		}
		g_shop_map = shop_map;
		handles.push(shop_map);
		servlet = DataSession::create_servlet(SHOP_FILE, Data::encode_csv_as_json(csv, "shop_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const Item> Item::get(ItemId item_id){
		PROFILE_ME;

		const auto item_map = g_item_map.lock();
		if(!item_map){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return { };
		}

		const auto it = item_map->find<0>(item_id);
		if(it == item_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("Item not found: item_id = ", item_id);
			return { };
		}
		return boost::shared_ptr<const Item>(item_map, &*it);
	}
	boost::shared_ptr<const Item> Item::require(ItemId item_id){
		PROFILE_ME;

		auto ret = get(item_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Item not found"));
		}
		return ret;
	}

	boost::shared_ptr<const Item> Item::get_by_type(Item::Category category, unsigned type){
		PROFILE_ME;

		const auto item_map = g_item_map.lock();
		if(!item_map){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return { };
		}

		const auto it = item_map->find<1>(std::make_pair(category, type));
		if(it == item_map->end<1>()){
			LOG_EMPERY_CENTER_DEBUG("Item not found: category = ", (unsigned)category, ", type = ", type);
			return { };
		}
		return boost::shared_ptr<const Item>(item_map, &*it);
	}
	boost::shared_ptr<const Item> Item::require_by_type(Item::Category category, unsigned type){
		PROFILE_ME;

		auto ret = get_by_type(category, type);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Item not found"));
		}
		return ret;
	}

	void Item::get_init(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto item_map = g_item_map.lock();
		if(!item_map){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return;
		}

		const auto begin = item_map->upper_bound<2>(0);
		const auto end = item_map->end<2>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(item_map, &*it);
		}
	}
	void Item::get_auto_inc(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto item_map = g_item_map.lock();
		if(!item_map){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return;
		}

		const auto begin = item_map->upper_bound<3>(AIT_NONE);
		const auto end = item_map->end<3>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(item_map, &*it);
		}
	}
	void Item::get_public(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto item_map = g_item_map.lock();
		if(!item_map){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return;
		}

		const auto begin = item_map->upper_bound<4>(false);
		const auto end = item_map->end<4>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(item_map, &*it);
		}
	}

	boost::shared_ptr<const ItemTrade> ItemTrade::get(TradeId trade_id){
		PROFILE_ME;

		const auto trade_map = g_trade_map.lock();
		if(!trade_map){
			LOG_EMPERY_CENTER_WARNING("TradeMap has not been loaded.");
			return { };
		}

		const auto it = trade_map->find<0>(trade_id);
		if(it == trade_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("ItemTrade not found: trade_id = ", trade_id);
			return { };
		}
		return boost::shared_ptr<const ItemTrade>(trade_map, &*it);
	}
	boost::shared_ptr<const ItemTrade> ItemTrade::require(TradeId trade_id){
		PROFILE_ME;

		auto ret = get(trade_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("ItemTrade not found"));
		}
		return ret;
	}

	boost::shared_ptr<const ItemRecharge> ItemRecharge::get(RechargeId recharge_id){
		PROFILE_ME;

		const auto recharge_map = g_recharge_map.lock();
		if(!recharge_map){
			LOG_EMPERY_CENTER_WARNING("RechargeMap has not been loaded.");
			return { };
		}

		const auto it = recharge_map->find<0>(recharge_id);
		if(it == recharge_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("ItemRecharge not found: recharge_id = ", recharge_id);
			return { };
		}
		return boost::shared_ptr<const ItemRecharge>(recharge_map, &*it);
	}
	boost::shared_ptr<const ItemRecharge> ItemRecharge::require(RechargeId recharge_id){
		PROFILE_ME;

		auto ret = get(recharge_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("ItemRecharge not found"));
		}
		return ret;
	}

	boost::shared_ptr<const ItemShop> ItemShop::get(ShopId shop_id){
		PROFILE_ME;

		const auto shop_map = g_shop_map.lock();
		if(!shop_map){
			LOG_EMPERY_CENTER_WARNING("ShopMap has not been loaded.");
			return { };
		}

		const auto it = shop_map->find<0>(shop_id);
		if(it == shop_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("ItemShop not found: shop_id = ", shop_id);
			return { };
		}
		return boost::shared_ptr<const ItemShop>(shop_map, &*it);
	}
	boost::shared_ptr<const ItemShop> ItemShop::require(ShopId shop_id){
		PROFILE_ME;

		auto ret = get(shop_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("ItemShop not found"));
		}
		return ret;
	}

	void unpack_item_trade(std::vector<ItemTransactionElement> &transaction,
		const boost::shared_ptr<const ItemTrade> &trade_data, boost::uint64_t repeat_count,
		boost::int64_t param1)
	{
		PROFILE_ME;

		transaction.reserve(transaction.size() + trade_data->items_consumed.size() + trade_data->items_produced.size());
		for(auto it = trade_data->items_consumed.begin(); it != trade_data->items_consumed.end(); ++it){
			transaction.emplace_back(ItemTransactionElement::OP_REMOVE, it->first, checked_mul(it->second, repeat_count),
				ReasonIds::ID_TRADE_REQUEST, param1, trade_data->trade_id.get(), repeat_count);
		}
		for(auto it = trade_data->items_produced.begin(); it != trade_data->items_produced.end(); ++it){
			transaction.emplace_back(ItemTransactionElement::OP_ADD, it->first, checked_mul(it->second, repeat_count),
				ReasonIds::ID_TRADE_REQUEST, param1, trade_data->trade_id.get(), repeat_count);
		}
	}

}

}
