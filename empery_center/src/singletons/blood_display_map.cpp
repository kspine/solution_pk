#include "../precompiled.hpp"
#include "blood_display_map.hpp"
#include <poseidon/multi_index_map.hpp>

namespace EmperyCenter {

namespace {
	struct BloodDisplayElement {
		AccountUuid      account_uuid;
		AccountUuid      enemy_account_uuid;
		std::pair<AccountUuid,AccountUuid> fight_sides; 
		std::uint64_t    last_attack_time;
		std::uint64_t    last_finish_display_blood_time;
		BloodDisplayElement(AccountUuid account_uuid,AccountUuid enemy_account_uuid, std::uint64_t last_attack_time,std::uint64_t last_finish_display_blood_time)
			: account_uuid(account_uuid)
			, enemy_account_uuid(enemy_account_uuid)
			, fight_sides(std::make_pair(account_uuid,enemy_account_uuid))
			, last_attack_time(last_attack_time)
			, last_finish_display_blood_time(last_finish_display_blood_time)
		{
		}
	};

	MULTI_INDEX_MAP(BloodDisplayInfosContainer, BloodDisplayElement,
		MULTI_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(enemy_account_uuid)
		UNIQUE_MEMBER_INDEX(fight_sides)
		MULTI_MEMBER_INDEX(last_finish_display_blood_time)
	)

	boost::weak_ptr<BloodDisplayInfosContainer> g_blood_display_info_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto blood_display_info_map = boost::make_shared<BloodDisplayInfosContainer>();
		g_blood_display_info_map = blood_display_info_map;
		handles.push(blood_display_info_map);
	}

}

	void BloodDisplayMap::get_display_blood_time(AccountUuid account_uuid,AccountUuid enemy_uuid,std::uint64_t& last_attack_time,std::uint64_t& last_display_time){
		PROFILE_ME;
		const auto blood_display_info_map = g_blood_display_info_map.lock();
		const auto it = blood_display_info_map->find<2>(std::make_pair(account_uuid,enemy_uuid));
		if(it != blood_display_info_map->end<2>()){
			last_attack_time  = it->last_attack_time;
			last_display_time = it->last_finish_display_blood_time;
		}
	}
	
	void BloodDisplayMap::update_attack_info(AccountUuid account_uuid,AccountUuid enemy_uuid,std::uint64_t last_atatack,std::uint64_t last_finish_display_blood){
		PROFILE_ME;
		const auto blood_display_info_map = g_blood_display_info_map.lock();
		if(!blood_display_info_map){
			LOG_EMPERY_CENTER_WARNING("no base fight info now.");
			DEBUG_THROW(Exception, sslit("no base fight info now."));
		}
		
		const auto utc_now = Poseidon::get_utc_time();
		blood_display_info_map->erase<3>(blood_display_info_map->begin<3>(), blood_display_info_map->upper_bound<3>(utc_now));
		
		const auto it = blood_display_info_map->find<2>(std::make_pair(account_uuid,enemy_uuid));
		if(it == blood_display_info_map->end<2>()){
			blood_display_info_map->insert(BloodDisplayElement(account_uuid,enemy_uuid,last_atatack,last_finish_display_blood));
		}else{
			std::uint64_t last_finish_time;
			if(0 != last_finish_display_blood){
				last_finish_time = last_finish_display_blood;
			}else{
				last_finish_time = it->last_finish_display_blood_time;
			}
			blood_display_info_map->replace<2>(it,BloodDisplayElement(account_uuid,enemy_uuid,last_atatack,last_finish_time));
		}
	}


}
