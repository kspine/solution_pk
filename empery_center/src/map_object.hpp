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
	class Center_MapObjectBuff;
}

class Castle;
class PlayerSession;
class ClusterSession;

class MapObject : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
		enum Action {
		ACT_GUARD                             = 0,
		ACT_ATTACK                            = 1,
		ACT_DEPLOY_INTO_CASTLE                = 2,
//		ACT_HARVEST_OVERLAY                   = 3,
		ACT_ENTER_CASTLE                      = 4,
		ACT_HARVEST_STRATEGIC_RESOURCE        = 5,
		ACT_ATTACK_TERRITORY_FORCE            = 6,
		ACT_MONTER_REGRESS             		  = 7,
		ACT_HARVEST_RESOURCE_CRATE            = 8,
		ACT_ATTACK_TERRITORY           		  = 9,
//		ACT_HARVEST_OVERLAY_FORCE             = 10,
		ACT_HARVEST_STRATEGIC_RESOURCE_FORCE  = 11,
		ACT_HARVEST_RESOURCE_CRATE_FORCE      = 12,
		ACT_STAND_BY                          = 13,
		ACT_HARVEST_LEGION_RESOURCE           = 14,
	};
public:
	struct BuffInfo {
		BuffId buff_id;
		std::uint64_t duration;
		std::uint64_t time_begin;
		std::uint64_t time_end;
	};

public:
	static const std::initializer_list<AttributeId> COMBAT_ATTRIBUTES;

private:
	const boost::shared_ptr<MySql::Center_MapObject> m_obj;

	boost::container::flat_map<AttributeId,
		boost::shared_ptr<MySql::Center_MapObjectAttribute>> m_attributes;
	boost::container::flat_map<BuffId,
		boost::shared_ptr<MySql::Center_MapObjectBuff>> m_buffs;

	// 非持久化数据。
	std::uint64_t m_last_updated_time = 0;
	mutable std::uint64_t m_stamp = 0;

	unsigned m_action = 0;
	std::string m_action_param;

public:
	MapObject(MapObjectUuid map_object_uuid, MapObjectTypeId map_object_type_id, AccountUuid owner_uuid, MapObjectUuid parent_object_uuid,
		std::string name, Coord coord, std::uint64_t created_time, std::uint64_t expiry_time, bool garrisoned);
	MapObject(boost::shared_ptr<MySql::Center_MapObject> obj,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectAttribute>> &attributes,
		const std::vector<boost::shared_ptr<MySql::Center_MapObjectBuff>> &buffs);
	~MapObject();

protected:
	// 在世界地图上用于向客户端发送防御建筑的数据。
	virtual void synchronize_with_player_additional(const boost::shared_ptr<PlayerSession> &session) const;

public:
	virtual bool should_auto_update() const;
	virtual void pump_status();
	virtual void recalculate_attributes(bool recursive);

	virtual bool is_protectable() const;

	MapObjectUuid get_map_object_uuid() const;
	MapObjectTypeId get_map_object_type_id() const;
	AccountUuid get_owner_uuid() const;
	MapObjectUuid get_parent_object_uuid() const;

	const std::string &get_name() const;
	void set_name(std::string name);

	Coord get_coord() const;
	void set_coord(Coord coord) noexcept;
	void set_coord_no_synchronize(Coord coord) noexcept;

	std::uint64_t get_created_time() const;
	std::uint64_t get_expiry_time() const;

	bool has_been_deleted() const;
	void delete_from_game() noexcept;

	bool is_garrisoned() const;
	void set_garrisoned(bool garrisoned);

	bool is_idle() const; // 进驻，进入副本等。

	std::uint64_t get_stamp() const {
		return m_stamp;
	}

	std::int64_t get_attribute(AttributeId attribute_id) const;
	void get_attributes(boost::container::flat_map<AttributeId, std::int64_t> &ret) const;
	void set_attributes(boost::container::flat_map<AttributeId, std::int64_t> modifiers);

	BuffInfo get_buff(BuffId buff_id) const;
	bool is_buff_in_effect(BuffId buff_id) const;
	void get_buffs(std::vector<BuffInfo> &ret) const;
	void set_buff(BuffId buff_id, std::uint64_t duration);
	void set_buff(BuffId buff_id, std::uint64_t time_begin, std::uint64_t duration);
	void accumulate_buff(BuffId buff_id, std::uint64_t delta_duration);
	void accumulate_buff_hint(BuffId buff_id, std::uint64_t utc_now, std::uint64_t delta_duration);
	void clear_buff(BuffId buff_id) noexcept;

	std::uint64_t get_resource_amount_carriable() const;
	std::uint64_t get_resource_amount_carried() const;
	std::uint64_t load_resource(ResourceId resource_id, std::uint64_t amount_to_add, bool ignore_limit, bool use_alt_id);
	void unload_resources(const boost::shared_ptr<Castle> &castle);
	void discard_resources();

	unsigned get_action() const {
		return m_action;
	}
	const std::string &get_action_param() const {
		return m_action_param;
	}
	void set_action(unsigned action, std::string action_param);

	bool is_virtually_removed() const;
	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	void synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const;
};

}

#endif
