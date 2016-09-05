#include "../precompiled.hpp"
#include "world_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/json.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include "../map_utilities.hpp"
#include "../../../empery_center/src/msg/ks_map.hpp"
#include "../coord.hpp"
#include "../map_cell.hpp"
#include "../map_object.hpp"
#include "../resource_crate.hpp"
#include "../rectangle.hpp"
#include "../cluster_client.hpp"
#include "../data/global.hpp"
#include "../data/map.hpp"

namespace EmperyCluster {

namespace Msg = ::EmperyCenter::Msg;

namespace {
	struct MapCellElement {
		boost::shared_ptr<MapCell> map_cell;

		boost::weak_ptr<ClusterClient> master;
		Coord coord;
		MapObjectUuid parent_object_uuid;

		MapCellElement(boost::shared_ptr<MapCell> map_cell_, boost::weak_ptr<ClusterClient> master_)
			: map_cell(std::move(map_cell_))
			, master(std::move(master_))
			, coord(map_cell->get_coord()), parent_object_uuid(map_cell->get_parent_object_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(MapCellContainer, MapCellElement,
		MULTI_MEMBER_INDEX(master)
		UNIQUE_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(parent_object_uuid)
	)

	boost::weak_ptr<MapCellContainer> g_map_cell_map;

	struct MapObjectElement {
		boost::shared_ptr<MapObject> map_object;

		boost::weak_ptr<ClusterClient> master;
		MapObjectUuid map_object_uuid;
		Coord coord;
		AccountUuid owner_uuid;
		LegionUuid legion_uuid;

		MapObjectElement(boost::shared_ptr<MapObject> map_object_, boost::weak_ptr<ClusterClient> master_)
			: map_object(std::move(map_object_))
			, master(std::move(master_))
			, map_object_uuid(map_object->get_map_object_uuid()), coord(map_object->get_coord()), owner_uuid(map_object->get_owner_uuid())
			,legion_uuid(map_object->get_legion_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(MapObjectContainer, MapObjectElement,
		MULTI_MEMBER_INDEX(master)
		UNIQUE_MEMBER_INDEX(map_object_uuid)
		MULTI_MEMBER_INDEX(coord)
		MULTI_MEMBER_INDEX(owner_uuid)
		MULTI_MEMBER_INDEX(legion_uuid)
	)

	boost::weak_ptr<MapObjectContainer> g_map_object_map;
	struct ResourceCrateElement {
		boost::shared_ptr<ResourceCrate> resource_crate;

		boost::weak_ptr<ClusterClient> master;
		ResourceCrateUuid resource_crate_uuid;
		Coord coord;

		explicit ResourceCrateElement(boost::shared_ptr<ResourceCrate> resource_crate_,boost::weak_ptr<ClusterClient> master_)
			: resource_crate(std::move(resource_crate_))
			, master(std::move(master_))
			, resource_crate_uuid(resource_crate->get_resource_crate_uuid()), coord(resource_crate->get_coord())
		{
		}
	};

	MULTI_INDEX_MAP(ResourceCrateContainer, ResourceCrateElement,
		MULTI_MEMBER_INDEX(master)
		UNIQUE_MEMBER_INDEX(resource_crate_uuid)
		MULTI_MEMBER_INDEX(coord)
	)

	boost::weak_ptr<ResourceCrateContainer> g_resource_crate_map;

	enum : unsigned {
		MAP_WIDTH          = 600,
		MAP_HEIGHT         = 640,
	};

	struct ClusterElement {
		Coord cluster_coord;
		boost::weak_ptr<ClusterClient> cluster;

		ClusterElement(Coord cluster_coord_, boost::weak_ptr<ClusterClient> cluster_)
			: cluster_coord(cluster_coord_), cluster(std::move(cluster_))
		{
		}
	};

	MULTI_INDEX_MAP(ClusterContainer, ClusterElement,
		UNIQUE_MEMBER_INDEX(cluster_coord)
		UNIQUE_MEMBER_INDEX(cluster)
	)

	boost::weak_ptr<ClusterContainer> g_cluster_map;

	void gc_timer_proc(std::uint64_t /* now */){
		PROFILE_ME;

		const auto map_cell_map = g_map_cell_map.lock();
		if(map_cell_map){
			auto it = map_cell_map->begin<0>();
			while(it != map_cell_map->end<0>()){
				const auto prev = it;
				it = map_cell_map->upper_bound<0>(prev->master);

				if(prev->master.expired()){
					LOG_EMPERY_CLUSTER_WARNING("Master is gone! Removing ", std::distance(prev, it), " map cell(s).");
					map_cell_map->erase<0>(prev, it);
				}
			}
		}

		const auto map_object_map = g_map_object_map.lock();
		if(map_object_map){
			auto it = map_object_map->begin<0>();
			while(it != map_object_map->end<0>()){
				const auto prev = it;
				it = map_object_map->upper_bound<0>(prev->master);

				if(prev->master.expired()){
					LOG_EMPERY_CLUSTER_WARNING("Master is gone! Removing ", std::distance(prev, it), " map object(s).");
					map_object_map->erase<0>(prev, it);
				}
			}
		}
	}

	struct InitServerElement {
		std::pair<std::int64_t, std::int64_t> num_coord;
		std::string name;
		std::uint64_t created_time;

		mutable boost::weak_ptr<ClusterClient> cluster;

		InitServerElement(std::int64_t num_x_, std::int64_t num_y_,
			std::string name_, std::uint64_t created_time_)
			: num_coord(num_x_, num_y_)
			, name(std::move(name_)), created_time(created_time_)
		{
		}
	};

	MULTI_INDEX_MAP(InitServerMap, InitServerElement,
		UNIQUE_MEMBER_INDEX(num_coord)
		MULTI_MEMBER_INDEX(name)
	)

	boost::weak_ptr<InitServerMap> g_init_server_map;

	void cluster_server_reconnect_timer_proc(std::uint64_t /* now */){
		PROFILE_ME;

		const auto init_server_map = g_init_server_map.lock();
		if(init_server_map){
			for(auto it = init_server_map->begin(); it != init_server_map->end(); ++it){
				auto cluster = it->cluster.lock();
				if(!cluster){
					const auto num_x = it->num_coord.first;
					const auto num_y = it->num_coord.second;
					const auto &name = it->name;
					const auto created_time = it->created_time;
					LOG_EMPERY_CLUSTER_INFO("Creating cluster client: num_x = ", num_x, ", num_y = ", num_y, ", name = ", name);
					cluster = ClusterClient::create(num_x, num_y, name, created_time);
					it->cluster = cluster;
					break; // 每次只连一个。
				}
			}
		}
	}

	inline Coord get_cluster_coord_from_world_coord(Coord coord){
		const auto mask_x = (coord.x() >= 0) ? 0 : -1;
		const auto mask_y = (coord.y() >= 0) ? 0 : -1;

		const auto cluster_x = ((coord.x() ^ mask_x) / MAP_WIDTH  ^ mask_x) * MAP_WIDTH;
		const auto cluster_y = ((coord.y() ^ mask_y) / MAP_HEIGHT ^ mask_y) * MAP_HEIGHT;

		return Coord(cluster_x, cluster_y);
	}

	MODULE_RAII_PRIORITY(handles, 5300){
		const auto map_cell_map = boost::make_shared<MapCellContainer>();
		g_map_cell_map = map_cell_map;
		handles.push(map_cell_map);

		const auto map_object_map = boost::make_shared<MapObjectContainer>();
		g_map_object_map = map_object_map;
		handles.push(map_object_map);

		const auto resource_crate_map = boost::make_shared<ResourceCrateContainer>();
		g_resource_crate_map = resource_crate_map;
		handles.push(resource_crate_map);

		const auto cluster_map = boost::make_shared<ClusterContainer>();
		g_cluster_map = cluster_map;
		handles.push(cluster_map);

		const auto gc_timer_interval = get_config<std::uint64_t>("world_map_gc_timer_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_timer_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(std::move(timer));

		const auto init_server_map = boost::make_shared<InitServerMap>();
		LOG_EMPERY_CLUSTER_INFO("Loading logical map servers...");
		const auto init_logical_map_servers = get_config_v<std::string>("init_logical_map_server");
		for(auto it = init_logical_map_servers.begin(); it != init_logical_map_servers.end(); ++it){
			const auto temp_array = boost::lexical_cast<Poseidon::JsonArray>(*it);
			const auto num_x = static_cast<std::int64_t>(temp_array.at(0).get<double>());
			const auto num_y = static_cast<std::int64_t>(temp_array.at(1).get<double>());
			const auto &name = temp_array.at(2).get<std::string>();
			const auto created_time = Poseidon::scan_time(temp_array.at(3).get<std::string>().c_str());
			if(!init_server_map->insert(InitServerElement(num_x, num_y, name, created_time)).second){
				LOG_EMPERY_CLUSTER_ERROR("Map server conflict: num_x = ", num_x, ", num_y = ", num_y, ", name = ", name);
				DEBUG_THROW(Exception, sslit("Map server conflict"));
			}
			LOG_EMPERY_CLUSTER_INFO("Init map server: num_x = ", num_x, ", num_y = ", num_y, ", created_time = ", created_time);
		}
		g_init_server_map = init_server_map;
		handles.push(std::move(init_server_map));

		const auto client_timer_interval = get_config<std::uint64_t>("cluster_client_reconnect_delay", 5000);
		timer = Poseidon::TimerDaemon::register_timer(0, client_timer_interval,
			std::bind(&cluster_server_reconnect_timer_proc, std::placeholders::_2));
		handles.push(std::move(timer));
	}

	void notify_cluster_map_object_updated(const boost::shared_ptr<MapObject> &map_object, const boost::shared_ptr<ClusterClient> &cluster){
		PROFILE_ME;

		boost::container::flat_map<AttributeId, std::int64_t> attributes;
		map_object->get_attributes(attributes);

		Msg::KS_MapUpdateMapObjectAction msg;
		msg.map_object_uuid = map_object->get_map_object_uuid().str();
		msg.stamp           = map_object->get_stamp();
		msg.x               = map_object->get_coord().x();
		msg.y               = map_object->get_coord().y();
		msg.action          = map_object->get_action();
		msg.param           = map_object->get_action_param();
		cluster->send(msg);
	}
}

boost::shared_ptr<MapCell> WorldMap::get_map_cell(Coord coord){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CLUSTER_WARNING("Map cell map not loaded.");
		return { };
	}

	const auto it = map_cell_map->find<1>(coord);
	if(it == map_cell_map->end<1>()){
		LOG_EMPERY_CLUSTER_TRACE("Map cell not found: coord = ", coord);
		return { };
	}
	if(it->master.expired()){
		LOG_EMPERY_CLUSTER_DEBUG("Master expired: coord = ", coord);
		return { };
	}
	return it->map_cell;
}
boost::shared_ptr<MapCell> WorldMap::require_map_cell(Coord coord){
	PROFILE_ME;

	auto ret = get_map_cell(coord);
	if(!ret){
		DEBUG_THROW(Exception, sslit("Map cell not found"));
	}
	return ret;
}
void WorldMap::replace_map_cell_no_synchronize(const boost::shared_ptr<ClusterClient> &master, const boost::shared_ptr<MapCell> &map_cell){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CLUSTER_WARNING("Map cell map not loaded.");
		DEBUG_THROW(Exception, sslit("Map cell map not loaded"));
	}

	const auto result = map_cell_map->insert(MapCellElement(map_cell, master));
	if(!result.second){
		map_cell_map->replace(result.first, MapCellElement(map_cell, master));
	}
}

void WorldMap::get_map_cells_by_rectangle(std::vector<boost::shared_ptr<MapCell>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto map_cell_map = g_map_cell_map.lock();
	if(!map_cell_map){
		LOG_EMPERY_CLUSTER_WARNING("Map cell map not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = map_cell_map->lower_bound<1>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == map_cell_map->end<1>()){
				goto _exit_while;
			}
			if(it->coord.x() != x){
				x = it->coord.x();
				break;
			}
			if(it->coord.y() >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->map_cell);
			++it;
		}
	}
_exit_while:
	;
}

boost::shared_ptr<MapObject> WorldMap::get_map_object(MapObjectUuid map_object_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
		return { };
	}

