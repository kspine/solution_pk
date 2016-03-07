#ifndef EMPERY_CLUSTER_SINGLETONS_WORLD_MAP_HPP_
#define EMPERY_CLUSTER_SINGLETONS_WORLD_MAP_HPP_

#include "../id_types.hpp"
#include "../coord.hpp"
#include "../rectangle.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCluster {

class MapCell;
class MapObject;
class ClusterClient;

struct WorldMap {
	// MapCell
	static boost::shared_ptr<MapCell> get_map_cell(Coord coord);
	static boost::shared_ptr<MapCell> require_map_cell(Coord coord);
	static void replace_map_cell_no_synchronize(const boost::shared_ptr<ClusterClient> &master, const boost::shared_ptr<MapCell> &map_cell);

	static void get_map_cells_by_rectangle(std::vector<boost::shared_ptr<MapCell>> &ret, Rectangle rectangle);

	// MapObject
	static boost::shared_ptr<MapObject> get_map_object(MapObjectUuid map_object_uuid);
	static void get_map_objects_by_account(std::vector<boost::shared_ptr<MapObject>> &ret,AccountUuid owner_uuid);
	static void replace_map_object_no_synchronize(const boost::shared_ptr<ClusterClient> &master, const boost::shared_ptr<MapObject> &map_object);
	static void remove_map_object_no_synchronize(const boost::weak_ptr<ClusterClient> &master, MapObjectUuid map_object_uuid) noexcept;
	static void update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists = true);
	static void remove_map_object(MapObjectUuid map_object_uuid) noexcept;

	static void get_map_objects_by_rectangle(std::vector<boost::shared_ptr<MapObject>> &ret, Rectangle rectangle);

	// ClusterClient
	static Rectangle get_cluster_scope(Coord coord);

	static boost::shared_ptr<ClusterClient> get_cluster(Coord coord);
	static void get_all_clusters(boost::container::flat_map<Coord, boost::shared_ptr<ClusterClient>> &ret);
	static void set_cluster(const boost::shared_ptr<ClusterClient> &cluster, Coord coord);

private:
	WorldMap() = delete;
};

}

#endif
