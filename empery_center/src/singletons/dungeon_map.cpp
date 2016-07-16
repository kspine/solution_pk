#include "../precompiled.hpp"
#include "dungeon_map.hpp"
#include "../mmain.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../dungeon.hpp"
#include "../dungeon_session.hpp"
#include "account_map.hpp"

namespace EmperyCenter {

namespace {
	struct DungeonElement {
		boost::shared_ptr<Dungeon> dungeon;

		DungeonUuid dungeon_uuid;
		AccountUuid founder_uuid;
		std::uint64_t expiry_time;

		explicit DungeonElement(boost::shared_ptr<Dungeon> dungeon_)
			: dungeon(std::move(dungeon_))
			, dungeon_uuid(dungeon->get_dungeon_uuid()), founder_uuid(dungeon->get_founder_uuid())
			, expiry_time(dungeon->get_expiry_time())
		{
		}
	};

	MULTI_INDEX_MAP(DungeonContainer, DungeonElement,
		UNIQUE_MEMBER_INDEX(dungeon_uuid)
		MULTI_MEMBER_INDEX(founder_uuid)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<DungeonContainer> g_dungeon_map;

	void gc_timer_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Dungeon gc timer: now = ", now);

		const auto dungeon_map = g_dungeon_map.lock();
		if(!dungeon_map){
			return;
		}

		for(;;){
			const auto it = dungeon_map->begin<2>();
			if(it == dungeon_map->end<2>()){
				break;
			}
			if(now < it->expiry_time){
				break;
			}

			LOG_EMPERY_CENTER_DEBUG("Reclaiming dungeon: dungeon_uuid = ", it->dungeon_uuid);
			const auto dungeon = it->dungeon;
			dungeon_map->erase<2>(it);

			dungeon->clear();
		}
	}

	using ServerSet        = boost::container::flat_set<boost::weak_ptr<DungeonSession>>;
	boost::weak_ptr<ServerSet> g_servers;

	// RR scheduling
	using ServerRrVector = std::vector<boost::weak_ptr<DungeonSession>>;
	boost::weak_ptr<ServerRrVector> g_server_rr_vector;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto dungeon_map = boost::make_shared<DungeonContainer>();
		g_dungeon_map = dungeon_map;
		handles.push(dungeon_map);

		const auto servers = boost::make_shared<ServerSet>();
		g_servers = servers;
		handles.push(servers);

		const auto server_rr_vector = boost::make_shared<ServerRrVector>();
		g_server_rr_vector = server_rr_vector;
		handles.push(server_rr_vector);

		const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 300000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, gc_interval,
			std::bind(&gc_timer_proc, std::placeholders::_2));
		handles.push(timer);
	}
}

boost::shared_ptr<Dungeon> DungeonMap::get(DungeonUuid dungeon_uuid){
	PROFILE_ME;

	const auto dungeon_map = g_dungeon_map.lock();
	if(!dungeon_map){
		LOG_EMPERY_CENTER_WARNING("DungeonMap is not loaded.");
		return { };
	}

	const auto it = dungeon_map->find<0>(dungeon_uuid);
	if(it == dungeon_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Dungeon not found: dungeon_uuid = ", dungeon_uuid);
		return { };
	}
	return it->dungeon;
}
boost::shared_ptr<Dungeon> DungeonMap::require(DungeonUuid dungeon_uuid){
	PROFILE_ME;

	auto dungeon = get(dungeon_uuid);
	if(!dungeon){
		LOG_EMPERY_CENTER_WARNING("Dungeon not found: dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon not found"));
	}
	return dungeon;
}
void DungeonMap::get_by_founder(std::vector<boost::shared_ptr<Dungeon>> &ret, AccountUuid account_uuid){
	PROFILE_ME;

	const auto dungeon_map = g_dungeon_map.lock();
	if(!dungeon_map){
		LOG_EMPERY_CENTER_WARNING("DungeonMap is not loaded.");
		return;
	}

	const auto range = dungeon_map->equal_range<1>(account_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->dungeon);
	}
}

void DungeonMap::insert(const boost::shared_ptr<Dungeon> &dungeon){
	PROFILE_ME;

	const auto dungeon_map = g_dungeon_map.lock();
	if(!dungeon_map){
		LOG_EMPERY_CENTER_WARNING("DungeonMap is not loaded.");
		DEBUG_THROW(Exception, sslit("DungeonMap is not loaded"));
	}

	const auto dungeon_uuid = dungeon->get_dungeon_uuid();

	LOG_EMPERY_CENTER_DEBUG("Inserting dungeon: dungeon_uuid = ", dungeon_uuid);
	if(!dungeon_map->insert(DungeonElement(dungeon)).second){
		LOG_EMPERY_CENTER_WARNING("Dungeon already exists: dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon already exists"));
	}
}
void DungeonMap::update(const boost::shared_ptr<Dungeon> &dungeon, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_map = g_dungeon_map.lock();
	if(!dungeon_map){
		LOG_EMPERY_CENTER_WARNING("DungeonMap is not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("DungeonMap is not loaded"));
		}
		return;
	}

	const auto dungeon_uuid = dungeon->get_dungeon_uuid();

	const auto it = dungeon_map->find<0>(dungeon_uuid);
	if(it == dungeon_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Dungeon not found: dungeon_uuid = ", dungeon_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Dungeon not found"));
		}
		return;
	}
	if(it->dungeon != dungeon){
		LOG_EMPERY_CENTER_DEBUG("Dungeon expired: dungeon_uuid = ", dungeon_uuid);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating dungeon: dungeon_uuid = ", dungeon_uuid);
	dungeon_map->replace<0>(it, DungeonElement(dungeon));
}

boost::shared_ptr<DungeonSession> DungeonMap::pick_server(){
	PROFILE_ME;

	const auto server_rr_vector = g_server_rr_vector.lock();
	if(!server_rr_vector){
		LOG_EMPERY_CENTER_WARNING("Dungeon server RR vector has not been loaded?");
		return { };
	}

	if(server_rr_vector->empty()){
		const auto servers = g_servers.lock();
		if(!servers){
			LOG_EMPERY_CENTER_WARNING("Dungeon server set has not been loaded?");
			return { };
		}
		auto it = servers->begin();
		while(it != servers->end()){
			const auto server = it->lock();
			if(!server){
				it = servers->erase(it);
				continue;
			}
			server_rr_vector->emplace_back(server);
			++it;
		}
		LOG_EMPERY_CENTER_DEBUG("Collected dungeon servers: server_count = ", servers->size());
	}
	while(!server_rr_vector->empty()){
		auto server = server_rr_vector->back().lock();
		server_rr_vector->pop_back();
		if(server){
			return std::move(server);
		}
	}
	LOG_EMPERY_CENTER_WARNING("No dungeon server available.");
	return { };
}
void DungeonMap::add_server(const boost::shared_ptr<DungeonSession> &server){
	PROFILE_ME;

	const auto servers = g_servers.lock();
	if(!servers){
		LOG_EMPERY_CENTER_WARNING("Dungeon server set has not been loaded?");
		DEBUG_THROW(Exception, sslit("Dungeon server set has not been loaded"));
	}

	LOG_EMPERY_CENTER_INFO("Registering dungeon server: remote_info = ", server->get_remote_info());
	servers->insert(server);
}

}