	const auto it = map_object_map->find<1>(map_object_uuid);
	if(it == map_object_map->end<1>()){
		LOG_EMPERY_CLUSTER_TRACE("Map object not found: map_object_uuid = ", map_object_uuid);
		return { };
	}
	if(it->master.expired()){
		LOG_EMPERY_CLUSTER_DEBUG("Master expired: map_object_uuid = ", map_object_uuid);
		return { };
	}
	return it->map_object;
}

void WorldMap::get_map_objects_by_account(std::vector<boost::shared_ptr<MapObject>> &ret,AccountUuid owner_uuid){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
	}
	const auto range = map_object_map->equal_range<3>(owner_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_object);
	}
}

void WorldMap::get_map_objects_surrounding(std::vector<boost::shared_ptr<MapObject>> &ret,Coord origin,std::uint64_t radius){
	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
		return;
	}
	std::vector<Coord> surrounding;
	for(std::uint64_t i = 0; i <= radius; i++){
		get_surrounding_coords(surrounding,origin,i);
		for(auto it = surrounding.begin(); it != surrounding.end(); ++it){
			const auto coord = *it;
			const auto it_map_object = map_object_map->find<2>(coord);
			if(it_map_object != map_object_map->end<2>()){
				ret.emplace_back(it_map_object->map_object);
			}
		}
	}
}

