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

class Dungeon : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum QuitReason {
		Q_DESTRUCTOR     = 0,
		Q_PLAYER_REQUEST = 1,
	};

private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonTypeId m_dungeon_type_id;

	const boost::weak_ptr<DungeonClient> m_dungeon_client;

	AccountUuid m_founder_uuid;
	boost::container::flat_map<DungeonObjectUuid, boost::shared_ptr<DungeonObject>> m_objects;
	boost::container::flat_map<std::string, boost::shared_ptr<Trigger>>             m_triggers;
	//boost::shared_ptr<Poseidon::TimerItem> m_dungeon_timer;
public:
	Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id,
		const boost::shared_ptr<DungeonClient> &dungeon_client,AccountUuid founder_uuid);
	~Dungeon();

public:
	virtual void pump_status();
	virtual void pump_triggers();

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

	boost::shared_ptr<DungeonClient> get_dungeon_client() const {
		return m_dungeon_client.lock();
	}
	void set_dungeon_duration(std::uint64_t duration);

	boost::shared_ptr<DungeonObject> get_object(DungeonObjectUuid dungeon_object_uuid) const;
	void get_objects_all(std::vector<boost::shared_ptr<DungeonObject>> &ret) const;
	void get_dungeon_objects_by_rectangle(std::vector<boost::shared_ptr<DungeonObject>> &ret,Rectangle rectangle) const;
	void get_dungeon_objects_by_account(std::vector<boost::shared_ptr<DungeonObject>> &ret,AccountUuid account_uuid);
	void insert_object(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists = true);
	void replace_dungeon_object_no_synchronize(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void reove_dungeon_object_no_synchronize(DungeonObjectUuid dungeon_object_uuid);

	void init_triggers();
	void check_triggers(const TriggerCondition &condition);
	void check_triggers_enter_dungeon(const TriggerCondition &condition);
	void parse_triggers_action(std::deque<TriggerAction> &actions,std::string effect,std::string effect_param);
	void on_triggers_action(const TriggerAction &action);
	void on_triggers_create_dungeon_object(const TriggerAction &action);
};

}

#endif
