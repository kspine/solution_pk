#include "../precompiled.hpp"
#include "item.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include "formats.hpp"
#include "../data_session.hpp"
#include "../checked_arithmetic.hpp"
#include "../item_transaction_element.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(ItemMap, Data::Item,
		UNIQUE_MEMBER_INDEX(item_id)
		MULTI_MEMBER_INDEX(init_count)
		MULTI_MEMBER_INDEX(auto_inc_type)
	)
	boost::weak_ptr<const ItemMap> g_item_map;
	const char ITEM_FILE[] = "item";

	MULTI_INDEX_MAP(TradeMap, Data::Trade,
		UNIQUE_MEMBER_INDEX(trade_id)
	)
	boost::weak_ptr<const TradeMap> g_trade_map;
	const char TRADE_FILE[] = "item_trading";

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto data_directory = get_config<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;

		const auto item_map = boost::make_shared<ItemMap>();
		auto path = data_directory + "/" + ITEM_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading items: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::Item elem = { };

			csv.get(elem.item_id,       "itemid");
			csv.get(elem.quality,      "quality");
			csv.get(elem.type.first,   "class");
			csv.get(elem.type.second,  "type");
			csv.get(elem.value,        "value");

			csv.get(elem.init_count,    "init_count");

			std::string str;
			csv.get(str,               "autoinc_type");
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
			boost::uint64_t minutes;
			csv.get(minutes,           "autoinc_time");
			elem.auto_inc_offset = checked_mul<boost::uint64_t>(minutes, 60000);
			csv.get(elem.auto_inc_step,  "autoinc_step");
			csv.get(elem.auto_inc_bound, "autoinc_bound");

			if(!item_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate item: item_id = ", elem.item_id);
				DEBUG_THROW(Exception, sslit("Duplicate item"));
			}
		}
		g_item_map = item_map;
		handles.push(DataSession::create_servlet(ITEM_FILE, serialize_csv(csv, "itemid")));
		handles.push(item_map);

		const auto trade_map = boost::make_shared<TradeMap>();
		path = data_directory + "/" + TRADE_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading trade items: path = ", path);
		csv.load(path.c_str());
		while(csv.fetch_row()){
			Data::Trade elem = { };

			csv.get(elem.trade_id, "trading_id");

			std::string str;
			csv.get(str, "consumption_item", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_object(iss);
				elem.items_consumed.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto item_id = boost::lexical_cast<ItemId>(it->first);
					const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
					if(!elem.items_consumed.emplace(item_id, count).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate item amount: item_id = ", item_id);
						DEBUG_THROW(Exception, sslit("Duplicate item amount"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", trade_id = ", elem.trade_id, ", str = ", str);
				throw;
			}

			csv.get(str, "obtain_item", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parse_object(iss);
				elem.items_produced.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto item_id = boost::lexical_cast<ItemId>(it->first);
					const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
					if(!elem.items_produced.emplace(item_id, count).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate item amount: item_id = ", item_id);
						DEBUG_THROW(Exception, sslit("Duplicate item amount"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", trade_id = ", elem.trade_id, ", str = ", str);
				throw;
			}

			if(!trade_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate trade item: trade_id = ", elem.trade_id);
				DEBUG_THROW(Exception, sslit("Duplicate trade item"));
			}
		}
		g_trade_map = trade_map;
		handles.push(DataSession::create_servlet(TRADE_FILE, serialize_csv(csv, "trading_id")));
		handles.push(trade_map);
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

	void Item::get_init(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto item_map = g_item_map.lock();
		if(!item_map){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return;
		}

		const auto begin = item_map->upper_bound<1>(0);
		const auto end = item_map->end<1>();
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

		const auto begin = item_map->upper_bound<2>(AIT_NONE);
		const auto end = item_map->end<2>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(item_map, &*it);
		}
	}

	boost::shared_ptr<const Trade> Trade::get(TradeId trade_id){
		PROFILE_ME;

		const auto trade_map = g_trade_map.lock();
		if(!trade_map){
			LOG_EMPERY_CENTER_WARNING("TradeMap has not been loaded.");
			return { };
		}

		const auto it = trade_map->find<0>(trade_id);
		if(it == trade_map->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("Trade not found: trade_id = ", trade_id);
			return { };
		}
		return boost::shared_ptr<const Trade>(trade_map, &*it);
	}
	boost::shared_ptr<const Trade> Trade::require(TradeId trade_id){
		PROFILE_ME;

		auto ret = get(trade_id);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Trade not found"));
		}
		return ret;
	}

	void Trade::unpack(std::vector<ItemTransactionElement> &transaction,
		const boost::shared_ptr<const Trade> &trade_data, boost::uint64_t repeat_times,
		ReasonId reason, boost::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3)
	{
		PROFILE_ME;

		transaction.reserve(transaction.size() + trade_data->items_consumed.size() + trade_data->items_produced.size());
		for(auto it = trade_data->items_consumed.begin(); it != trade_data->items_consumed.end(); ++it){
			transaction.emplace_back(ItemTransactionElement::OP_REMOVE, it->first, checked_mul(it->second, repeat_times),
				reason, param1, param2, param3);
		}
		for(auto it = trade_data->items_produced.begin(); it != trade_data->items_produced.end(); ++it){
			transaction.emplace_back(ItemTransactionElement::OP_ADD, it->first, checked_mul(it->second, repeat_times),
				reason, param1, param2, param3);
		}
	}
}

}
