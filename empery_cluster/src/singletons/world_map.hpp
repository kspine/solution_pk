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
class ResourceCrate;
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
	static void get_map_objects_surrounding(std::vector<boost::shared_ptr<MapObject>> &ret,Coord origin,std::uint64_t radius);
	static void replace_map_object_no_synchronize(const boost::shared_ptr<ClusterClient> &master, const boost::shared_ptr<MapObject> &map_object);
	static void remove_map_object_no_synchronize(const boost::weak_ptr<ClusterClient> &master, MapObjectUuid map_object_uuid) noexcept;
	static void update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists = true);
	static void get_map_objects_by_rectangle(std::vector<boost::shared_ptr<MapObject>> &ret, Rectangle rectangle);

	// ResourceCrate
	static boost::shared_ptr<ResourceCrate> get_resource_crate(ResourceCrateUuid resource_crate_uuid);
	static void replace_resource_crate_no_synchronize(const boost::shared_ptr<ClusterClient> &master, const boost::shared_ptr<ResourceCrate> &resource_crate);
	static void remove_resource_crate_no_synchronize(const boost::weak_ptr<ClusterClient> & /* master */, ResourceCrateUuid resource_crate_uuid) noexcept ;
	static void update_resource_crate(const boost::shared_ptr<ResourceCrate> &resource_crate, bool throws_if_not_exists = true);
	static void get_resource_crates_by_rectangle(std::vector<boost::shared_ptr<ResourceCrate>> &ret, Rectangle rectangle);

	// ClusterClient
	static Rectangle get_cluster_scope(Coord coord);

	static boost::shared_ptr<ClusterClient> get_cluster(Coord coord);
	static void get_all_clusters(boost::container::flat_map<Coord, boost::shared_ptr<ClusterClient>> &ret);
	static void set_cluster(const boost::shared_ptr<ClusterClient> &cluster, Coord coord);

	// legion
	static void get_map_objects_by_legion_uuid(std::vector<boost::shared_ptr<MapObject>> &ret,LegionUuid legion_uuid);

private:
	WorldMap() = delete;
};

}

#endif
