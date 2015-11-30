#ifndef EMPERY_CENTER_DATA_GLOBAL_HPP_
#define EMPERY_CENTER_DATA_GLOBAL_HPP_

#include "common.hpp"
#include <poseidon/fwd.hpp>

namespace EmperyCenter {

namespace Data {
	class Global {
	public:
		enum Slot {
			SLOT_CASTLE_CANCELLATION_REFUND_RATIO           = 100001,
			SLOT_DEFAULT_MAIL_EXPIRY_DURATION               = 100002,
			SLOT_MAP_SIZE                                   = 100003,
			SLOT_NON_BEST_RESOURCE_PRODUCTION_RATE_MODIFIER = 100004,
			SLOT_NON_BEST_RESOURCE_CAPACITY_MODIFIER        = 100005,
			SLOT_ACCELERATION_CARD_PRODUCTION_RATE_MODIFIER = 100006,
			SLOT_ACCELERATION_CARD_CAPACITY_MODIFIER        = 100007,
			SLOT_MINIMUM_DISTANCE_BETWEEN_CASTLES           = 100008,
			SLOT_PRODUCIBLE_RESOURCES                       = 100009,
			SLOT_MAP_CELL_UPGRADE_TRADE_ID                  = 100010,
		};

	public:
		static const std::string &as_string(Slot slot);
		static boost::int64_t as_signed(Slot slot);
		static boost::uint64_t as_unsigned(Slot slot);
		static double as_double(Slot slot);
		static const Poseidon::JsonArray &as_array(Slot slot);
		static const Poseidon::JsonObject &as_object(Slot slot);
	};
}

}

#endif
