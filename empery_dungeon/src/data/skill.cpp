#include "../precompiled.hpp"
#include "skill.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyDungeon {

namespace {
	MULTI_INDEX_MAP(MonsterSkillContainer, Data::Skill,
		UNIQUE_MEMBER_INDEX(skill_id)
	)
	boost::weak_ptr<const MonsterSkillContainer> g_monster_skill_container;
	const char MONSTER_SKILL_FILE[] = "dungeon_monster_skill";

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto monster_skill_container = boost::make_shared<MonsterSkillContainer>();

		auto csv = Data::sync_load_data(MONSTER_SKILL_FILE);
		while(csv.fetch_row()){
			Data::Skill elem = { };
			csv.get(elem.skill_id,           "id");
			csv.get(elem.cast_type,          "caster_releas");
			Poseidon::JsonArray array;
			csv.get(array,                   "caster_interval");
			if(array.size() != 2){
				LOG_EMPERY_DUNGEON_ERROR("error dungeon monster skill caster_interval, skill_id = ",elem.skill_id);
				DEBUG_THROW(Exception, sslit("error dungeon monster skill caster_interval"));
			}
			auto low = static_cast<std::uint32_t>(array.at(0).get<double>());
			auto high = static_cast<std::uint32_t>(array.at(1).get<double>());
			elem.cast_interval = std::make_pair(low,high);
			csv.get(elem.cast_object,             "caster_object");
			csv.get(elem.cast_range,              "caster_range");
			csv.get(elem.sing_time,               "sing_time");
			csv.get(elem.cast_time,               "cast_time");

			array.clear();
			csv.get(array,                       "damage_coefficient");
			if(array.size() == 2){
				elem.attack_rate = array.at(0).get<double>();
				elem.attack_fix  = array.at(1).get<double>();
			}else if(array.size() == 1){
				elem.attack_rate = array.at(0).get<double>();
				elem.attack_fix  = 0.0;
			}
			else{
				elem.attack_rate = 0.0;
				elem.attack_fix  = 0.0;
			}
			csv.get(elem.attack_type,               "attack_type");
			csv.get(elem.skill_range,               "skill_range");
			csv.get(elem.buff_id,                   "buff_id");
			csv.get(elem.pet_id,                    "pet_id");
			csv.get(elem.random_lattice,            "random_lattice");
			csv.get(elem.warning_time,              "warning_time");
			auto skill_id = elem.skill_id;
			if(!monster_skill_container->insert(std::move(elem)).second){
				LOG_EMPERY_DUNGEON_ERROR("Duplicate dungeon monster skill, skill_id = ", skill_id);
				DEBUG_THROW(Exception, sslit("Duplicate dungeon monster skill"));
			}
		}
		g_monster_skill_container = monster_skill_container;
		handles.push(monster_skill_container);
	}
}

namespace Data {
	boost::shared_ptr<const Skill> Skill::get(DungeonMonsterSkillId skill_id){
		PROFILE_ME;

		const auto monster_skill_container = g_monster_skill_container.lock();
		if(!monster_skill_container){
			LOG_EMPERY_DUNGEON_WARNING("monster_skill_container has not been loaded.");
			return { };
		}

		const auto it = monster_skill_container->find<0>(skill_id);
		if(it == monster_skill_container->end<0>()){
			LOG_EMPERY_DUNGEON_TRACE("dugeon monster Skill not found: Skill_id = ",skill_id);
			return { };
		}
		return boost::shared_ptr<const Skill>(monster_skill_container, &*it);
	}

	boost::shared_ptr<const Skill> Skill::require(DungeonMonsterSkillId skill_id){
		PROFILE_ME;

		auto ret = get(skill_id);
		if(!ret){
			LOG_EMPERY_DUNGEON_TRACE("dugeon monster Skill not found: Skill_id = ",skill_id);
			DEBUG_THROW(Exception, sslit("dungeon monster Skill not found"));
		}
		return ret;
	}

	void Skill::get_all(std::vector<boost::shared_ptr<const Skill>> &ret){
		PROFILE_ME;

		const auto monster_skill_container = g_monster_skill_container.lock();
		if(!monster_skill_container){
			LOG_EMPERY_DUNGEON_WARNING("monster_skill_container has not been loaded.");
			return ;
		}
		ret.reserve(ret.size() + monster_skill_container-> size());
		for(auto it = monster_skill_container->begin(); it != monster_skill_container->end(); ++it){
			ret.emplace_back(monster_skill_container,&*it);
		}
	}
}

}
