#ifndef EMPERY_CENTER_MAP_OBJECT_HPP_
#define EMPERY_CENTER_MAP_OBJECT_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MapObject;
	class Center_MapObjectAttribute;
}

class PlayerSession;
class ClusterSession;

class MapObject : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_MapObject> m_obj;

	boost::container::flat_map<AttributeId,
		boost::shared_ptr<MySql::Center_MapObjectAttribute>> m_attributes;

	// 非持久化数据。
	unsigned m_action = 0;
	std::string m_action_param;

public:
	MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid, MapObjectUuid parent_object_uuid,
		std::string name, Coord coord, std::uint64_t created_time, bool garrisoned);
	MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes);
	~MapObject();

public:
	virtual void pump_status();
	virtual void recalculate_attributes();

	MapObjectUuid get_map_object_uuid() const;
	MapObjectTypeId get_map_object_type_id() const;
	AccountUuid get_owner_uuid() const;
	MapObjectUuid get_parent_object_uuid() const;

	const std::string &get_name() const;
	void set_name(std::string name);

	Coord get_coord() const;
	void set_coord(Coord coord) noexcept;

	std::uint64_t get_created_time() const;

	bool has_been_deleted() const;
	void delete_from_game() noexcept;

	bool is_garrisoned() const;
	void set_garrisoned(bool garrisoned);

	std::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(boost::container::flat_map<AttributeId, std::int64_t> modifiers);

	std::uint64_t get_resource_amount_carried() const;

	unsigned get_action() const {
		return m_action;
	}
	const std::string &get_action_param() const {
		return m_action_param;
	}
	void set_action(unsigned action, std::string action_param){
		m_action       = action;
		m_action_param = std::move(action_param);
	}

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const;
};

inline void synchronize_map_object_with_player(const boost::shared_ptr<const MapObject> &map_object,
	const boost::shared_ptr<PlayerSession> &session)
{
	map_object->synchronize_with_player(session);
}
inline void synchronize_map_object_with_player(const boost::shared_ptr<MapObject> &map_object,
	const boost::shared_ptr<PlayerSession> &session)
{
	map_object->synchronize_with_player(session);
}
inline void synchronize_map_object_with_cluster(const boost::shared_ptr<const MapObject> &map_object,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	map_object->synchronize_with_cluster(cluster);
}
inline void synchronize_map_object_with_cluster(const boost::shared_ptr<MapObject> &map_object,
	const boost::shared_ptr<ClusterSession> &cluster)
{
	map_object->synchronize_with_cluster(cluster);
}

}

#endif
