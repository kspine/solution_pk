#ifndef EMPERY_GOLD_SCRAMBLE_SINGLETONS_BID_RECORD_MAP_HPP_
#define EMPERY_GOLD_SCRAMBLE_SINGLETONS_BID_RECORD_MAP_HPP_

#include <boost/cstdint.hpp>
#include <string>

namespace EmperyGoldScramble {

struct BidRecordMap {
	struct Record {
		boost::uint64_t recordAutoId;
		boost::uint64_t gameAutoId;
		boost::uint64_t timestamp;
		std::string loginName;
		std::string nick;
		boost::uint64_t goldCoins;
		boost::uint64_t accountBalance;
	};

	static void append(std::string loginName, std::string nick, boost::uint64_t goldCoins, boost::uint64_t accountBalance);
	static std::size_t getSize();
	static void getAll(std::vector<Record> &ret, std::size_t limit);
	static void clear() noexcept;

private:
	BidRecordMap() = delete;
};

}

#endif
