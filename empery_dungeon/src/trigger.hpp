#ifndef EMPERY_DUNGEON_TRIGGER_HPP_
#define EMPERY_DUNGEON_TRIGGER_HPP_
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyDungeon {

struct TriggerCondition {
public:
	enum Type {
		C_ENTER_DUNGEON               = 1, // 进入副本触发
		C_DUNGEON_OBJECT_PASS         = 2, // 部队通过
		C_DUNGEON_OBJECT_HP           = 3, // HP
		C_DUNGEON_FINISH              = 4, // 通关触发(通关之后需要立即执行)
	};

	Type                     type;
	std::string              params;
};

struct TriggerAction {
public:
	enum Type {
		A_CREATE_DUNGEON_OBJECT  = 1,//刷怪
		A_DUNGEON_OBJECT_DAMAGE  = 2,//造成伤害
		A_BUFF                   = 3,//buff
		A_SET_TRIGGER            = 4,//开启关闭trigger
		A_DUNGEON_FINISHED       = 5,//副本通关
		A_DUNGEON_FAILED         = 6,//通关失败
		A_SET_TASK_FINISHED      = 7,//指定任务完成
		A_SET_TASK_FAIL          = 8,//指定任务失败
		A_SET_SCOPE              = 9,//镜头移动
		A_LOCK_VIEW              = 10,//锁定视野
	};

	Type                     type;
	std::string              params;
};

struct TriggerDamage {
public:
	bool          is_target_monster;
	std::uint64_t loop;
	std::uint64_t damage;
	std::uint64_t delay;
	std::uint64_t next_damage_time;
	std::vector<Coord> damage_range;
public:
	TriggerDamage(bool is_target_monster_,std::uint64_t loop_,std::uint64_t damage_,std::uint64_t delay_, std::uint64_t next_damage_time_, std::vector<Coord> damage_range_);
	~TriggerDamage();
};

struct Trigger {
public:
	DungeonUuid                dungeon_uuid;
	std::string                name;
	std::uint64_t              delay;
	TriggerCondition           condition;
	std::deque<TriggerAction>  actions;
	bool                       activated = false;
	std::uint64_t              activated_time;
public:
	Trigger(DungeonUuid dungeon_uuid_,std::string name_,std::uint64_t delay_,TriggerCondition condition,std::deque<TriggerAction> actions_,bool activited_,std::uint64_t activated_time_);
	~Trigger();
};

}


#endif
