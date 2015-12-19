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
			// SLOT_MAP_CELL_UPGRADE_TRADE_ID                  = 100010,
			SLOT_CASTLE_BUILDING_IMMEDIATE_UPGRADE_TRADE_ID = 100011,
			SLOT_CASTLE_TECH_IMMEDIATE_UPGRADE_TRADE_ID     = 100012,
			SLOT_IMMIGRANT_CREATION_TRADE_ID                = 100013,
			SLOT_BROADCAST_CHAT_MESSAGE_TRADE_ID            = 100014,
			SLOT_MAX_MESSAGES_IN_ADJACENT_CHANNEL           = 100015,
			SLOT_MAX_MESSAGES_IN_ALLIANCE_CHANNEL           = 100016,
			SLOT_MAX_MESSAGES_IN_SYSTEM_CHANNEL             = 100017,
			SLOT_MAX_MESSAGES_IN_TRADE_CHANNEL              = 100018,
			//                                                 = 100019,
			SLOT_MIN_MESSAGE_INTERVAL_IN_ADJACENT_CHANNEL   = 100020,
			SLOT_MIN_MESSAGE_INTERVAL_IN_TRADE_CHANNEL      = 100021,
			SLOT_MIN_MESSAGE_INTERVAL_IN_ALLIANCE_CHANNEL   = 100022,
			SLOT_MAX_CONCURRENT_UPGRADING_BUILDING_COUNT    = 100023,
			SLOT_MAX_CONCURRENT_UPGRADING_TECH_COUNT        = 100024,
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
