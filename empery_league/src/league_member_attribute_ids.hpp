#ifndef EMPERY_LEAGUE_LEAGUE_MEMBER_ATTRIBUTE_IDS_HPP_
#define EMPERY_LEAGUE_LEAGUE_MEMBER_ATTRIBUTE_IDS_HPP_

#include "id_types.hpp"

namespace EmperyLeague {
	namespace LeagueMemberAttributeIds {
		constexpr LeagueMemberAttributeId

			ID_BEGIN(0),
			ID_CUSTOM_PUBLIC_END(100),
			ID_CUSTOM_END(200),
			ID_PUBLIC_END(300),
			ID_END(500),

			ID_TITLEID(101),
			ID_SPEAKFLAG(102);
	}
}

#endif