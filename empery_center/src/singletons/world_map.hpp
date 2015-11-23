#ifndef EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_WORLD_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include "../rectangle.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCenter {

class MapCell;
class MapObject;
class ClusterSession;
class PlayerSession;

struct WorldMap {
	// MapCell
	static boost::shared_ptr<MapCell> get_map_cell(Coord coord);
	static void insert_map_cell(const boost::shared_ptr<MapCell> &map_cell);
	static void update_map_cell(const boost::shared_ptr<MapCell> &map_cell, bool throws_if_not_exists = true);

	static void get_map_cells_by_parent_object(std::vector<boost::shared_ptr<MapCell>> &ret, MapObjectUuid parent_object_uuid);
	static void get_map_cells_by_rectangle(boost::container::flat_map<Coord, boost::shared_ptr<MapCell>> &ret, Rectangle rectangle);

	// MapObject
	static boost::shared_ptr<MapObject> get_map_object(MapObjectUuid map_object_uuid);
	static void insert_map_object(const boost::shared_ptr<MapObject> &map_object);
	static void update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists = true);
	static void remove_map_object(MapObjectUuid map_object_uuid) noexcept;

	static void get_map_objects_by_owner(std::vector<boost::shared_ptr<MapObject>> &ret, AccountUuid owner_uuid);
	static void get_map_objects_by_parent_object(std::vector<boost::shared_ptr<MapObject>> &ret, MapObjectUuid parent_object_uuid);
	static void get_map_objects_by_rectangle(boost::container::flat_map<Coord, boost::shared_ptr<MapObject>> &ret, Rectangle rectangle);

	// PlayerSession
	static void get_players_viewing_coord(std::vector<boost::shared_ptr<PlayerSession>> &ret, Coord coord);
	static void set_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view);
	static void synchronize_player_view(const boost::shared_ptr<PlayerSession> &session, Rectangle view) noexcept;

	// ClusterSession
	static boost::shared_ptr<ClusterSession> get_cluster(Coord coord);
	static void get_all_clusters(boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> &ret);
	static void set_cluster(const boost::shared_ptr<ClusterSession> &cluster, Coord coord);
	static void synchronize_cluster(const boost::shared_ptr<ClusterSession> &cluster, Coord coord) noexcept;

	static Rectangle get_cluster_range(Coord coord);

private:
	WorldMap() = delete;
};

}

#endif