void WorldMap::replace_map_object_no_synchronize(const boost::shared_ptr<ClusterClient> &master, const boost::shared_ptr<MapObject> &map_object){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
		DEBUG_THROW(Exception, sslit("Map object map not loaded"));
	}

	const auto result = map_object_map->insert(MapObjectElement(map_object, master));
	if(!result.second){
		map_object_map->replace(result.first, MapObjectElement(map_object, master));
	}
}
void WorldMap::remove_map_object_no_synchronize(const boost::weak_ptr<ClusterClient> & /* master */, MapObjectUuid map_object_uuid) noexcept {
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
		return;
	}

	const auto it = map_object_map->find<1>(map_object_uuid);
	if(it == map_object_map->end<1>()){
		LOG_EMPERY_CLUSTER_TRACE("Map object not found: map_object_uuid = ", map_object_uuid);
		return;
	}
	const auto map_object = it->map_object;
	const auto old_coord  = it->coord;

	LOG_EMPERY_CLUSTER_TRACE("Removing map object: map_object_uuid = ", map_object_uuid, ", old_coord = ", old_coord);
	map_object_map->erase<1>(it);
}
void WorldMap::update_map_object(const boost::shared_ptr<MapObject> &map_object, bool throws_if_not_exists){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object map not loaded"));
		}
		return;
	}
	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CLUSTER_WARNING("Cluster map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Cluster object map not loaded"));
		}
		return;
	}

	const auto map_object_uuid = map_object->get_map_object_uuid();

	const auto it = map_object_map->find<1>(map_object_uuid);
	if(it == map_object_map->end<1>()){
		LOG_EMPERY_CLUSTER_WARNING("Map object not found: map_object_uuid = ", map_object_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Map object not found"));
		}
		return;
	}
	if(it->map_object != map_object){
		LOG_EMPERY_CLUSTER_DEBUG("Map object expired: map_object_uuid = ", map_object_uuid);
		return;
	}
	const auto old_coord = it->coord;
	const auto new_coord = map_object->get_coord();

	const auto cit = cluster_map->find<1>(it->master);
	if(cit == cluster_map->end<1>()){
		LOG_EMPERY_CLUSTER_DEBUG("Master expired? map_object_uuid = ", map_object_uuid);
		return;
	}
	const auto scope = get_cluster_scope(cit->cluster_coord);
	if(!scope.hit_test(new_coord)){
		LOG_EMPERY_CLUSTER_DEBUG("Map object is out of the scope of its master: map_object_uuid = ", map_object_uuid);
		return;
	}

	LOG_EMPERY_CLUSTER_TRACE("Updating map object: map_object_uuid = ", map_object_uuid,
		", old_coord = ", old_coord, ", new_coord = ", new_coord);
	map_object_map->replace<1>(it, MapObjectElement(map_object, it->master));

	const auto old_cluster = get_cluster(old_coord);
	if(old_cluster){
		try {
			notify_cluster_map_object_updated(map_object, old_cluster);
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
			old_cluster->shutdown(e.what());
		}
	}
}

