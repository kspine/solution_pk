#ifndef EMPERY_GOLD_SCRAMBLE_SINGLETONS_GLOBAL_STATUS_HPP_
#define EMPERY_GOLD_SCRAMBLE_SINGLETONS_GLOBAL_STATUS_HPP_

#include <cstddef>
#include <cstdint>

namespace EmperyGoldScramble {

struct GlobalStatus {
	enum {
		SLOT_GAME_BEGIN_TIME            =  1,
		SLOT_GAME_END_TIME              =  2,
		SLOT_GOLD_COINS_IN_POT          =  3,
		SLOT_ACCOUNT_BALANCE_IN_POT     =  4,
		SLOT_PERCENT_WINNERS            =  5,

		SLOT_STATUS_INVALIDATED         = 70,
		SLOT_GAME_AUTO_ID               = 71,
		SLOT_RECORD_AUTO_ID             = 72,
	};

	static std::uint64_t get(unsigned slot);
	static std::uint64_t exchange(unsigned slot, std::uint64_t new_value);
	static std::uint64_t fetch_add(unsigned slot, std::uint64_t delta_value);
	static std::uint64_t fetch_sub(unsigned slot, std::uint64_t delta_value);

private:
	GlobalStatus() = delete;
};

}

#endif
