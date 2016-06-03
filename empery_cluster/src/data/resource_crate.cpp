#include "../precompiled.hpp"
#include "resource_crate.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>

namespace EmperyCluster {

namespace {
	MULTI_INDEX_MAP(ResourceCrateMap, Data::ResourceCrate,
		UNIQUE_MEMBER_INDEX(resource_amount)
	)
	boost::weak_ptr<const ResourceCrateMap> g_resource_crate_map;
	const char RESOURCE_CRATE_FILE[] = "chest";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(RESOURCE_CRATE_FILE);
		const auto resource_crate_map         =  boost::make_shared<ResourceCrateMap>();
		while(csv.fetch_row()){
			Data::ResourceCrate elem = { };
			Poseidon::JsonArray array;
			csv.get(array, "resource_id");
			if(array.size() != 2){
				continue;
			}
			const auto resouce_id = boost::lexical_cast<ResourceId>(array.at(0));
			const auto amount = boost::lexical_cast<std::uint64_t>(array.at(1));
			elem.resource_amount = std::make_pair(resouce_id,amount);
			csv.get(elem.defence,                "chest_defense");
			csv.get(elem.health,                 "chest_health");
			csv.get(elem.defence_type,           "arm_def_type");
			if(!resource_crate_map->insert(std::move(elem)).second){
				LOG_EMPERY_CLUSTER_ERROR("Duplicate ResourceCrate: Resourceid = ", resouce_id," amount = ",amount);
				DEBUG_THROW(Exception, sslit("Duplicate ResourceCrate"));
			}
		}
		g_resource_crate_map = resource_crate_map;
		handles.push(resource_crate_map);
	}
}

namespace Data {
	boost::shared_ptr<const ResourceCrate> ResourceCrate::get(ResourceId resource_id,std::uint64_t amount){
		PROFILE_ME;

		const auto resource_crate_map = g_resource_crate_map.lock();
		if(!resource_crate_map){
			LOG_EMPERY_CLUSTER_WARNING("resource_crate_map has not been loaded.");
			return { };
		}

		auto it = resource_crate_map->upper_bound<0>(std::make_pair(resource_id,amount));
		if(it != resource_crate_map->begin<0>()){
			--it;
		}
		
		if(it == resource_crate_map->end<0>()){
			LOG_EMPERY_CLUSTER_DEBUG("resource_crate not found: resourceId = ", resource_id, " amount = ",amount);
			return { };
		}
		
		if(it->resource_amount.first != resource_id){
			LOG_EMPERY_CLUSTER_DEBUG("resource_crate not found: resourceId = ", resource_id, " amount = ",amount);
			return { };
		}
		return boost::shared_ptr<const ResourceCrate>(resource_crate_map, &*it);
	}

}

}
