#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_castle.hpp"
#include "../msg/cerr_castle.hpp"
#include "../singletons/map_object_map.hpp"
#include "../castle.hpp"
#include "../data/castle.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

namespace {
	inline boost::shared_ptr<Castle> require_castle(AccountUuid account_uuid, MapObjectUuid map_object_uuid){
		PROFILE_ME;

		const auto castle = boost::dynamic_pointer_cast<Castle>(MapObjectMap::get(map_object_uuid));
		if(!castle){
			DEBUG_THROW(Poseidon::Cbpp::Exception, Msg::CERR_NO_SUCH_CASTLE, SharedNts(map_object_uuid.str()));
		}
		if(castle->get_owner_uuid() != account_uuid){
			DEBUG_THROW(Poseidon::Cbpp::Exception, Msg::CERR_NOT_CASTLE_OWNER, SharedNts(castle->get_owner_uuid().str()));
		}
		return castle;
	}
}

PLAYER_SERVLET(Msg::CS_CastleQueryInfo, account_uuid, session, req){
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_status(true);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCreateBuilding, account_uuid, session, req){
	const auto building_base_id = BuildingBaseId(req.building_base_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_ANOTHER_BUILDING_THERE) <<building_base_id;
	}

	const auto building_id = BuildingId(req.building_id);
	const auto building_data = Data::CastleBuilding::get(building_id);
	if(!building_data){
		return Response(Msg::CERR_NO_SUCH_BUILDING) <<building_id;
	}
	const auto count = castle->count_buildings_by_id(building_id);
	if(count >= building_data->build_limit){
		return Response(Msg::CERR_BUILD_LIMIT_EXCEEDED) <<building_id;
	}

	const auto building_base_data = Data::CastleBuildingBase::get(building_base_id);
	if(!building_base_data){
		return Response(Msg::CERR_NO_SUCH_CASTLE_BASE) <<building_base_id;
	}
	if(building_base_data->buildings_allowed.find(building_data->building_id) == building_base_data->buildings_allowed.end()){
		return Response(Msg::CERR_BUILDING_NOT_ALLOWED) <<building_id;
	}

	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, 1);
	for(auto it = upgrade_data->prerequisite.begin(); it != upgrade_data->prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::CERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = upgrade_data->upgrade_cost.begin(); it != upgrade_data->upgrade_cost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0);
	}
	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction.data(), transaction.size(),
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_CONSTRUCT, building_data->building_id); });
	if(insuff_resource_id){
		return Response(Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelBuildingMission, account_uuid, session, req){
	const auto building_base_id = BuildingBaseId(req.building_base_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_BUILDING_MISSION) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, info.building_level + 1);
	std::vector<Castle::ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refund_ratio = get_config<double>("castle_cancellation_refund_ratio", 0.5);
		for(auto it = upgrade_data->upgrade_cost.begin(); it != upgrade_data->upgrade_cost.end(); ++it){
			transaction.emplace_back(Castle::ResourceTransactionElement::OP_ADD,
				it->first, static_cast<boost::uint64_t>(std::floor(it->second * refund_ratio)),
				ReasonIds::ID_CANCEL_UPGRADE_BUILDING, info.building_id.get(), info.building_level, 0);
		}
	}
	castle->commit_resource_transaction(transaction.data(), transaction.size(),
		[&]{ castle->cancel_building_mission(building_base_id); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeBuilding, account_uuid, session, req){
	const auto building_base_id = BuildingBaseId(req.building_base_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.building_id == BuildingId()){
		return Response(Msg::CERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_BUILDING_MISSION_CONFLICT) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	const auto upgrade_data = Data::CastleUpgradeAbstract::get(building_data->type, info.building_level + 1);
	if(!upgrade_data){
		return Response(Msg::CERR_BUILDING_UPGRADE_MAX) <<info.building_id;
	}
	for(auto it = upgrade_data->prerequisite.begin(); it != upgrade_data->prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: building_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::CERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = upgrade_data->upgrade_cost.begin(); it != upgrade_data->upgrade_cost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_BUILDING, building_data->building_id.get(), upgrade_data->building_level, 0);
	}
	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction.data(), transaction.size(),
		[&]{ castle->create_building_mission(building_base_id, Castle::MIS_UPGRADE, { }); });
	if(insuff_resource_id){
		return Response(Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleDestroyBuilding, account_uuid, session, req){
	const auto building_base_id = BuildingBaseId(req.building_base_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.building_id == BuildingId()){
		return Response(Msg::CERR_NO_BUILDING_THERE) <<building_base_id;
	}
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_BUILDING_MISSION_CONFLICT) <<building_base_id;
	}

	const auto building_data = Data::CastleBuilding::require(info.building_id);
	const auto upgrade_data = Data::CastleUpgradeAbstract::require(building_data->type, info.building_level);
	if(upgrade_data->destruct_duration == 0){
		return Response(Msg::CERR_BUILDING_NOT_DESTRUCTIBLE) <<info.building_id;
	}

	castle->create_building_mission(building_base_id, Castle::MIS_DESTRUCT, BuildingId());

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteBuildingImmediately, account_uuid, session, req){
	const auto building_base_id = BuildingBaseId(req.building_base_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_building_status(building_base_id);

	const auto info = castle->get_building_base(building_base_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_BUILDING_MISSION) <<building_base_id;
	}

	// TODO 计算消耗。
	castle->speed_up_building_mission(building_base_id, UINT64_MAX);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualBuildingInfo, account_uuid, session, req){
	const auto building_base_id = BuildingBaseId(req.building_base_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_building_status(building_base_id, true);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleUpgradeTech, account_uuid, session, req){
	const auto tech_id = TechId(req.tech_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission != Castle::MIS_NONE){
		return Response(Msg::CERR_TECH_MISSION_CONFLICT) <<tech_id;
	}

	const auto tech_data = Data::CastleTech::get(TechId(req.tech_id), info.tech_level + 1);
	if(!tech_data){
		if(info.tech_level == 0){
			return Response(Msg::CERR_NO_SUCH_TECH) <<tech_id;
		}
		return Response(Msg::CERR_TECH_UPGRADE_MAX) <<tech_id;
	}

	for(auto it = tech_data->prerequisite.begin(); it != tech_data->prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Prerequisite not met: tech_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::CERR_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	for(auto it = tech_data->display_prerequisite.begin(); it != tech_data->display_prerequisite.end(); ++it){
		std::vector<Castle::BuildingBaseInfo> prerequisite_buildings;
		castle->get_buildings_by_id(prerequisite_buildings, it->first);
		unsigned max_level = 0;
		for(auto prereq_it = prerequisite_buildings.begin(); prereq_it != prerequisite_buildings.end(); ++prereq_it){
			if(max_level < prereq_it->building_level){
				max_level = prereq_it->building_level;
			}
		}
		if(max_level < it->second){
			LOG_EMPERY_CENTER_DEBUG("Display prerequisite not met: tech_id = ", it->first,
				", level_required = ", it->second, ", max_level = ", max_level);
			return Response(Msg::CERR_DISPLAY_PREREQUISITE_NOT_MET) <<it->first;
		}
	}
	std::vector<Castle::ResourceTransactionElement> transaction;
	for(auto it = tech_data->upgrade_cost.begin(); it != tech_data->upgrade_cost.end(); ++it){
		transaction.emplace_back(Castle::ResourceTransactionElement::OP_REMOVE, it->first, it->second,
			ReasonIds::ID_UPGRADE_TECH, tech_data->tech_id_level.first.get(), tech_data->tech_id_level.second, 0);
	}
	const auto insuff_resource_id = castle->commit_resource_transaction_nothrow(transaction.data(), transaction.size(),
		[&]{ castle->create_tech_mission(tech_id, Castle::MIS_UPGRADE); });
	if(insuff_resource_id){
		return Response(Msg::CERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCancelTechMission, account_uuid, session, req){
	const auto tech_id = TechId(req.tech_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_TECH_MISSION) <<tech_id;
	}

	const auto tech_data = Data::CastleTech::require(info.tech_id, info.tech_level + 1);
	std::vector<Castle::ResourceTransactionElement> transaction;
	if((info.mission == Castle::MIS_CONSTRUCT) || (info.mission == Castle::MIS_UPGRADE)){
		const auto refund_ratio = get_config<double>("castle_cancellation_refund_ratio", 0.5);
		for(auto it = tech_data->upgrade_cost.begin(); it != tech_data->upgrade_cost.end(); ++it){
			transaction.emplace_back(Castle::ResourceTransactionElement::OP_ADD,
				it->first, static_cast<boost::uint64_t>(std::floor(it->second * refund_ratio)),
				ReasonIds::ID_CANCEL_UPGRADE_TECH, info.tech_id.get(), info.tech_level, 0);
		}
	}
	castle->commit_resource_transaction(transaction.data(), transaction.size(),
		[&]{ castle->cancel_tech_mission(tech_id); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleCompleteTechImmediately, account_uuid, session, req){
	const auto tech_id = TechId(req.tech_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_tech_status(tech_id);

	const auto info = castle->get_tech(tech_id);
	if(info.mission == Castle::MIS_NONE){
		return Response(Msg::CERR_NO_TECH_MISSION) <<tech_id;
	}

	// TODO 计算消耗。
	castle->speed_up_tech_mission(tech_id, UINT64_MAX);

	return Response();
}

PLAYER_SERVLET(Msg::CS_CastleQueryIndividualTechInfo, account_uuid, session, req){
	const auto tech_id = TechId(req.tech_id);
	const auto castle = require_castle(account_uuid, MapObjectUuid(req.map_object_uuid));
	castle->pump_tech_status(tech_id, true);

	return Response();
}

}
