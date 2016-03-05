#ifndef EMPERY_CENTER_DATA_GLOBAL_HPP_
#define EMPERY_CENTER_DATA_GLOBAL_HPP_

#include "common.hpp"
#include <poseidon/fwd.hpp>

namespace EmperyCenter {

namespace Data {
	class Global {
	public:
		enum Slot {
			SLOT_CASTLE_UPGRADE_CANCELLATION_REFUND_RATIO           = 100001,
			SLOT_DEFAULT_MAIL_EXPIRY_DURATION                       = 100002,
			SLOT_NON_BEST_RESOURCE_PRODUCTION_RATE_MODIFIER         = 100004,
			SLOT_NON_BEST_RESOURCE_CAPACITY_MODIFIER                = 100005,
			SLOT_ACCELERATION_CARD_PRODUCTION_RATE_MODIFIER         = 100006,
			SLOT_ACCELERATION_CARD_CAPACITY_MODIFIER                = 100007,
			SLOT_MINIMUM_DISTANCE_BETWEEN_CASTLES                   = 100008,
			SLOT_CASTLE_IMMEDIATE_BUILDING_UPGRADE_TRADE_ID         = 100011,
			SLOT_CASTLE_IMMEDIATE_TECH_UPGRADE_TRADE_ID             = 100012,
			SLOT_IMMIGRANT_CREATION_TRADE_ID                        = 100013,
			SLOT_BROADCAST_CHAT_MESSAGE_TRADE_ID                    = 100014,
			SLOT_MAX_MESSAGES_IN_ADJACENT_CHANNEL                   = 100015,
			SLOT_MAX_MESSAGES_IN_ALLIANCE_CHANNEL                   = 100016,
			SLOT_MAX_MESSAGES_IN_SYSTEM_CHANNEL                     = 100017,
			SLOT_MAX_MESSAGES_IN_TRADE_CHANNEL                      = 100018,
			SLOT_MIN_MESSAGE_INTERVAL_IN_ADJACENT_CHANNEL           = 100020,
			SLOT_MIN_MESSAGE_INTERVAL_IN_TRADE_CHANNEL              = 100021,
			SLOT_MIN_MESSAGE_INTERVAL_IN_ALLIANCE_CHANNEL           = 100022,
			SLOT_MAX_CONCURRENT_UPGRADING_BUILDING_COUNT            = 100023,
			SLOT_MAX_CONCURRENT_UPGRADING_TECH_COUNT                = 100024,
			SLOT_MAP_BORDER_THICKNESS                               = 100025,
			SLOT_MAP_SWITCH_MAX_BYPASSABLE_BLOCKS                   = 100026,
			SLOT_ACCELERATION_CARD_UNIT_PRICE                       = 100027,
			SLOT_AUCTION_TRANSFER_FEE_RATIO                         = 100028,
			SLOT_AUCTION_TAX_RATIO                                  = 100029,
			SLOT_AUCTION_EARNEST_PAYMENT                            = 100030,
			SLOT_AUCTION_TRANSFER_DURATION                          = 100031,
			SLOT_AUCTION_TRANSFER_ITEM_COUNT_PER_BOX                = 100033,
			SLOT_AUCTION_TRANSFER_RESOURCE_AMOUNT_PER_BOX           = 100034,
			SLOT_ADJACENT_CHAT_RANGE                                = 100035,
			SLOT_TAX_RECORD_EXPIRY_DAYS                             = 100036,
			SLOT_BATTALION_PRODUCTION_CANCELLATION_REFUND_RATIO     = 100037,
			SLOT_MAX_DAILY_TASK_COUNT                               = 100038,
			SLOT_CASTLE_IMMEDIATE_BATTALION_PRODUCTION_TRADE_ID     = 100039,
			SLOT_WAR_STATE_PERSISTENCE_DURATION                     = 100041,
			SLOT_ACTIVE_CASTLE_THRESHOLD_DAYS                       = 100042,
			SLOT_MIN_ACTIVE_CASTLES_BY_MAP_EVENT_CIRCLE             = 100043,
			SLOT_MAX_ACTIVE_CASTLES_BY_MAP_EVENT_CIRCLE             = 100044,
			SLOT_INIT_ACTIVE_CASTLES_BY_MAP_EVENT_CIRCLE            = 100045,
			SLOT_MAX_BATTLE_RECORD_COUNT                            = 100047,
			SLOT_BATTLE_RECORD_EXPIRY_DAYS                          = 100048,
		};

	public:
		static const std::string &as_string(Slot slot);
		static std::int64_t as_signed(Slot slot);
		static std::uint64_t as_unsigned(Slot slot);
		static double as_double(Slot slot);
		static const Poseidon::JsonArray &as_array(Slot slot);
		static const Poseidon::JsonObject &as_object(Slot slot);
	};
}

}

#endif
