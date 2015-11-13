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
		UNIQUE_MEMBER_INDEX(itemId)
		MULTI_MEMBER_INDEX(initCount)
		MULTI_MEMBER_INDEX(autoIncType)
	)
	boost::weak_ptr<const ItemMap> g_itemMap;
	const char ITEM_FILE[] = "item";

	MULTI_INDEX_MAP(TradeMap, Data::Trade,
		UNIQUE_MEMBER_INDEX(tradeId)
	)
	boost::weak_ptr<const TradeMap> g_tradeMap;
	const char TRADE_FILE[] = "item_trading";

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto dataDirectory = getConfig<std::string>("data_directory", "empery_center_data");

		Poseidon::CsvParser csv;

		const auto itemMap = boost::make_shared<ItemMap>();
		auto path = dataDirectory + "/" + ITEM_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading items: path = ", path);
		csv.load(path.c_str());
		while(csv.fetchRow()){
			Data::Item elem = { };

			csv.get(elem.itemId,       "itemid");
			csv.get(elem.quality,      "quality");
			csv.get(elem.type.first,   "class");
			csv.get(elem.type.second,  "type");
			csv.get(elem.value,        "value");

			csv.get(elem.initCount,    "init_count");

			std::string str;
			csv.get(str,               "autoinc_type");
			if(::strcasecmp(str.c_str(), "none") == 0){
				elem.autoIncType = Data::Item::AIT_NONE;
			} else if(::strcasecmp(str.c_str(), "hourly") == 0){
				elem.autoIncType = Data::Item::AIT_HOURLY;
			} else if(::strcasecmp(str.c_str(), "daily") == 0){
				elem.autoIncType = Data::Item::AIT_DAILY;
			} else if(::strcasecmp(str.c_str(), "weekly") == 0){
				elem.autoIncType = Data::Item::AIT_WEEKLY;
			} else if(::strcasecmp(str.c_str(), "periodic") == 0){
				elem.autoIncType = Data::Item::AIT_PERIODIC;
			} else {
				LOG_EMPERY_CENTER_WARNING("Unknown item auto increment type: ", str);
				DEBUG_THROW(Exception, sslit("Unknown item auto increment type"));
			}
			boost::uint64_t minutes;
			csv.get(minutes,           "autoinc_time");
			elem.autoIncOffset = checkedMul<boost::uint64_t>(minutes, 60000);
			csv.get(elem.autoIncStep,  "autoinc_step");
			csv.get(elem.autoIncBound, "autoinc_bound");

			if(!itemMap->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate item: itemId = ", elem.itemId);
				DEBUG_THROW(Exception, sslit("Duplicate item"));
			}
		}
		g_itemMap = itemMap;
		handles.push(DataSession::createServlet(ITEM_FILE, serializeCsv(csv, "itemid")));
		handles.push(itemMap);

		const auto tradeMap = boost::make_shared<TradeMap>();
		path = dataDirectory + "/" + TRADE_FILE + ".csv";
		LOG_EMPERY_CENTER_INFO("Loading trade items: path = ", path);
		csv.load(path.c_str());
		while(csv.fetchRow()){
			Data::Trade elem = { };

			csv.get(elem.tradeId, "trading_id");

			std::string str;
			csv.get(str, "consumption_item", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parseObject(iss);
				elem.itemsConsumed.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto itemId = boost::lexical_cast<ItemId>(it->first);
					const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
					if(!elem.itemsConsumed.emplace(itemId, count).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate item amount: itemId = ", itemId);
						DEBUG_THROW(Exception, sslit("Duplicate item amount"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", tradeId = ", elem.tradeId, ", str = ", str);
				throw;
			}

			csv.get(str, "obtain_item", "{}");
			try {
				std::istringstream iss(str);
				const auto root = Poseidon::JsonParser::parseObject(iss);
				elem.itemsProduced.reserve(root.size());
				for(auto it = root.begin(); it != root.end(); ++it){
					const auto itemId = boost::lexical_cast<ItemId>(it->first);
					const auto count = static_cast<boost::uint64_t>(it->second.get<double>());
					if(!elem.itemsProduced.emplace(itemId, count).second){
						LOG_EMPERY_CENTER_ERROR("Duplicate item amount: itemId = ", itemId);
						DEBUG_THROW(Exception, sslit("Duplicate item amount"));
					}
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what(), ", tradeId = ", elem.tradeId, ", str = ", str);
				throw;
			}

			if(!tradeMap->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate trade item: tradeId = ", elem.tradeId);
				DEBUG_THROW(Exception, sslit("Duplicate trade item"));
			}
		}
		g_tradeMap = tradeMap;
		handles.push(DataSession::createServlet(TRADE_FILE, serializeCsv(csv, "trading_id")));
		handles.push(tradeMap);
	}
}

