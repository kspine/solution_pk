#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_castle.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("castle_tech/get_all", root, session, params){
	const auto map_object_uuid = MapObjectUuid(params.at("map_object_uuid"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	castle->pump_status();

	std::vector<Castle::TechInfo> techs;
	castle->get_techs_all(techs);

	Poseidon::JsonObject elem_object;
	for(auto it = techs.begin(); it != techs.end(); ++it){
		Poseidon::JsonObject tech_object;
		tech_object[sslit("tech_level")]         = it->tech_level;
		tech_object[sslit("mission")]            = static_cast<unsigned>(it->mission);
		tech_object[sslit("mission_duration")]   = it->mission_duration;
		tech_object[sslit("mission_time_begin")] = it->mission_time_begin;
		tech_object[sslit("mission_time_end")]   = it->mission_time_end;
		char str[64];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->tech_id.get());
		elem_object[SharedNts(str, len)] = std::move(tech_object);
	}
	root[sslit("techs")] = std::move(elem_object);

	return Response();
}

ADMIN_SERVLET("castle_tech/set", root, session, params){
	const auto map_object_uuid  = MapObjectUuid(params.at("map_object_uuid"));
	const auto tech_id = boost::lexical_cast<TechId>(params.at("tech_id"));
	const auto tech_level   = boost::lexical_cast<unsigned>(params.at("tech_level"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	castle->forced_replace_tech(tech_id, tech_level);

	castle->pump_status();

	return Response();
}

}
