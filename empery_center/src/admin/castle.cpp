#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_account.hpp"
#include "../msg/err_castle.hpp"
#include "../singletons/account_map.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../map_object_type_ids.hpp"

namespace EmperyCenter {

namespace {
	void fill_castle_object(Poseidon::JsonObject &object, const boost::shared_ptr<Castle> &castle){
		PROFILE_ME;

		object[sslit("map_object_uuid")]         = castle->get_map_object_uuid().str();
		object[sslit("owner_uuid")]              = castle->get_owner_uuid().str();
		object[sslit("parent_object_uuid")]      = castle->get_parent_object_uuid().str();
		object[sslit("name")]                    = castle->get_name();
		object[sslit("coord")]                   = boost::lexical_cast<std::string>(castle->get_coord());
		object[sslit("created_time")]            = castle->get_created_time();
		object[sslit("is_garrisoned")]           = castle->is_garrisoned();

		Poseidon::JsonObject elem_attributes;
		boost::container::flat_map<AttributeId, std::int64_t> attributes;
		castle->get_attributes(attributes);
		for(auto it = attributes.begin(); it != attributes.end(); ++it){
			char str[64];
			unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)it->first.get());
			elem_attributes[SharedNts(str, len)] = it->second;
		}
		object[sslit("attributes")]              = std::move(elem_attributes);
	}
}

ADMIN_SERVLET("castle/get", root, session, params){
	const auto map_object_uuid = MapObjectUuid(params.at("map_object_uuid"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	castle->pump_status();

	Poseidon::JsonObject object;
	fill_castle_object(object, castle);
	root[sslit("castle")] = std::move(object);

	return Response();
}

ADMIN_SERVLET("castle/get_by_owner", root, session, params){
	const auto account_uuid = AccountUuid(params.at("account_uuid"));

	const auto account = AccountMap::get(account_uuid);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<account_uuid;
	}

	Poseidon::JsonArray elem_castles;
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_owner(map_objects, account_uuid);
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		const auto map_object_type_id = map_object->get_map_object_type_id();
		if(map_object_type_id != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto castle = boost::dynamic_pointer_cast<Castle>(map_object);
		if(!castle){
			continue;
		}
		castle->pump_status();

		Poseidon::JsonObject object;
		fill_castle_object(object, castle);
		elem_castles.emplace_back(std::move(object));
	}
	root[sslit("castles")] = std::move(elem_castles);

	return Response();
}

}
