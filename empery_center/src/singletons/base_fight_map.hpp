#ifndef EMPERY_CENTER_SINGLETONS_BASE_FIGHT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_BASE_FIGHT_MAP_HPP_
#include "../id_types.hpp"

namespace EmperyCenter {

struct BaseFightMap {
	static std::uint64_t get_last_finish_show_blood_time(AccountUuid accountUuid,AccountUuid enemyUuid);
	static void update_attack_info(AccountUuid accountUuid,AccountUuid enemyUuid,std::uint64_t last_atatack,std::uint64_t last_notify = 0);
private:
	BaseFightMap() = delete;
};

}

#endif