void WorldMap::get_map_objects_by_rectangle(std::vector<boost::shared_ptr<MapObject>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = map_object_map->lower_bound<2>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == map_object_map->end<2>()){
				goto _exit_while;
			}
			if(it->coord.x() != x){
				x = it->coord.x();
				break;
			}
			if(it->coord.y() >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->map_object);
			++it;
		}
	}
_exit_while:
	;
}

boost::shared_ptr<ResourceCrate> WorldMap::get_resource_crate(ResourceCrateUuid resource_crate_uuid){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CLUSTER_WARNING("Resource crate map not loaded.");
		return { };
	}

	const auto it = resource_crate_map->find<1>(resource_crate_uuid);
	if(it == resource_crate_map->end<1>()){
		LOG_EMPERY_CLUSTER_TRACE("Resource crate not found: resource_crate_uuid = ", resource_crate_uuid);
		return { };
	}
	if(it->master.expired()){
		LOG_EMPERY_CLUSTER_DEBUG("Master expired: resource_crate_uuid = ", resource_crate_uuid);
		return { };
	}
	return it->resource_crate;
}

void WorldMap::replace_resource_crate_no_synchronize(const boost::shared_ptr<ClusterClient> &master, const boost::shared_ptr<ResourceCrate> &resource_crate){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CLUSTER_WARNING("Resource crate map not loaded.");
		DEBUG_THROW(Exception, sslit("Resource crate map not loaded."));
	}

	const auto result = resource_crate_map->insert(ResourceCrateElement(resource_crate, master));
	if(!result.second){
		resource_crate_map->replace(result.first, ResourceCrateElement(resource_crate, master));
	}
}

