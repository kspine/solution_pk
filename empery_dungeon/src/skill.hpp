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
	ID_SKILL_SUMMON_MONSTER            (1009), //召唤怪物
	ID_SKILL_ENERGY_SPHERE             (1010); //能量球

	constexpr DungeonBuffTypeId
	ID_BUFF_CORROSION                   (6404001),//腐蚀
	ID_BUFF_REFLEX_INJURY               (6405001),//反伤
	ID_BUFF_RAGE                        (6406001);//狂暴
}
class SkillRecycleDamage : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	Coord                   origin_coord;
	DungeonObjectUuid       dungon_objec_uuid;
	AccountUuid             owner_account_uuid;
	std::uint64_t           attack;
	DungeonMonsterSkillId   skill_id;
	std::uint64_t           next_damage_time;
	std::uint64_t           interval;
	std::uint64_t           times;
	std::vector<Coord>      damage_range;
public:
	SkillRecycleDamage(Coord coord_, DungeonObjectUuid dungon_objec_uuid_,AccountUuid owner_account_uuid_,std::uint64_t attack_,DungeonMonsterSkillId skill_id_, std::uint64_t next_damage_time_,std::uint64_t interval_,std::uint64_t times_,std::vector<Coord> damage_range_);
	~SkillRecycleDamage();
};

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
	std::string           m_cast_params;
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

	std::string get_cast_params(){
		return m_cast_params;
	}
	void set_cast_params(std::string params);
	virtual Skill_Direct   get_cast_direct();
	virtual void   do_effects();
	virtual void   notify_effects(const std::vector<Coord> &coords);
	virtual void   do_damage(const std::vector<Coord> &coords);
};


//顺劈斩
class SkillCleave : public Skill{
public:
	SkillCleave(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillCleave();

public:
	Skill_Direct   get_cast_direct() override;
	void   do_effects() override;
};

//旋风
class SkillCyclone : public Skill {
public:
	SkillCyclone(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillCyclone();

public:
	void   do_effects() override;
};

//火印
class SkillFireBrand : public Skill {
public:
	SkillFireBrand(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillFireBrand();

public:
	void do_effects() override;
	void do_damage(const std::vector<Coord> &coords) override;
};

//腐蚀术
class SkillCorrosion : public Skill {
public:
	SkillCorrosion(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillCorrosion();
public:
	void do_effects() override;
};

//反伤术
class SkillReflexInjury : public Skill {
public:
	SkillReflexInjury(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillReflexInjury();
public:
	void do_effects() override;
};

//狂暴
class SkillRage : public Skill {
public:
	SkillRage(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillRage();
public:
	void do_effects() override;
};

//召唤骷髅
class SkillSummonSkulls  : public Skill {
public:
	SkillSummonSkulls(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillSummonSkulls();
public:
	void do_effects() override;
};
//召唤野怪
class SkillSummonMonster  : public Skill {
public:
	SkillSummonMonster(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillSummonMonster();
public:
	void do_effects() override;
};

//灵魂攻击
class SkillSoulAttack  : public Skill {
public:
	SkillSoulAttack(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillSoulAttack();
public:
	void do_effects() override;
};

//能量球
class SkillEnergySphere  : public Skill {
public:
	SkillEnergySphere(DungeonMonsterSkillId skill_id,const boost::weak_ptr<DungeonObject> owner);
	~SkillEnergySphere();
public:
	void do_effects() override;
};






}

#endif