namespace Data {
	boost::shared_ptr<const Item> Item::get(ItemId itemId){
		PROFILE_ME;

		const auto itemMap = g_itemMap.lock();
		if(!itemMap){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return { };
		}

		const auto it = itemMap->find<0>(itemId);
		if(it == itemMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("Item not found: itemId = ", itemId);
			return { };
		}
		return boost::shared_ptr<const Item>(itemMap, &*it);
	}
	boost::shared_ptr<const Item> Item::require(ItemId itemId){
		PROFILE_ME;

		auto ret = get(itemId);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Item not found"));
		}
		return ret;
	}

	void Item::getInit(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto itemMap = g_itemMap.lock();
		if(!itemMap){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return;
		}

		const auto begin = itemMap->upperBound<1>(0);
		const auto end = itemMap->end<1>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(itemMap, &*it);
		}
	}
	void Item::getAutoInc(std::vector<boost::shared_ptr<const Item>> &ret){
		PROFILE_ME;

		const auto itemMap = g_itemMap.lock();
		if(!itemMap){
			LOG_EMPERY_CENTER_WARNING("ItemMap has not been loaded.");
			return;
		}

		const auto begin = itemMap->upperBound<2>(AIT_NONE);
		const auto end = itemMap->end<2>();
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
		for(auto it = begin; it != end; ++it){
			ret.emplace_back(itemMap, &*it);
		}
	}

	boost::shared_ptr<const Trade> Trade::get(TradeId tradeId){
		PROFILE_ME;

		const auto tradeMap = g_tradeMap.lock();
		if(!tradeMap){
			LOG_EMPERY_CENTER_WARNING("TradeMap has not been loaded.");
			return { };
		}

		const auto it = tradeMap->find<0>(tradeId);
		if(it == tradeMap->end<0>()){
			LOG_EMPERY_CENTER_DEBUG("Trade not found: tradeId = ", tradeId);
			return { };
		}
		return boost::shared_ptr<const Trade>(tradeMap, &*it);
	}
	boost::shared_ptr<const Trade> Trade::require(TradeId tradeId){
		PROFILE_ME;

		auto ret = get(tradeId);
		if(!ret){
			DEBUG_THROW(Exception, sslit("Trade not found"));
		}
		return ret;
	}

	void Trade::unpack(std::vector<ItemTransactionElement> &transaction,
		const boost::shared_ptr<const Trade> &tradeData, boost::uint64_t repeatTimes,
		ReasonId reason, boost::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3)
	{
		PROFILE_ME;

		transaction.reserve(transaction.size() + tradeData->itemsConsumed.size() + tradeData->itemsProduced.size());
		for(auto it = tradeData->itemsConsumed.begin(); it != tradeData->itemsConsumed.end(); ++it){
			transaction.emplace_back(ItemTransactionElement::OP_REMOVE, it->first, checkedMul(it->second, repeatTimes),
				reason, param1, param2, param3);
		}
		for(auto it = tradeData->itemsProduced.begin(); it != tradeData->itemsProduced.end(); ++it){
			transaction.emplace_back(ItemTransactionElement::OP_ADD, it->first, checkedMul(it->second, repeatTimes),
				reason, param1, param2, param3);
		}
	}
}

}
