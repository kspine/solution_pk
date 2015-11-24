#include "../precompiled.hpp"
#include "world_map.hpp"
#include <poseidon/multi_index_map.hpp>

namespace EmperyCluster {

boost::shared_ptr<MapObject> WorldMap::get_map_object(MapObjectUuid map_object_uuid){
	return { };
}
void WorldMap::replace_map_object(const boost::shared_ptr<MapObject> &map_object){
}
void WorldMap::update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists){
}
void WorldMap::remove_map_object(MapObjectUuid map_object_uuid) noexcept {
}

boost::shared_ptr<ClusterClient> WorldMap::get_cluster(Coord coord){
	return { };
}
void WorldMap::get_all_clusters(std::vector<std::pair<Rectangle, boost::shared_ptr<ClusterClient>>> &ret){
}
Rectangle WorldMap::get_cluster_scope(const boost::weak_ptr<ClusterClient> &cluster){
	return {0,0,0,0};
}
void WorldMap::set_cluster(const boost::shared_ptr<ClusterClient> &cluster, Rectangle scope){
}

}
