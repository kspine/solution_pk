#include "../precompiled.hpp"
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/string.hpp>
#include <poseidon/file.hpp>
#include "../cluster_client.hpp"

namespace EmperyCluster {

namespace {
	std::string     g_clientConnect           = "127.0.0.1";
	unsigned        g_clientPort              = 13217;
	bool            g_clientUseSsl            = false;
	boost::uint64_t g_clientKeepAliveInterval = 15000;

	struct ClientElement {
		std::pair<boost::int64_t, boost::int64_t> mapCoord;
		boost::int64_t mapX;
		boost::int64_t mapY;

		mutable boost::weak_ptr<ClusterClient> client;

		ClientElement(boost::int64_t mapX_, boost::int64_t mapY_)
			: mapCoord(mapX_, mapY_), mapX(mapX_), mapY(mapY_)
		{
		}
	};

	MULTI_INDEX_MAP(ClientMapDelegator, ClientElement,
		UNIQUE_MEMBER_INDEX(mapCoord)
		MULTI_MEMBER_INDEX(mapX)
		MULTI_MEMBER_INDEX(mapY)
	)

	boost::weak_ptr<ClientMapDelegator> g_clientMap;

	MODULE_RAII(handles){
		getConfig(g_clientConnect,           "cluster_cbpp_client_connect");
		getConfig(g_clientPort,              "cluster_cbpp_client_port");
		getConfig(g_clientUseSsl,            "cluster_cbpp_client_use_ssl");
		getConfig(g_clientKeepAliveInterval, "cluster_cbpp_keep_alive_interval");

		const auto clientMap = boost::make_shared<ClientMapDelegator>();
		LOG_EMPERY_CLUSTER_INFO("Creating maps...");
		const auto mapServingV = getConfigV<std::string>("map_serving");
		for(auto it = mapServingV.begin(); it != mapServingV.end(); ++it){
			LOG_EMPERY_CLUSTER_DEBUG("> Map serving: ", *it);
			auto parts = Poseidon::explode<std::string>(',', *it);

			auto mapX = boost::lexical_cast<boost::int64_t>(parts.at(0));
			auto mapY = boost::lexical_cast<boost::int64_t>(parts.at(1));
			auto mapFilePath = std::move(parts.at(2));

			if(!clientMap->insert(ClientElement(mapX, mapY)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate map coord: mapX = ", mapX, ", mapY = ", mapY);
				DEBUG_THROW(Exception, sslit("Duplicate map coord"));
			}
		}
		g_clientMap = clientMap;
		handles.push(clientMap);

		const auto timer = Poseidon::TimerDaemon::registerTimer(0, 1500, std::bind([]{
			PROFILE_ME;

			const auto clientMap = g_clientMap.lock();
			if(clientMap){
				for(auto it = clientMap->begin(); it != clientMap->end(); ++it){
					if(!it->client.expired()){
						continue;
					}
					LOG_EMPERY_CLUSTER_INFO("Bringing up cluster client: mapX = ", it->mapX, ", mapY = ", it->mapY);
					auto client = boost::make_shared<ClusterClient>(
						Poseidon::IpPort(SharedNts(g_clientConnect), g_clientPort), g_clientUseSsl, g_clientKeepAliveInterval,
						it->mapX, it->mapY);
					client->goResident();
					it->client = client;
				}
			}
		}));
		LOG_EMPERY_CLUSTER_INFO("Creating client daemon timer...");
		handles.push(timer);
	}
}

}
