#ifndef EMPERY_LEAGUE_DATA_GLOBAL_HPP_
#define EMPERY_LEAGUE_DATA_GLOBAL_HPP_

#include "common.hpp"
#include <poseidon/fwd.hpp>

namespace EmperyLeague {

namespace Data {
	class Global {
	public:
		enum Slot {
			SLOT_LEGAUE_APPLYJOIN_MAX                              	= 100132,
			SLOT_LEGAUE_APPLYJOIN_EFFECT_MINUTE                    	= 100133,
			SLOT_LEGAUE_INVITEJOIN_EFFECT_MINUTE                   	= 100134,
			SLOT_LEGAUE_CREATE_NEED                                	= 100135,
			SLOT_LEGAUE_CREATE_DEFAULT_MEMBERCOUNT                 	= 100136,
			SLOT_LEGAUE_EXPAND_CONSUME                 			   	= 100137,
			SLOT_LEAGUE_LEAVE_WAIT_MINUTE							= 100138,
			SLOT_LEAGUE_ATTORN_WAIT_MINUTE						    = 100139,
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
