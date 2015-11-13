#ifndef EMPERY_CENTER_DATA_ITEM_HPP_
#define EMPERY_CENTER_DATA_ITEM_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCenter {

class ItemTransactionElement;

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

	public:
		static boost::shared_ptr<const Item> get(ItemId itemId);
		static boost::shared_ptr<const Item> require(ItemId itemId);

		static void getInit(std::vector<boost::shared_ptr<const Item>> &ret);
		static void getAutoInc(std::vector<boost::shared_ptr<const Item>> &ret);

	public:
		ItemId itemId;
		unsigned quality;
		std::pair<unsigned, unsigned> type;
		boost::uint64_t value;

		boost::uint64_t initCount;

		AutoIncType autoIncType;
		boost::uint64_t autoIncOffset;
		boost::int64_t autoIncStep;
		boost::uint64_t autoIncBound;
	};

	class Trade {
	public:
		static boost::shared_ptr<const Trade> get(TradeId tradeId);
		static boost::shared_ptr<const Trade> require(TradeId tradeId);

		static void unpack(std::vector<ItemTransactionElement> &transaction,
			const boost::shared_ptr<const Trade> &tradeData, boost::uint64_t repeatTimes,
			ReasonId reason, boost::uint64_t param1, boost::uint64_t param2, boost::uint64_t param3);

	public:
		TradeId tradeId;
		boost::container::flat_map<ItemId, boost::uint64_t> itemsConsumed;
		boost::container::flat_map<ItemId, boost::uint64_t> itemsProduced;
	};
}

}

#endif
