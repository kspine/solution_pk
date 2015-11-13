#ifndef EMPERY_GOLD_SCRAMBLE_SINGLETONS_BID_RECORD_MAP_HPP_
#define EMPERY_GOLD_SCRAMBLE_SINGLETONS_BID_RECORD_MAP_HPP_

#include <boost/cstdint.hpp>
#include <string>

namespace EmperyGoldScramble {

struct BidRecordMap {
	struct Record {
		boost::uint64_t record_auto_id;
		boost::uint64_t game_auto_id;
		boost::uint64_t timestamp;
		std::string login_name;
		std::string nick;
		boost::uint64_t gold_coins;
		boost::uint64_t account_balance;
	};

	static void append(std::string login_name, std::string nick, boost::uint64_t gold_coins, boost::uint64_t account_balance);
	static std::size_t get_size();
	static void get_all(std::vector<Record> &ret, std::size_t limit);
	static void clear() noexcept;

private:
	BidRecordMap() = delete;
};

}

#endif
