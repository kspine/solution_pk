#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_castle.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("castle_building_base/get_all", root, session, params){
	const auto map_object_uuid = MapObjectUuid(params.at("map_object_uuid"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	castle->pump_status();

	std::vector<Castle::BuildingBaseInfo> building_bases;
	castle->get_all_building_bases(building_bases);

	Poseidon::JsonObject elem_object;
	for(auto it = building_bases.begin(); it != building_bases.end(); ++it){
		Poseidon::JsonObject building_object;
		building_object[sslit("building_id")]        = it->building_id.get();
		building_object[sslit("building_level")]     = it->building_level;
		building_object[sslit("mission")]            = static_cast<unsigned>(it->mission);
		building_object[sslit("mission_duration")]   = it->mission_duration;
		building_object[sslit("mission_time_begin")] = it->mission_time_begin;
		building_object[sslit("mission_time_end")]   = it->mission_time_end;
		char str[64];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->building_base_id.get());
		elem_object[SharedNts(str, len)] = std::move(building_object);
	}
	root[sslit("building_bases")] = std::move(elem_object);

	return Response();
}

ADMIN_SERVLET("castle_building_base/set", root, session, params){
	const auto map_object_uuid  = MapObjectUuid(params.at("map_object_uuid"));
	const auto building_base_id = boost::lexical_cast<BuildingBaseId>(params.at("building_base_id"));
	const auto building_id      = boost::lexical_cast<BuildingId>(params.at("building_id"));
	const auto building_level   = boost::lexical_cast<unsigned>(params.at("building_level"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	castle->pump_status();

	castle->forced_replace_building(building_base_id, building_id, building_level);

	return Response();
}

}