void WorldMap::remove_resource_crate_no_synchronize(const boost::weak_ptr<ClusterClient> & /* master */, ResourceCrateUuid resource_crate_uuid) noexcept {
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CLUSTER_WARNING("Resource crate map not loaded.");
		return;
	}

	const auto it = resource_crate_map->find<1>(resource_crate_uuid);
	if(it == resource_crate_map->end<1>()){
		LOG_EMPERY_CLUSTER_TRACE("Resource crate not found: resource_crate_uuid = ", resource_crate_uuid);
		return;
	}
	const auto resource_crate = it->resource_crate;
	const auto old_coord  = it->coord;

	LOG_EMPERY_CLUSTER_TRACE("Removing Resource crate: resource_crate_uuid = ", resource_crate_uuid, ", old_coord = ", old_coord);
	resource_crate_map->erase<1>(it);
}

void WorldMap::update_resource_crate(const boost::shared_ptr<ResourceCrate> &resource_crate, bool throws_if_not_exists){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CLUSTER_WARNING("Resource crate map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Resource crate map not loaded"));
		}
		return;
	}
	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CLUSTER_WARNING("Cluster map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Cluster object map not loaded"));
		}
		return;
	}

	const auto resource_crate_uuid = resource_crate->get_resource_crate_uuid();

	const auto it = resource_crate_map->find<1>(resource_crate_uuid);
	if(it == resource_crate_map->end<1>()){
		LOG_EMPERY_CLUSTER_WARNING("Resource crate not found: resource_crate_uuid = ", resource_crate_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Resource crate not found"));
		}
		return;
	}
	const auto old_coord = it->coord;
	const auto new_coord = resource_crate->get_coord();

	bool out_of_scope = true;
	const auto cit = cluster_map->find<1>(it->master);
	if(cit != cluster_map->end<1>()){
		const auto scope = get_cluster_scope(cit->cluster_coord);
		if(scope.hit_test(new_coord)){
			out_of_scope = false;
		}
	}
	if(out_of_scope){
		LOG_EMPERY_CLUSTER_DEBUG("Resource crate is out of the scope of its master: resource_crate_uuid = ", resource_crate_uuid);
	} else {
		LOG_EMPERY_CLUSTER_TRACE("Updating resource crate: resource_crate_uuid = ", resource_crate_uuid,
			", old_coord = ", old_coord, ", new_coord = ", new_coord);
		resource_crate_map->replace<1>(it, ResourceCrateElement(resource_crate, it->master));
	}

	const auto old_cluster = get_cluster(old_coord);
	if(old_cluster){
		try {
			//notify_cluster_map_object_updated(map_object, old_cluster);
		} catch(std::exception &e){
			LOG_EMPERY_CLUSTER_WARNING("std::exception thrown: what = ", e.what());
			old_cluster->shutdown(e.what());
		}
	}
}

