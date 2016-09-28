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
	MULTI_INDEX_MAP(ItemContainer, Data::Item,
		UNIQUE_MEMBER_INDEX(item_id)
		MULTI_MEMBER_INDEX(type)
		MULTI_MEMBER_INDEX(init_count)
		MULTI_MEMBER_INDEX(auto_inc_type)
		MULTI_MEMBER_INDEX(is_public)
	)
	boost::weak_ptr<const ItemContainer> g_item_container;
	const char ITEM_FILE[] = "item";

	MULTI_INDEX_MAP(TradeContainer, Data::ItemTrade,
		UNIQUE_MEMBER_INDEX(trade_id)
	)
	boost::weak_ptr<const TradeContainer> g_trade_container;
	const char TRADE_FILE[] = "item_trading";
	
	MULTI_INDEX_MAP(ResourceTradeContainer, Data::ResourceTrade,
		UNIQUE_MEMBER_INDEX(trade_id)
	)
	boost::weak_ptr<const ResourceTradeContainer> g_resouce_trade_container;
	const char RESOUCE_TRADE_FILE[] = "item_material";

	MULTI_INDEX_MAP(RechargeContainer, Data::ItemRecharge,
		UNIQUE_MEMBER_INDEX(recharge_id)
	)
	boost::weak_ptr<const RechargeContainer> g_recharge_container;
	const char RECHARGE_FILE[] = "Pay";

	MULTI_INDEX_MAP(ShopContainer, Data::ItemShop,
		UNIQUE_MEMBER_INDEX(shop_id)
	)
	boost::weak_ptr<const ShopContainer> g_shop_container;
	const char SHOP_FILE[] = "shop";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(ITEM_FILE);
		const auto item_container = boost::make_shared<ItemContainer>();
		while(csv.fetch_row()){
			Data::Item elem = { };

			csv.get(elem.item_id, "itemid");

			unsigned category = elem.CAT_UNKNOWN;
			std::uint32_t type = 0;
			csv.get(category,     "class");
			csv.get(type,         "type");
			elem.type = std::make_pair(static_cast<Data::Item::Category>(category), type);
			csv.get(elem.value,   "value");

			csv.get(elem.use_as_trade_id, "canuse");
			csv.get(elem.init_count,      "init_count");

			std::string str;
			csv.get(str, "autoinc_type");
			if(::strcasecmp(str.c_str(), "none") == 0){
				elem.auto_inc_type = elem.AIT_NONE;
			} else if(::strcasecmp(str.c_str(), "hourly") == 0){
				elem.auto_inc_type = elem.AIT_HOURLY;
			} else if(::strcasecmp(str.c_str(), "daily") == 0){
				elem.auto_inc_type = elem.AIT_DAILY;
			} else if(::strcasecmp(str.c_str(), "weekly") == 0){
				elem.auto_inc_type = elem.AIT_WEEKLY;
			} else if(::strcasecmp(str.c_str(), "periodic") == 0){
				elem.auto_inc_type = elem.AIT_PERIODIC;
			} else {
				LOG_EMPERY_CENTER_WARNING("Unknown item auto increment type: ", str);
				DEBUG_THROW(Exception, sslit("Unknown item auto increment type"));
			}
			csv.get(elem.auto_inc_offset, "autoinc_time");
			csv.get(elem.auto_inc_step,   "autoinc_step");
			csv.get(elem.auto_inc_bound,  "autoinc_bound");

			csv.get(elem.is_public,       "is_public");

			if(!item_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate item: item_id = ", elem.item_id);
				DEBUG_THROW(Exception, sslit("Duplicate item"));
			}
		}
		g_item_container = item_container;
		handles.push(item_container);
		auto servlet = DataSession::create_servlet(ITEM_FILE, Data::encode_csv_as_json(csv, "itemid"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TRADE_FILE);
		const auto trade_container = boost::make_shared<TradeContainer>();
		while(csv.fetch_row()){
			Data::ItemTrade elem = { };

			csv.get(elem.trade_id, "trading_id");

			Poseidon::JsonObject object;
			csv.get(object, "consumption_item");
			elem.items_consumed.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.items_consumed.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate item amount: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate item amount"));
				}
			}

			object.clear();
			csv.get(object, "obtain_item");
			elem.items_produced.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.items_produced.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate item amount: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate item amount"));
				}
			}

			if(!trade_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate trade element: trade_id = ", elem.trade_id);
				DEBUG_THROW(Exception, sslit("Duplicate trade element"));
			}
		}
		g_trade_container = trade_container;
		handles.push(trade_container);
		servlet = DataSession::create_servlet(TRADE_FILE, Data::encode_csv_as_json(csv, "trading_id"));
		handles.push(std::move(servlet));
		
		csv = Data::sync_load_data(RESOUCE_TRADE_FILE);
		const auto resource_trade_container = boost::make_shared<ResourceTradeContainer>();
		while(csv.fetch_row()){
			Data::ResourceTrade elem = { };

			csv.get(elem.trade_id, "trading_id");

			Poseidon::JsonObject object;
			csv.get(object, "obtain_item");
			elem.resource_produced.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.resource_produced.emplace(resource_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate resource amount: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate resource amount"));
				}
			}

			if(!resource_trade_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate trade element: trade_id = ", elem.trade_id);
				DEBUG_THROW(Exception, sslit("Duplicate trade element"));
			}
		}
		g_resouce_trade_container = resource_trade_container;
		handles.push(resource_trade_container);
		servlet = DataSession::create_servlet(RESOUCE_TRADE_FILE, Data::encode_csv_as_json(csv, "trading_id"));
		handles.push(std::move(servlet));
		
		csv = Data::sync_load_data(RECHARGE_FILE);
		const auto recharge_container = boost::make_shared<RechargeContainer>();
		while(csv.fetch_row()){
			Data::ItemRecharge elem = { };

			csv.get(elem.recharge_id, "recharge_id");
			csv.get(elem.trade_id,    "trading_id");

			if(!recharge_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate recharge element: recharge_id = ", elem.recharge_id);
				DEBUG_THROW(Exception, sslit("Duplicate recharge element"));
			}
		}
		g_recharge_container = recharge_container;
		handles.push(recharge_container);
		servlet = DataSession::create_servlet(RECHARGE_FILE, Data::encode_csv_as_json(csv, "recharge_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(SHOP_FILE);
		const auto shop_container = boost::make_shared<ShopContainer>();
		while(csv.fetch_row()){
			Data::ItemShop elem = { };

			csv.get(elem.shop_id,  "shop_id");
			csv.get(elem.trade_id, "trading_id");

			if(!shop_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate shop element: shop_id = ", elem.shop_id);
				DEBUG_THROW(Exception, sslit("Duplicate shop element"));
			}
		}
		g_shop_container = shop_container;
		handles.push(shop_container);
		servlet = DataSession::create_servlet(SHOP_FILE, Data::encode_csv_as_json(csv, "shop_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const Item> Item::get(ItemId item_id){
		PROFILE_ME;

		const auto item_container = g_item_container.lock();
		if(!item_container){
			LOG_EMPERY_CENTER_WARNING("ItemContainer has not been loaded.");
			return { };
		}

		const auto it = item_container->find<0>(item_id);
		if(it == item_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("Item not found: item_id = ", item_id);
			return { };
		}
		return boost::shared_ptr<const Item>(item_container, &*it);
	}
	boost::shared_ptr<const Item> Item::require(ItemId item_id){
		PROFILE_ME;

		auto ret = get(item_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("Item not found: item_id = ", item_id);
			DEBUG_THROW(Exception, sslit("Item not found"));
		}
		return ret;
	}

	boost::shared_ptr<const Item> Item::get_by_type(Item::Category category, std::uint32_t type){
		PROFILE_ME;

		const auto item_container = g_item_container.lock();
		if(!item_container){
			LOG_EMPERY_CENTER_WARNING("ItemContainer has not been loaded.");
			return { };
		}

		const auto it = item_container->find<1>(std::make_pair(category, type));
		if(it == item_container->end<1>()){
			LOG_EMPERY_CENTER_TRACE("Item not found: category = ", (unsigned)category, ", type = ", type);
			return { };
		}
		return boost::shared_ptr<const Item>(item_container, &*it);
	}
	boost::shared_ptr<const Item> Item::require_by_type(Item::Category category, std::uint32_t type){
		PROFILE_ME;

		auto ret = get_by_type(category, type);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("Item not found: category = ", (unsigned)category, ", type = ", type);
			DEBUG_THROW(Exception, sslit("Item not found"));
		}
		return ret;
	}

	void Item::get_init(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto item_container = g_item_container.lock();
		if(!item_container){
			LOG_EMPERY_CENTER_WARNING("ItemContainer has not been loaded.");
			return;
		}

		const auto begin = item_container->upper_bound<2>(0);
		const auto end = item_container->end<2>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(item_container, &*it);
		}
	}
	void Item::get_auto_inc(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto item_container = g_item_container.lock();
		if(!item_container){
			LOG_EMPERY_CENTER_WARNING("ItemContainer has not been loaded.");
			return;
		}

		const auto begin = item_container->upper_bound<3>(AIT_NONE);
		const auto end = item_container->end<3>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(item_container, &*it);
		}
	}

	void Item::get_public(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto item_container = g_item_container.lock();
		if(!item_container){
			LOG_EMPERY_CENTER_WARNING("ItemContainer has not been loaded.");
			return;
		}

		const auto begin = item_container->upper_bound<4>(false);
		const auto end = item_container->end<4>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(item_container, &*it);
		}
	}

	boost::shared_ptr<const ItemTrade> ItemTrade::get(TradeId trade_id){
		PROFILE_ME;

		const auto trade_container = g_trade_container.lock();
		if(!trade_container){
			LOG_EMPERY_CENTER_WARNING("TradeContainer has not been loaded.");
			return { };
		}

		const auto it = trade_container->find<0>(trade_id);
		if(it == trade_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("ItemTrade not found: trade_id = ", trade_id);
			return { };
		}
		return boost::shared_ptr<const ItemTrade>(trade_container, &*it);
	}
	boost::shared_ptr<const ItemTrade> ItemTrade::require(TradeId trade_id){
		PROFILE_ME;

		auto ret = get(trade_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("ItemTrade not found: trade_id = ", trade_id);
			DEBUG_THROW(Exception, sslit("ItemTrade not found"));
		}
		return ret;
	}
	
	boost::shared_ptr<const ResourceTrade> ResourceTrade::get(TradeId trade_id){
		PROFILE_ME;

		const auto resource_trade_container = g_resouce_trade_container.lock();
		if(!resource_trade_container){
			LOG_EMPERY_CENTER_WARNING("TradeContainer has not been loaded.");
			return { };
		}

		const auto it = resource_trade_container->find<0>(trade_id);
		if(it == resource_trade_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("ResourceTrade not found: trade_id = ", trade_id);
			return { };
		}
		return boost::shared_ptr<const ResourceTrade>(resource_trade_container, &*it);
	}
	boost::shared_ptr<const ResourceTrade> ResourceTrade::require(TradeId trade_id){
		PROFILE_ME;

		auto ret = get(trade_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("ResourceTrade not found: trade_id = ", trade_id);
			DEBUG_THROW(Exception, sslit("ResourceTrade not found"));
		}
		return ret;
	}
	

	boost::shared_ptr<const ItemRecharge> ItemRecharge::get(RechargeId recharge_id){
		PROFILE_ME;

		const auto recharge_container = g_recharge_container.lock();
		if(!recharge_container){
			LOG_EMPERY_CENTER_WARNING("RechargeContainer has not been loaded.");
			return { };
		}

		const auto it = recharge_container->find<0>(recharge_id);
		if(it == recharge_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("ItemRecharge not found: recharge_id = ", recharge_id);
			return { };
		}
		return boost::shared_ptr<const ItemRecharge>(recharge_container, &*it);
	}
	boost::shared_ptr<const ItemRecharge> ItemRecharge::require(RechargeId recharge_id){
		PROFILE_ME;

		auto ret = get(recharge_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("ItemRecharge not found: recharge_id = ", recharge_id);
			DEBUG_THROW(Exception, sslit("ItemRecharge not found"));
		}
		return ret;
	}

	boost::shared_ptr<const ItemShop> ItemShop::get(ShopId shop_id){
		PROFILE_ME;

		const auto shop_container = g_shop_container.lock();
		if(!shop_container){
			LOG_EMPERY_CENTER_WARNING("ShopContainer has not been loaded.");
			return { };
		}

		const auto it = shop_container->find<0>(shop_id);
		if(it == shop_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("ItemShop not found: shop_id = ", shop_id);
			return { };
		}
		return boost::shared_ptr<const ItemShop>(shop_container, &*it);
	}
	boost::shared_ptr<const ItemShop> ItemShop::require(ShopId shop_id){
		PROFILE_ME;

		auto ret = get(shop_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("ItemShop not found: shop_id = ", shop_id);
			DEBUG_THROW(Exception, sslit("ItemShop not found"));
		}
		return ret;
	}

	void unpack_item_trade(std::vector<ItemTransactionElement> &transaction,
		const boost::shared_ptr<const ItemTrade> &trade_data, std::uint64_t repeat_count,
		std::int64_t param1)
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
