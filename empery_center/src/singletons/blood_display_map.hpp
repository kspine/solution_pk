#ifndef EMPERY_CENTER_SINGLETONS_BLOOD_DISPLAY_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_BLOOD_DISPLAY_MAP_HPP_
#include "../id_types.hpp"

namespace EmperyCenter {

struct BloodDisplayMap {
	static void get_display_blood_time(AccountUuid account_uuid,AccountUuid enemy_uuid,std::uint64_t& last_attack_time,std::uint64_t& last_display_time);
	static void update_attack_info(AccountUuid account_uuid,AccountUuid enemy_uuid,std::uint64_t last_atatack_time,std::uint64_t last_finish_display_time = 0);
private:
	BloodDisplayMap() = delete;
};

}

#endif
