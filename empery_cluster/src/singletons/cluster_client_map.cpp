#include "../precompiled.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/string.hpp>
#include <poseidon/file.hpp>
#include "../cluster_client.hpp"
/*
namespace EmperyCluster {

namespace {
	std::string     g_client_connect             = "127.0.0.1";
	unsigned        g_client_port                = 13217;
	bool            g_client_use_ssl             = false;
	boost::uint64_t g_client_keep_alive_interval = 15000;

	struct ClientElement {
		std::pair<boost::int64_t, boost::int64_t> map_coord;
		boost::int64_t map_x;
		boost::int64_t map_y;

		mutable boost::weak_ptr<ClusterClient> client;

		ClientElement(boost::int64_t map_x_, boost::int64_t map_y_)
			: map_coord(map_x_, map_y_), map_x(map_x_), map_y(map_y_)
		{
		}
	};

	MULTI_INDEX_MAP(ClientMapDelegator, ClientElement,
		UNIQUE_MEMBER_INDEX(map_coord)
		MULTI_MEMBER_INDEX(map_x)
		MULTI_MEMBER_INDEX(map_y)
	)

	boost::weak_ptr<ClientMapDelegator> g_client_map;

	MODULE_RAII(handles){
		get_config(g_client_connect,             "cluster_cbpp_client_connect");
		get_config(g_client_port,                "cluster_cbpp_client_port");
		get_config(g_client_use_ssl,             "cluster_cbpp_client_use_ssl");
		get_config(g_client_keep_alive_interval, "cluster_cbpp_keep_alive_interval");

		const auto client_map = boost::make_shared<ClientMapDelegator>();
		LOG_EMPERY_CLUSTER_INFO("Creating maps...");
		const auto map_serving_v = get_config_v<std::string>("map_serving");
		for(auto it = map_serving_v.begin(); it != map_serving_v.end(); ++it){
			LOG_EMPERY_CLUSTER_DEBUG("> Map serving: ", *it);
			auto parts = Poseidon::explode<std::string>(',', *it);

			auto map_x = boost::lexical_cast<boost::int64_t>(parts.at(0));
			auto map_y = boost::lexical_cast<boost::int64_t>(parts.at(1));
			auto map_file_path = std::move(parts.at(2));

			if(!client_map->insert(ClientElement(map_x, map_y)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate map coord: map_x = ", map_x, ", map_y = ", map_y);
				DEBUG_THROW(Exception, sslit("Duplicate map coord"));
			}
		}
		g_client_map = client_map;
		handles.push(client_map);

		const auto timer = Poseidon::TimerDaemon::register_timer(0, 1500, std::bind([]{
			PROFILE_ME;

			const auto client_map = g_client_map.lock();
			if(client_map){
				for(auto it = client_map->begin(); it != client_map->end(); ++it){
					if(!it->client.expired()){
						continue;
					}
					LOG_EMPERY_CLUSTER_INFO("Bringing up cluster client: map_x = ", it->map_x, ", map_y = ", it->map_y);
					auto client = boost::make_shared<ClusterClient>(
						Poseidon::IpPort(SharedNts(g_client_connect), g_client_port), g_client_use_ssl, g_client_keep_alive_interval,
						it->map_x, it->map_y);
					client->go_resident();
					it->client = client;
				}
			}
		}));
		LOG_EMPERY_CLUSTER_INFO("Creating client daemon timer...");
		handles.push(timer);
	}
}

}
*/
