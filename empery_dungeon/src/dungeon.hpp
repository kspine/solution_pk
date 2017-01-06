#ifndef EMPERY_DUNGEON_DUNGEON_HPP_
#define EMPERY_DUNGEON_DUNGEON_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"
#include "rectangle.hpp"

namespace EmperyDungeon {

class DungeonObject;
class DungeonClient;
class Trigger;
class TriggerCondition;
class TriggerAction;
class TriggerDamage;
class TriggerConfirmation;
class DungeonBuff;
class SkillRecycleDamage;
class DefenseMatrix;

class Dungeon : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum QuitReason {
		Q_DESTRUCTOR     = 0,
		Q_PLAYER_REQUEST = 1,
	};

	enum DungeonState {
		S_INIT           = 0,
		S_PASS           = 1,
		S_FAIL           = 2,
	};

	enum FightState {
		FIGHT_START          = 0,
		FIGHT_PAUSE          = 1,
	};

private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonTypeId m_dungeon_type_id;

	const boost::weak_ptr<DungeonClient> m_dungeon_client;

	AccountUuid m_founder_uuid;
	std::uint64_t                                                                   m_create_dungeon_time;
	std::uint64_t                                                                   m_finish_count;
	std::uint64_t                                                                   m_expiry_time;
	boost::container::flat_map<DungeonObjectTypeId,std::uint64_t>                   m_damage_solider;
	std::vector<std::uint64_t>                                                      m_finish_tasks;
	boost::container::flat_map<DungeonObjectUuid, boost::shared_ptr<DungeonObject>> m_objects;
	boost::container::flat_map<std::uint64_t, boost::shared_ptr<Trigger>>           m_triggers;
	boost::container::flat_map<std::string, boost::shared_ptr<TriggerConfirmation>> m_triggers_confirmation;
	std::vector<boost::shared_ptr<TriggerDamage>>                                   m_triggers_damages;
	DungeonState                                                                    m_dungeon_state;
	std::uint64_t                                                                   m_monster_removed_count;
	std::vector<std::string>                                                        m_die_objects;
	boost::container::flat_map<Coord, boost::shared_ptr<DungeonBuff>>               m_dungeon_buffs;
	std::set<Coord>                                                                 m_dungeon_blocks;
	FightState                                                                      m_fight_state;
	boost::container::flat_map<Coord,std::vector<boost::shared_ptr<SkillRecycleDamage>>>  m_skill_damages;
	boost::container::flat_map<std::pair<DungeonObjectUuid,DungeonBuffTypeId>,boost::shared_ptr<DungeonBuff>>   m_skill_buffs;
	std::vector<boost::shared_ptr<DefenseMatrix>>                                   m_defense_matrixs;
public:
	Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id,
		const boost::shared_ptr<DungeonClient> &dungeon_client,AccountUuid founder_uuid,std::uint64_t create_time,std::uint64_t finish_count,std::uint64_t expiry_time);
	~Dungeon();

