#include "../precompiled.hpp"
#include "base_fight_map.hpp"
#include <poseidon/multi_index_map.hpp>

namespace EmperyCenter {

namespace {
	struct BaseFightSnapElement {
		AccountUuid      account_uuid;
		AccountUuid      enemy_account_uuid;
		std::pair<AccountUuid,AccountUuid> fight_sides; 
		std::uint64_t    last_attack_time;
		std::uint64_t    last_finish_show_blood_time;
		BaseFightSnapElement(AccountUuid account_uuid_,AccountUuid enemy_account_uuid, std::uint64_t last_attack_time,std::uint64_t last_finish_show_blood_time)
			: account_uuid(account_uuid_)
			, enemy_account_uuid(enemy_account_uuid)
			, fight_sides(std::make_pair(account_uuid,enemy_account_uuid))
			, last_attack_time(last_attack_time)
			, last_finish_show_blood_time(last_finish_show_blood_time)
		{
		}
	};

	MULTI_INDEX_MAP(BaseFightInfosContainer, BaseFightSnapElement,
		MULTI_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(enemy_account_uuid)
		UNIQUE_MEMBER_INDEX(fight_sides)
	)

	boost::weak_ptr<BaseFightInfosContainer> g_bases_fight_info_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto base_fight_info_map = boost::make_shared<BaseFightInfosContainer>();
		g_bases_fight_info_map = base_fight_info_map;
		handles.push(base_fight_info_map);
	}

}

	std::uint64_t BaseFightMap::get_last_finish_show_blood_time(AccountUuid accountUuid,AccountUuid enemyUuid){
		PROFILE_ME;
		const auto base_fight_info_map = g_bases_fight_info_map.lock();
		const auto it = base_fight_info_map->find<2>(std::make_pair(accountUuid,enemyUuid));
		if(it != base_fight_info_map->end<2>()){
			return it->last_finish_show_blood_time;
		}
		return 0;
	}
	void BaseFightMap::update_attack_info(AccountUuid accountUuid,AccountUuid enemyUuid,std::uint64_t last_atatack,std::uint64_t last_finish_show_blood){
		PROFILE_ME;
		const auto base_fight_info_map = g_bases_fight_info_map.lock();
		if(!base_fight_info_map){
			LOG_EMPERY_CENTER_WARNING("no base fight info now.");
			DEBUG_THROW(Exception, sslit("no base fight info now."));
		}
		const auto it = base_fight_info_map->find<2>(std::make_pair(accountUuid,enemyUuid));
		if(it == base_fight_info_map->end<2>()){
			base_fight_info_map->insert(BaseFightSnapElement(accountUuid,enemyUuid,last_atatack,last_finish_show_blood));
		}else{
			std::uint64_t last_finish_time;
			if(0 != last_finish_show_blood){
				last_finish_time = last_finish_show_blood;
			}else{
				last_finish_time = it->last_finish_show_blood_time;
			}
			base_fight_info_map->replace<2>(it,BaseFightSnapElement(accountUuid,enemyUuid,last_atatack,last_finish_time));
			
		}
	}


}
