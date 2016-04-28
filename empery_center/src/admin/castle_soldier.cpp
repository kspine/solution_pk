#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/err_castle.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

ADMIN_SERVLET("castle_soldier/get_all", root, session, params){
	const auto map_object_uuid = MapObjectUuid(params.at("map_object_uuid"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	std::vector<Castle::SoldierInfo> soldiers;
	castle->get_all_soldiers(soldiers);

	Poseidon::JsonObject elem_object;
	for(auto it = soldiers.begin(); it != soldiers.end(); ++it){
		const auto map_object_type_id = it->map_object_type_id;
		const auto soldier_count = it->count;
		char str[64];
		unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)map_object_type_id.get());
		elem_object[SharedNts(str, len)] = soldier_count;
	}
	root[sslit("soldiers")] = std::move(elem_object);

	return Response();
}

ADMIN_SERVLET("castle_soldier/add", root, session, params){
	const auto map_object_uuid    = MapObjectUuid(params.at("map_object_uuid"));
	const auto map_object_type_id = boost::lexical_cast<MapObjectTypeId>(params.at("map_object_type_id"));
	const auto count_to_add       = boost::lexical_cast<std::uint64_t>(params.at("count_to_add"));
	const auto param1             = boost::lexical_cast<std::uint64_t>(params.at("param1"));
	const auto param2             = boost::lexical_cast<std::uint64_t>(params.at("param2"));
	const auto param3             = boost::lexical_cast<std::uint64_t>(params.at("param3"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	std::vector<SoldierTransactionElement> transaction;
	const auto operation = SoldierTransactionElement::OP_ADD;
	transaction.emplace_back(operation, map_object_type_id, count_to_add,
		ReasonIds::ID_ADMIN_OPERATION, param1, param2, param3);
	castle->commit_soldier_transaction(transaction);

	return Response();
}

ADMIN_SERVLET("castle_soldier/remove", root, session, params){
	const auto map_object_uuid    = MapObjectUuid(params.at("map_object_uuid"));
	const auto map_object_type_id = boost::lexical_cast<MapObjectTypeId>(params.at("map_object_type_id"));
	const auto count_to_remove    = boost::lexical_cast<std::uint64_t>(params.at("count_to_remove"));
	const auto saturated          = params.get("saturated").empty() == false;
	const auto param1             = boost::lexical_cast<std::uint64_t>(params.at("param1"));
	const auto param2             = boost::lexical_cast<std::uint64_t>(params.at("param2"));
	const auto param3             = boost::lexical_cast<std::uint64_t>(params.at("param3"));

	const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(map_object_uuid));
	if(!castle){
		return Response(Msg::ERR_NO_SUCH_CASTLE) <<map_object_uuid;
	}

	std::vector<SoldierTransactionElement> transaction;
	const auto operation = saturated ? SoldierTransactionElement::OP_REMOVE_SATURATED : SoldierTransactionElement::OP_REMOVE;
	transaction.emplace_back(operation, map_object_type_id, count_to_remove,
		ReasonIds::ID_ADMIN_OPERATION, param1, param2, param3);
	const auto insuff_map_object_type_id = castle->commit_soldier_transaction_nothrow(transaction);
	if(insuff_map_object_type_id){
		return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_map_object_type_id;
	}

	return Response();
}

}
