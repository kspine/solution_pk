#include "precompiled.hpp"
#include "skill.hpp"

namespace EmperyDungeon {

Skill::Skill(DungeonMonsterSkillId skill_id)
	: m_skill_id(skill_id), m_next_execute_time(0)
{
}
Skill::~Skill(){
}

void Skill::set_next_execute_time(std::uint64_t next_execute_time){
	m_next_execute_time = next_execute_time;
}

}
