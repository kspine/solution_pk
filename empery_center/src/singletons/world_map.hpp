#ifndef EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include "../rectangle.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/function.hpp>
#include <vector>
#include <string>

namespace EmperyCenter {

class MapCell;
class MapObject;
class Overlay;
class StrategicResource;
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

//	static void get_all_map_cells(std::vector<boost::shared_ptr<MapCell>> &ret);
	static void get_map_cells_by_parent_object(std::vector<boost::shared_ptr<MapCell>> &ret, MapObjectUuid parent_object_uuid);
	static void get_map_cells_by_rectangle(std::vector<boost::shared_ptr<MapCell>> &ret, Rectangle rectangle);
	static void get_map_cells_by_occupier_object(std::vector<boost::shared_ptr<MapCell>> &ret, MapObjectUuid occupier_object_uuid);

	// MapObject
	static boost::shared_ptr<MapObject> get_map_object(MapObjectUuid map_object_uuid);
	static void insert_map_object(const boost::shared_ptr<MapObject> &map_object);
	static void update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists = true);

	static void get_all_map_objects(std::vector<boost::shared_ptr<MapObject>> &ret);
	static void get_map_objects_by_owner(std::vector<boost::shared_ptr<MapObject>> &ret, AccountUuid owner_uuid);
	static void get_map_objects_by_parent_object(std::vector<boost::shared_ptr<MapObject>> &ret, MapObjectUuid parent_object_uuid);
	static void get_map_objects_by_rectangle(std::vector<boost::shared_ptr<MapObject>> &ret, Rectangle rectangle);
	static MapObjectUuid get_primary_castle_uuid(AccountUuid owner_uuid);

	// Overlay
	static boost::shared_ptr<Overlay> get_overlay(Coord cluster_coord, const std::string &overlay_group_name);
	static boost::shared_ptr<Overlay> require_overlay(Coord cluster_coord, const std::string &overlay_group_name);
	static void insert_overlay(const boost::shared_ptr<Overlay> &overlay);
	static void update_overlay(const boost::shared_ptr<Overlay> &overlay, bool throws_if_not_exists = true);

	static void get_overlays_by_rectangle(std::vector<boost::shared_ptr<Overlay>> &ret, Rectangle rectangle);

	// StrategicResource
	static boost::shared_ptr<StrategicResource> get_strategic_resource(Coord coord);
	static boost::shared_ptr<StrategicResource> require_strategic_resource(Coord coord);
	static void insert_strategic_resource(const boost::shared_ptr<StrategicResource> &strategic_resource);
	static void update_strategic_resource(const boost::shared_ptr<StrategicResource> &strategic_resource, bool throws_if_not_exists = true);

	static void get_strategic_resources_by_rectangle(std::vector<boost::shared_ptr<StrategicResource>> &ret, Rectangle rectangle);

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
	static void get_all_clusters(boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> &ret);
	static void set_cluster(const boost::shared_ptr<ClusterSession> &cluster, Coord coord);
	static void synchronize_cluster(const boost::shared_ptr<ClusterSession> &cluster, Rectangle view) noexcept;

	// 出生点
	// 限定在指定小地图内。
	static boost::shared_ptr<Castle> create_init_castle_restricted(
		const boost::function<boost::shared_ptr<Castle> (Coord)> &factory, Coord coord_hint);
	// 如果指定小地图放满，就放别处。
	static boost::shared_ptr<Castle> create_init_castle(
		const boost::function<boost::shared_ptr<Castle> (Coord)> &factory, Coord coord_hint);

private:
	WorldMap() = delete;
};

}

#endif