void WorldMap::get_resource_crates_by_rectangle(std::vector<boost::shared_ptr<ResourceCrate>> &ret, Rectangle rectangle){
	PROFILE_ME;

	const auto resource_crate_map = g_resource_crate_map.lock();
	if(!resource_crate_map){
		LOG_EMPERY_CLUSTER_WARNING("Resource crate map not loaded.");
		return;
	}

	auto x = rectangle.left();
	while(x < rectangle.right()){
		auto it = resource_crate_map->lower_bound<2>(Coord(x, rectangle.bottom()));
		for(;;){
			if(it == resource_crate_map->end<2>()){
				goto _exit_while;
			}
			if(it->coord.x() != x){
				x = it->coord.x();
				break;
			}
			if(it->coord.y() >= rectangle.top()){
				++x;
				break;
			}
			ret.emplace_back(it->resource_crate);
			++it;
		}
	}
_exit_while:
	;
}

Rectangle WorldMap::get_cluster_scope(Coord coord){
	PROFILE_ME;

	return Rectangle(get_cluster_coord_from_world_coord(coord), MAP_WIDTH, MAP_HEIGHT);
}

boost::shared_ptr<ClusterClient> WorldMap::get_cluster(Coord coord){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CLUSTER_DEBUG("Cluster map not loaded.");
		return { };
	}

	const auto cluster_coord = get_cluster_coord_from_world_coord(coord);
	const auto it = cluster_map->find<0>(cluster_coord);
	if(it == cluster_map->end<0>()){
		LOG_EMPERY_CLUSTER_DEBUG("Cluster not found: coord = ", coord, ", cluster_coord = ", cluster_coord);
		return { };
	}
	auto cluster = it->cluster.lock();
	if(!cluster){
		LOG_EMPERY_CLUSTER_DEBUG("Cluster expired: coord = ", coord, ", cluster_coord = ", cluster_coord);
		cluster_map->erase<0>(it);
		return { };
	}
	return std::move(cluster);
}
void WorldMap::get_all_clusters(boost::container::flat_map<Coord, boost::shared_ptr<ClusterClient>> &ret){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CLUSTER_WARNING("Cluster map not loaded.");
		return;
	}

	ret.reserve(ret.size() + cluster_map->size());
	for(auto it = cluster_map->begin(); it != cluster_map->end(); ++it){
		auto cluster = it->cluster.lock();
		if(!cluster){
			continue;
		}
		ret.emplace(it->cluster_coord, std::move(cluster));
	}
}
void WorldMap::set_cluster(const boost::shared_ptr<ClusterClient> &cluster, Coord coord){
	PROFILE_ME;

	const auto cluster_map = g_cluster_map.lock();
	if(!cluster_map){
		LOG_EMPERY_CLUSTER_WARNING("Cluster map not loaded.");
		DEBUG_THROW(Exception, sslit("Cluster map not loaded"));
	}

	const auto cit = cluster_map->find<1>(cluster);
	if(cit != cluster_map->end<1>()){
		LOG_EMPERY_CLUSTER_WARNING("Cluster already registered: old_cluster_coord = ", cit->cluster_coord);
		DEBUG_THROW(Exception, sslit("Cluster already registered"));
	}

	const auto cluster_coord = get_cluster_coord_from_world_coord(coord);
	LOG_EMPERY_CLUSTER_INFO("Setting up cluster client: cluster_coord = ", cluster_coord);
	const auto result = cluster_map->insert(ClusterElement(cluster_coord, cluster));
	if(!result.second){
		if(!result.first->cluster.expired()){
			LOG_EMPERY_CLUSTER_WARNING("Cluster server conflict: cluster_coord = ", cluster_coord);
			DEBUG_THROW(Exception, sslit("Cluster server conflict"));
		}
		cluster_map->replace(result.first, ClusterElement(cluster_coord, cluster));
	}
}

void WorldMap::get_map_objects_by_legion_uuid(std::vector<boost::shared_ptr<MapObject>> &ret,LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto map_object_map = g_map_object_map.lock();
	if(!map_object_map){
		LOG_EMPERY_CLUSTER_WARNING("Map object map not loaded.");
	}
	const auto range = map_object_map->equal_range<4>(legion_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->map_object);
	}
}

}
