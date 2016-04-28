#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_castle.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("castle_resource/get_all", root, session, params){
	const auto map_object_uuid = MapObjectUuid(params.at("map_object_uuid"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	std::vector<Castle::ResourceInfo> resources;
	castle->get_all_resources(resources);

	Poseidon::JsonObject elem_object;
	for(auto it = resources.begin(); it != resources.end(); ++it){
		const auto resource_id = it->resource_id;
		const auto resource_amount = it->amount;
		char str[64];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)resource_id.get());
		elem_object[SharedNts(str, len)] = resource_amount;
	}
	root[sslit("resources")] = std::move(elem_object);

	return Response();
}

ADMIN_SERVLET("castle_resource/add", root, session, params){
	const auto map_object_uuid  = MapObjectUuid(params.at("map_object_uuid"));
	const auto resource_id      = boost::lexical_cast<ResourceId>   (params.at("resource_id"));
	const auto amount_to_add    = boost::lexical_cast<std::uint64_t>(params.at("amount_to_add"));
	const auto param1           = boost::lexical_cast<std::uint64_t>(params.at("param1"));
	const auto param2           = boost::lexical_cast<std::uint64_t>(params.at("param2"));
	const auto param3           = boost::lexical_cast<std::uint64_t>(params.at("param3"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	std::vector<ResourceTransactionElement> transaction;
	const auto operation = ResourceTransactionElement::OP_ADD;
	transaction.emplace_back(operation, resource_id, amount_to_add,
		ReasonIds::ID_ADMIN_OPERATION, param1, param2, param3);
	castle->commit_resource_transaction(transaction);

	return Response();
}

ADMIN_SERVLET("castle_resource/remove", root, session, params){
	const auto map_object_uuid  = MapObjectUuid(params.at("map_object_uuid"));
	const auto resource_id      = boost::lexical_cast<ResourceId>   (params.at("resource_id"));
	const auto amount_to_remove = boost::lexical_cast<std::uint64_t>(params.at("amount_to_remove"));
	const auto saturated        = params.get("saturated").empty() == false;
	const auto param1           = boost::lexical_cast<std::uint64_t>(params.at("param1"));
	const auto param2           = boost::lexical_cast<std::uint64_t>(params.at("param2"));
	const auto param3           = boost::lexical_cast<std::uint64_t>(params.at("param3"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	std::vector<ResourceTransactionElement> transaction;
	const auto operation = saturated ? ResourceTransactionElement::OP_REMOVE_SATURATED : ResourceTransactionElement::OP_REMOVE;
	transaction.emplace_back(operation, resource_id, amount_to_remove,
		ReasonIds::ID_ADMIN_OPERATION, param1, param2, param3);
	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction);
	if(insuff_resource_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

}