public:
	virtual void pump_status();
	virtual void pump_triggers();
	virtual void pump_triggers_damage();

	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	DungeonTypeId get_dungeon_type_id() const {
		return m_dungeon_type_id;
	}
	AccountUuid get_founder_uuid() const {
		return m_founder_uuid;
	}
	void set_founder_uuid(AccountUuid founder_uuid);
	std::uint64_t get_expiry_time(){
		return m_expiry_time;
	}
	void set_expiry_time(std::uint64_t expiry_time);

	void update_damage_solider(DungeonObjectTypeId dungeon_object_type_id,std::uint64_t damage_solider);

	std::uint64_t get_total_damage_solider();

	std::uint64_t get_create_dungeon_time(){
		return m_create_dungeon_time;
	}

	boost::shared_ptr<DungeonClient> get_dungeon_client() const {
		return m_dungeon_client.lock();
	}
	FightState get_fight_state(){
		return m_fight_state;
	}
	void set_fight_state(FightState fight_state);

	void check_move_pass(Coord coord,std::string params,bool is_monster);
	boost::shared_ptr<DungeonObject> get_object(DungeonObjectUuid dungeon_object_uuid) const;
	boost::shared_ptr<DungeonObject> get_object(Coord coord) const ;
	void get_objects_all(std::vector<boost::shared_ptr<DungeonObject>> &ret) const;
	void get_dungeon_objects_by_rectangle(std::vector<boost::shared_ptr<DungeonObject>> &ret,Rectangle rectangle) const;
	void get_dungeon_objects_by_account(std::vector<boost::shared_ptr<DungeonObject>> &ret,AccountUuid account_uuid);
	void insert_object(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists = true);
	void replace_dungeon_object_no_synchronize(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void remove_dungeon_object_no_synchronize(DungeonObjectUuid dungeon_object_uuid);
	bool check_all_die(bool is_monster);
	bool check_valid_coord_for_birth(const Coord &src_coord);
	bool get_monster_birth_coord(const Coord &src_coord,Coord &dest_coord);

	//副本中触发器buff相关操作
	boost::shared_ptr<DungeonBuff> get_dungeon_buff(const Coord coord) const;
	void insert_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff);
	void update_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff, bool throws_if_not_exists = true);
	void replace_dungeon_buff_no_synchronize(const boost::shared_ptr<DungeonBuff> &dungeon_buff);
	void remove_dungeon_buff_no_synchronize(Coord coord);
	std::uint64_t get_dungeon_attribute(DungeonObjectUuid create_uuid,AccountUuid owner_uuid,AttributeId attribute_id);

	bool is_dungeon_blocks_coord(const Coord coord);

	void init_triggers();
	void check_triggers_enter_dungeon();
	void check_triggers_move_pass(Coord coord,std::string params,bool isMonster = false);
	void check_triggers_hp(std::string tag,std::uint64_t total_hp,std::uint64_t old_hp, std::uint64_t new_hp);
	void check_triggers_dungeon_finish();
	void check_triggers_all_die();
	void check_triggers_tag_die();
	void forcast_triggers(const boost::shared_ptr<Trigger> &trigger,std::uint64_t now);
	void parse_triggers_action(std::deque<TriggerAction> &actions,std::string effect,std::string effect_param);
	void on_triggers_action(const TriggerAction &action);
	void on_triggers_create_dungeon_object(const TriggerAction &action);
	void on_triggers_dungeon_object_damage(const TriggerAction &action);
	void do_triggers_damage(const boost::shared_ptr<TriggerDamage>& damage);
	void on_triggers_buff(const TriggerAction &action);
	void on_triggers_set_trigger(const TriggerAction &action);
	void on_triggers_dungeon_failed(const TriggerAction &action);
	void on_triggers_dungeon_finished(const TriggerAction &action);
	void on_triggers_dungeon_task_finished(const TriggerAction &action);
	void on_triggers_dungeon_task_failed(const TriggerAction &action);
	void on_triggers_dungeon_move_camera(const TriggerAction &action);
	void on_triggers_dungeon_set_scope(const TriggerAction &action);
	void on_triggers_dungeon_wait_for_confirmation(const TriggerAction &action);
	void on_triggers_dungeon_player_confirmation(std::string context);
	void on_triggers_dungeon_show_pictures(const TriggerAction &action);
	void on_triggers_dungeon_remove_pictures(const TriggerAction &action);
	void on_triggers_dungeon_range_damage(const TriggerAction &action);
	void on_triggers_dungeon_transmit(const TriggerAction &action);
	void on_triggers_dungeon_set_block(const TriggerAction &action);
	void on_triggers_dungeon_remove_block(const TriggerAction &action);
	void on_triggers_dungeon_pause_fight(const TriggerAction &action);
	void on_triggers_dungeon_restart_fight(const TriggerAction &action);
	void on_triggers_dungeon_hide_all_solider(const TriggerAction &action);
	void on_triggers_dungeon_unhide_all_solider(const TriggerAction &action);
	void on_triggers_dungeon_hide_coords(const TriggerAction &action);
	void on_triggers_dungeon_unhide_coords(const TriggerAction &action);
	void on_triggers_dungeon_defense_matrix(const TriggerAction &action);
	void on_triggers_dungeon_set_foot_annimation(const TriggerAction &action);
	void on_triggers_dungeon_play_sound(const TriggerAction &action);
	void notify_triggers_executive(const boost::shared_ptr<Trigger> &trigger);

	//
	virtual void pump_skill_damage();
	void do_skill_damage(const boost::shared_ptr<SkillRecycleDamage>& damage);
	void insert_skill_damage(const boost::shared_ptr<SkillRecycleDamage>& damage);
	void check_skill_damage_move_pass(Coord coord,std::string params,bool isMonster = false);
	//副本中技能产生的buff
	void insert_skill_buff(DungeonObjectUuid dungeon_object_uuid,DungeonBuffTypeId buff_id,const boost::shared_ptr<DungeonBuff> dungeon_buff);
	void remove_skill_buff(DungeonObjectUuid dungeon_object_uuid,DungeonBuffTypeId buff_id);
	
	//
	virtual void pump_defense_matrix();
};

}

#endif
