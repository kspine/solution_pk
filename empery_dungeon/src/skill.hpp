#ifndef EMPERY_DUNGEON_SKILL_HPP_
#define EMPERY_DUNGEON_SKILL_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyDungeon {
namespace {
	constexpr DungeonMonsterSkillId
	ID_SKILL_CORROSION                 (1001), //腐蚀术
	ID_SKILL_SUMMON_SKULLS             (1002), //召唤骷髅
	ID_SKILL_REFLEX_INJURY             (1003), //反伤术
	ID_SKILL_SOUL_ATTACK               (1004), //灵魂攻击
	ID_SKILL_RAGE                      (1005), //狂暴
    ID_SKILL_CLEAVE                    (1006), //顺劈斩
	ID_SKILL_CYCLONE                   (1007), //旋风
	ID_SKILL_FIRE_BRAND                (1008), //火印
	ID_SKILL_SUMMON_MONSTER            (1009); //召唤怪物
}

class DungeonClient;
class Dungeon;
class DungeonObject;
class Skill : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Skill_Direct{
		DIRECT_NONE       = -1,
		DIRECT_RIGHT      = 0,
		DIRECT_LEFT       = 1,
		DIRECT_UP_LEFT    = 2,
		DIRECT_UP_RIGHT   = 3,
		DIRECT_DOWN_LEFT  = 4,
		DIRECT_DOWN_RIGHT = 5,
	};
public:
	Skill(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~Skill();
public:
	DungeonMonsterSkillId m_skill_id;
	boost::weak_ptr<DungeonObject>  m_owner;
	std::uint64_t         m_next_execute_time;
	Coord                 m_cast_coord;
public:
	DungeonMonsterSkillId  get_skill_id(){
		return m_skill_id;
	}

	boost::shared_ptr<DungeonObject> get_owner(){
		return m_owner.lock();
	}

	std::uint64_t  get_next_execute_time(){
		return m_next_execute_time;
	}
	void set_next_execute_time(std::uint64_t next_execute_time);

	Coord get_cast_coord(){
		return m_cast_coord;
	}
	void set_cast_coord(Coord coord);
	Skill_Direct get_cast_direct();
	virtual void   do_effects();
};

//顺劈斩
class SkillCleave : public Skill{
public:
	SkillCleave(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillCleave();

public:
	void   do_effects() override;
};

}

#endif
