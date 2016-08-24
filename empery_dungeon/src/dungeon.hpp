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

private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonTypeId m_dungeon_type_id;

	const boost::weak_ptr<DungeonClient> m_dungeon_client;

	AccountUuid m_founder_uuid;
	std::uint64_t                                                                   m_create_dungeon_time;
	boost::container::flat_map<DungeonObjectTypeId,std::uint64_t>                   m_damage_solider;
	std::vector<std::uint64_t>                                                      m_finish_tasks;
	boost::container::flat_map<DungeonObjectUuid, boost::shared_ptr<DungeonObject>> m_objects;
	boost::container::flat_map<std::uint64_t, boost::shared_ptr<Trigger>>           m_triggers;
	boost::container::flat_map<std::string, boost::shared_ptr<TriggerConfirmation>> m_triggers_confirmation;
	std::vector<boost::shared_ptr<TriggerDamage>>                                   m_triggers_damages;
	boost::shared_ptr<Poseidon::TimerItem>                                          m_trigger_timer;
	DungeonState                                                                    m_dungeon_state;
	std::uint64_t                                                                   m_monster_removed_count;
public:
	Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id,
		const boost::shared_ptr<DungeonClient> &dungeon_client,AccountUuid founder_uuid,std::uint64_t create_time);
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

	void update_damage_solider(DungeonObjectTypeId dungeon_object_type_id,std::uint64_t damage_solider);

	std::uint64_t get_total_damage_solider();

	std::uint64_t get_create_dungeon_time(){
		return m_create_dungeon_time;
	}

	boost::shared_ptr<DungeonClient> get_dungeon_client() const {
		return m_dungeon_client.lock();
	}
	boost::shared_ptr<DungeonObject> get_object(DungeonObjectUuid dungeon_object_uuid) const;
	void get_objects_all(std::vector<boost::shared_ptr<DungeonObject>> &ret) const;
	void get_dungeon_objects_by_rectangle(std::vector<boost::shared_ptr<DungeonObject>> &ret,Rectangle rectangle) const;
	void get_dungeon_objects_by_account(std::vector<boost::shared_ptr<DungeonObject>> &ret,AccountUuid account_uuid);
	void insert_object(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists = true);
	void replace_dungeon_object_no_synchronize(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void reove_dungeon_object_no_synchronize(DungeonObjectUuid dungeon_object_uuid);
	bool check_all_die(bool is_monster);
	bool check_valid_coord_for_birth(const Coord &src_coord);
	bool get_monster_birth_coord(const Coord &src_coord,Coord &dest_coord);

	void init_triggers();
	void check_triggers_enter_dungeon();
	void check_triggers_move_pass(Coord coord,bool isMonster = false);
	void check_triggers_hp(std::string tag,std::uint64_t total_hp,std::uint64_t old_hp, std::uint64_t new_hp);
	void check_triggers_dungeon_finish();
	void check_triggers_all_die();
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
};

}

#endif
