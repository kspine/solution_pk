#ifndef EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_

#include "../id_types.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>
#include <string>
#include "../coord.hpp"
#include "../rectangle.hpp"

namespace EmperyCenter {

class MapCell;
class MapObject;
class StrategicResource;
class MapEventBlock;
class ResourceCrate;
class ClusterSession;
class PlayerSession;
class Castle;

struct WorldMap {
	// MapCell
	static boost::shared_ptr<MapCell> get_map_cell(Coord coord);
	static boost::shared_ptr<MapCell> require_map_cell(Coord coord);
	static void insert_map_cell(const boost::shared_ptr<MapCell> &map_cell);
	static void update_map_cell(const boost::shared_ptr<MapCell> &map_cell, bool throws_if_not_exists = true);

//	static void get_map_cells_all(std::vector<boost::shared_ptr<MapCell>> &ret);
	static void get_map_cells_by_parent_object(std::vector<boost::shared_ptr<MapCell>> &ret, MapObjectUuid parent_object_uuid);
	static void get_map_cells_by_rectangle(std::vector<boost::shared_ptr<MapCell>> &ret, Rectangle rectangle);
	static void get_map_cells_by_occupier_object(std::vector<boost::shared_ptr<MapCell>> &ret, MapObjectUuid occupier_object_uuid);

	// MapObject
	static boost::shared_ptr<MapObject> get_map_object(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<MapObject> get_or_reload_map_object(MapObjectUuid map_object_uuid);
	static boost::shared_ptr<MapObject> forced_reload_map_object(MapObjectUuid map_object_uuid);
	static void insert_map_object(const boost::shared_ptr<MapObject> &map_object);
	static void update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists = true);

	static void get_map_objects_all(std::vector<boost::shared_ptr<MapObject>> &ret);
	static void get_map_objects_by_owner(std::vector<boost::shared_ptr<MapObject>> &ret, AccountUuid owner_uuid);
	static void forced_reload_map_objects_by_owner(AccountUuid owner_uuid);
	static void get_map_objects_by_parent_object(std::vector<boost::shared_ptr<MapObject>> &ret, MapObjectUuid parent_object_uuid);
	static void forced_reload_map_objects_by_parent_object(MapObjectUuid parent_object_uuid);
	static boost::shared_ptr<MapObject> get_map_object_by_garrisoning_object(MapObjectUuid garrisoning_object_uuid);
	static void get_map_objects_by_rectangle(std::vector<boost::shared_ptr<MapObject>> &ret, Rectangle rectangle);

	static boost::shared_ptr<Castle> get_primary_castle(AccountUuid owner_uuid);
	static boost::shared_ptr<Castle> require_primary_castle(AccountUuid owner_uuid);

	// StrategicResource
	static boost::shared_ptr<StrategicResource> get_strategic_resource(Coord coord);
	static boost::shared_ptr<StrategicResource> require_strategic_resource(Coord coord);
	static void insert_strategic_resource(const boost::shared_ptr<StrategicResource> &strategic_resource);
	static void update_strategic_resource(const boost::shared_ptr<StrategicResource> &strategic_resource, bool throws_if_not_exists = true);

	static void get_strategic_resources_by_rectangle(std::vector<boost::shared_ptr<StrategicResource>> &ret, Rectangle rectangle);

	// MapEventBlock
	static boost::shared_ptr<MapEventBlock> get_map_event_block(Coord coord);
	static void get_cluster_map_event_blocks(Coord cluster_coord, std::vector<boost::shared_ptr<MapEventBlock>> &ret);
	static boost::shared_ptr<MapEventBlock> require_map_event_block(Coord coord);
	static void insert_map_event_block(const boost::shared_ptr<MapEventBlock> &map_event_block);
	static void update_map_event_block(const boost::shared_ptr<MapEventBlock> &map_event_block, bool throws_if_not_exists = true);
	static void refresh_activity_event(unsigned map_event_type);
	static void remove_activity_event(unsigned map_event_type);
	static void refresh_world_activity_event(Coord cluster_coord,unsigned map_event_type);
	static void remove_world_activity_event(Coord cluster_coord, unsigned map_event_type);
	static void refresh_world_activity_boss(Coord cluster_coord, std::uint64_t since);
	static void remove_world_activity_boss(Coord cluster_coord,std::uint64_t since);

	// ResourceCrate
	static boost::shared_ptr<ResourceCrate> get_resource_crate(ResourceCrateUuid resource_crate_uuid);
	static boost::shared_ptr<ResourceCrate> require_resource_crate(ResourceCrateUuid resource_crate_uuid);
	static void insert_resource_crate(const boost::shared_ptr<ResourceCrate> &resource_crate);
	static void update_resource_crate(const boost::shared_ptr<ResourceCrate> &resource_crate, bool throws_if_not_exists = true);

	static void get_resource_crates_by_rectangle(std::vector<boost::shared_ptr<ResourceCrate>> &ret, Rectangle rectangle);

	// PlayerSession
	static void get_players_viewing_rectangle(std::vector<boost::shared_ptr<PlayerSession>> &ret, Rectangle rectangle);
	static void update_player_view(const boost::shared_ptr<PlayerSession> &session);
	static void synchronize_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view) noexcept;

	// ClusterSession
	static Rectangle get_cluster_scope(Coord coord);

	static boost::shared_ptr<ClusterSession> get_cluster(Coord coord);
	static void get_clusters_all(std::vector<std::pair<Coord, boost::shared_ptr<ClusterSession>>> &ret);
	static void set_cluster(const boost::shared_ptr<ClusterSession> &cluster, Coord coord);
	static void forced_reload_cluster(Coord coord);
	static void synchronize_cluster(const boost::shared_ptr<ClusterSession> &cluster, Rectangle view) noexcept;
	//
	static void synchronize_account_map_object_all(AccountUuid account_uuid) noexcept;

	// 出生点
	// 限定在指定小地图内。
	static boost::shared_ptr<Castle> place_castle_random_restricted(
		const boost::function<boost::shared_ptr<Castle> (Coord)> &factory, Coord coord_hint);
	// 如果指定小地图放满，就放别处。
	static boost::shared_ptr<Castle> place_castle_random(
		const boost::function<boost::shared_ptr<Castle> (Coord)> &factory);

private:
	WorldMap() = delete;
};

}

#endif
