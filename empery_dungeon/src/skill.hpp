#ifndef EMPERY_DUNGEON_SKILL_HPP_
#define EMPERY_DUNGEON_SKILL_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"

namespace EmperyDungeon {
class DungeonClient;
class Dungeon;
class Skill : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	Skill(DungeonMonsterSkillId skill_id);
	~Skill();
private:
	DungeonMonsterSkillId m_skill_id;
	std::uint64_t         m_next_execute_time;
public:
	DungeonMonsterSkillId  get_skill_id(){
		return m_skill_id;
	}
	
	std::uint64_t  get_next_execute_time(){
		return m_next_execute_time;
	}
	
	void set_next_execute_time(std::uint64_t next_execute_time);
};

}

#endif
