#ifndef EMPERY_CENTER_DUNGEON_HPP_
#define EMPERY_CENTER_DUNGEON_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"
#include "rectangle.hpp"
#include "player_session.hpp"
#include "dungeon_session.hpp"

namespace EmperyCenter {

class DungeonObject;
class PlayerSession;
class DungeonSession;
class DungeonBuff;

class Dungeon : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum QuitReason {
		Q_DUNGEON_EXPIRED  = 0,
		Q_INTERNAL_ERROR   = 1,
		Q_PLAYER_REQUEST   = 2,
		Q_PLAYER_WINS      = 3,
		Q_PLAYER_LOSES     = 4,
	};

	struct Suspension {
		std::string context;
		unsigned type;
		std::int64_t param1;
		std::int64_t param2;
		std::string param3;
	};

	struct SoldierStat {
		std::uint64_t damaged;
		std::uint64_t resuscitated;
		std::uint64_t wounded;
	};

private:
	const DungeonUuid m_dungeon_uuid;
	const DungeonTypeId m_dungeon_type_id;
	const boost::weak_ptr<DungeonSession> m_server;

	AccountUuid m_founder_uuid;
	std::uint64_t m_create_time;
	std::uint64_t m_expiry_time;
	std::uint64_t m_finish_count;
	bool          m_begin;

	struct Observer {
		boost::weak_ptr<PlayerSession> session;
		boost::container::flat_map<MapObjectTypeId, SoldierStat> soldier_stats;
	};
	boost::container::flat_map<AccountUuid, Observer> m_observers;
	boost::container::flat_map<DungeonObjectUuid, boost::shared_ptr<DungeonObject>> m_objects;
	boost::container::flat_map<Coord, boost::shared_ptr<DungeonBuff>>               m_dungeon_buffs;
	Rectangle m_scope;
	Suspension m_suspension = { };

public:
	Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id, const boost::shared_ptr<DungeonSession> &server,
		AccountUuid founder_uuid,std::uint64_t create_time, std::uint64_t expiry_time,std::uint64_t finish_count);
	~Dungeon();

private:
	template<typename T>
	void synchronize_with_all_observers(const boost::shared_ptr<T> &dungeon_object) const noexcept{
			PROFILE_ME;

			for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
				const auto session = it->second.session.lock();
				if(session){
					try {
						dungeon_object->synchronize_with_player(session);
					} catch(std::exception &e){
						LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
						session->shutdown(e.what());
					}
				}
			}
	}

	template<typename T>
	void synchronize_with_dungeon_server(const boost::shared_ptr<T> &dungeon_object) const noexcept{
		PROFILE_ME;

		const auto server = m_server.lock();
		if(server){
			try {
				dungeon_object->synchronize_with_dungeon_server(server);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				server->shutdown(e.what());
			}
		}
	}

public:
	virtual void pump_status();

	DungeonUuid get_dungeon_uuid() const {
		return m_dungeon_uuid;
	}
	DungeonTypeId get_dungeon_type_id() const {
		return m_dungeon_type_id;
	}
	boost::shared_ptr<DungeonSession> get_server() const {
		return m_server.lock();
	}

	AccountUuid get_founder_uuid() const {
		return m_founder_uuid;
	}
	void set_founder_uuid(AccountUuid founder_uuid) noexcept;

	std::uint64_t get_expiry_time() const {
		return m_expiry_time;
	}
	void set_expiry_time(std::uint64_t expiry_time) noexcept;
	
	std::uint64_t get_create_time() const {
		return m_create_time;
	}
	
	bool is_begin() const {
		return m_begin;
	}

	void set_begin(bool begin) noexcept;

	Rectangle get_scope() const {
		return m_scope;
	}
	void set_scope(Rectangle scope);

	const Suspension &get_suspension() const {
		return m_suspension;
	}
	void set_suspension(Suspension suspension);

	boost::shared_ptr<PlayerSession> get_observer(AccountUuid account_uuid) const;
	void get_observers_all(std::vector<std::pair<AccountUuid, boost::shared_ptr<PlayerSession>>> &ret) const;
	void insert_observer(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session);
	bool remove_observer(AccountUuid account_uuid, QuitReason reason, const char *param) noexcept;
	void clear_observers(QuitReason reason, const char *param) noexcept;

	void get_soldier_stats(std::vector<std::pair<MapObjectTypeId, SoldierStat>> &ret, AccountUuid account_uuid) const;
	void update_soldier_stat(AccountUuid account_uuid, MapObjectTypeId map_object_type_id,
		std::uint64_t damaged, std::uint64_t resuscitated, std::uint64_t wounded);

	void broadcast_to_observers(std::uint16_t message_id, const Poseidon::StreamBuffer &payload);
	template<class MessageT>
	void broadcast_to_observers(const MessageT &msg){
		broadcast_to_observers(MessageT::ID, Poseidon::StreamBuffer(msg));
	}
	boost::shared_ptr<DungeonObject> get_object(DungeonObjectUuid dungeon_object_uuid) const;
	void get_objects_all(std::vector<boost::shared_ptr<DungeonObject>> &ret) const;
	void insert_object(const boost::shared_ptr<DungeonObject> &dungeon_object);
	void update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists = true);

	boost::shared_ptr<DungeonBuff> get_dungeon_buff(Coord coord);
	void insert_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff);
	void update_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff, bool throws_if_not_exists = true);

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
