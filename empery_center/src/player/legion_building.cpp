#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../mysql/legion.hpp"
#include "../msg/cs_legion_building.hpp"
#include "../data/legion_building_config.hpp"
#include "../account.hpp"
#include "../legion.hpp"
#include "../legion_building.hpp"
#include "../singletons/legion_map.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../singletons/legion_building_map.hpp"
#include "../data/global.hpp"
#include "../data/legion_corps_power.hpp"
#include "../msg/err_legion.hpp"
#include "../legion_member_attribute_ids.hpp"
#include "../buff_ids.hpp"
#include "../resource_ids.hpp"
#include "../castle.hpp"
#include "../singletons/world_map.hpp"
#include "../msg/err_map.hpp"
#include "../msg/err_castle.hpp"
#include "../singletons/task_box_map.hpp"
#include "../legion_attribute_ids.hpp"
#include "../data/map_object_type.hpp"
#include "../data/map_defense.hpp"
#include "../data/castle.hpp"
#include "../reason_ids.hpp"
#include "../map_utilities_center.hpp"
#include "../resource_utilities.hpp"
#include "../map_utilities.hpp"
#include "../warehouse_building.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>


namespace EmperyCenter {


PLAYER_SERVLET(Msg::CS_GetLegionBuildingInfoMessage, account, session,  req )
{
    PROFILE_ME;

    LOG_EMPERY_CENTER_INFO("CS_GetLegionBuildingInfoMessage*******************",req);

	const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
           LegionBuildingMap::synchronize_with_player(member->get_legion_uuid(),session);

			return Response(Msg::ST_OK);
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

	return Response();
}

namespace {
	template<typename D, typename S>
	void copy_buff(const boost::shared_ptr<D> &dst, std::uint64_t utc_now, const boost::shared_ptr<S> &src, BuffId buff_id){
		PROFILE_ME;

		const auto info = src->get_buff(buff_id);
		if(utc_now < info.time_end){
			dst->set_buff(buff_id, info.time_begin, saturated_sub(info.time_end, info.time_begin));
		} else {
			dst->clear_buff(buff_id);
		}
	}
}


PLAYER_SERVLET(Msg::CS_CreateLegionBuildingMessage, account, session,  req )
{
    PROFILE_ME;

    const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
           // 查看member是否有权限
			const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_BUILDMINE))
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
			else
			{
                // 是否已经有该建筑物，以后可以扩展该建筑物的数量
                std::vector<boost::shared_ptr<LegionBuilding>> buildings;
                LegionBuildingMap::find_by_type(buildings,LegionUuid(member->get_legion_uuid()),req.map_object_type_id);
                if(buildings.size() > 1)
                {
                    return Response(Msg::ERR_LEGION_BUILDING_CREATE_LIMIT);
                }
                else
                {
                    // 检查所需消耗的资源、道具是否满足
                    const auto buildingingo = Data::LegionBuilding::get(0);
                    if(buildingingo)
                    {
                        // 查看消耗资源
                        for(auto it = buildingingo->need_resource.begin(); it != buildingingo->need_resource.end(); ++it)
                        {

                        }

                        // 满足条件，可以建造
                        const auto castle_uuid = MapObjectUuid(req.castle_uuid);
                        const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(castle_uuid));
                        if(!castle){
                            return Response(Msg::ERR_NO_SUCH_CASTLE) <<castle_uuid;
                        }
                        if(castle->get_owner_uuid() != account->get_account_uuid()){
                            return Response(Msg::ERR_NOT_CASTLE_OWNER) <<castle->get_owner_uuid();
                        }

                        const auto task_box = TaskBoxMap::require(account->get_account_uuid());

                        castle->pump_status();

                        const auto map_object_type_id = MapObjectTypeId(req.map_object_type_id);
                        const auto castle_level = castle->get_level();
                        const auto castle_upgrade_data = Data::CastleUpgradePrimary::require(castle_level);

                        const auto coord = Coord(req.coord_x, req.coord_y);
                        const auto distance = get_distance_of_coords(coord, castle->get_coord());
                        if(distance > castle_upgrade_data->max_map_cell_distance){
                            return Response(Msg::ERR_DEFENSE_BUILDING_IS_TOO_FAR_AWAY) <<distance;
                        }
                        const auto cluster = WorldMap::get_cluster(coord);
                        if(!cluster){
                            return CbppResponse(Msg::ERR_CLUSTER_CONNECTION_LOST);
                        }
                        const auto castle_cluster = WorldMap::get_cluster(castle->get_coord());
                        if(castle_cluster != cluster){
                            return CbppResponse(Msg::ERR_NOT_ON_THE_SAME_MAP_SERVER);
                        }
                        auto result = can_place_defense_building_at(coord);
                        if(result.first != Msg::ST_OK){
                            return std::move(result);
                        }


                        const auto duration = static_cast<std::uint64_t>(std::ceil(buildingingo->levelup_time * 60000.0 - 0.001));
                        const auto legion_building_uuid = LegionBuildingUuid(Poseidon::Uuid::random());
                        const auto defense_building_uuid = MapObjectUuid(Poseidon::Uuid::random());
                        const auto utc_now = Poseidon::get_utc_time();

                        const auto insuff_resource_id = try_decrement_resources(castle, task_box, { },
                            ReasonIds::ID_UPGRADE_BUILDING, map_object_type_id.get(), 0, 0,
                            [&]{

                                const auto defense_building = boost::make_shared<WarehouseBuilding>(defense_building_uuid, map_object_type_id,
                                    account->get_account_uuid(), castle_uuid, std::string(), coord, utc_now);
                                defense_building->pump_status();
                                copy_buff(defense_building, utc_now, castle, BuffIds::ID_CASTLE_PROTECTION_PREPARATION);
                                copy_buff(defense_building, utc_now, castle, BuffIds::ID_CASTLE_PROTECTION);
                                copy_buff(defense_building, utc_now, castle, BuffIds::ID_NOVICIATE_PROTECTION);
                                defense_building->create_mission(WarehouseBuilding::MIS_CONSTRUCT, duration, { });
                                WorldMap::insert_map_object(defense_building);

                                auto pair = LegionBuilding::async_create(legion_building_uuid,member->get_legion_uuid(),req.map_object_type_id);
                                Poseidon::JobDispatcher::yield(pair.first, true);

                                auto building = std::move(pair.second);
                                // 初始化军团建筑物的属性
                                building->InitAttributes(defense_building_uuid.str());

                                LegionBuildingMap::insert(building);

                                LOG_EMPERY_CENTER_DEBUG("Created defense building: defense_building_uuid = ", defense_building_uuid,
                                    ", map_object_type_id = ", map_object_type_id, ", account_uuid = ", account->get_account_uuid());
                            });
                        if(insuff_resource_id){
                            return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
                        }

                        return Response(Msg::ST_OK);

                    }
                    else
                    {
                        return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
                    }
                }

			}

			return Response(Msg::ST_OK);
		}
		else
		{
			return Response(Msg::ERR_LEGION_CANNOT_FIND);
		}
	}
	else
	{
		// 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
	}

    return Response();
}

PLAYER_SERVLET(Msg::CS_UpgradeLegionBuildingMessage, account, session,  req )
{
    PROFILE_ME;

    const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
            // 查看member是否有权限
			const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_LEVELUPMINE))
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
			else
			{
                // 查看是否存在该建筑物
                const auto legion_building = LegionBuildingMap::find(LegionBuildingUuid(req.legion_building_uuid));
                if(legion_building)
                {
                    // 查看当前是否正在升级中
                    const auto upgradetime = legion_building->get_attribute(LegionBuildingAttributeIds::ID_UPGRADE_TIME);
                    if(!upgradetime.empty() || upgradetime != Poseidon::EMPTY_STRING)
                    {
                        return Response(Msg::ERR_LEGION_BUILDING_UPGRADING);
                    }
                    else
                    {
                        const auto cur_level = legion_building->get_attribute(LegionBuildingAttributeIds::ID_LEVEL);
                        const auto buildingingo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(cur_level) + 1);
                        if(buildingingo)
                        {
                            // 查看消耗资源
                            // 修改军团资金
							boost::container::flat_map<LegionAttributeId, std::string> Attributes;

                            for(auto it = buildingingo->need.begin(); it != buildingingo->need.end(); ++it)
                            {
                                if(std::string(it->first) == "1915001")  // 军团等级
                                {
                                    const auto legion_level = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL));
                                    if(legion_level < it->second)
                                    {
                                        return Response(Msg::ERR_LEGION_BUILDING_UPGRADE_LIMIT);
                                    }
                                }
                            }

                            for(auto it = buildingingo->need_resource.begin(); it != buildingingo->need_resource.end(); ++it)
                            {
                                if(std::string(it->first) == "5500001")  // 军团资金
                                {
                                    const auto legion_momey = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
                                    if(legion_momey < it->second)
                                    {
                                        return Response(Msg::ERR_LEGION_BUILDING_UPGRADE_LACK);
                                    }
                                    else
                                    {
                                        Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY)) - it->second);
                                    }
                                }
                            }

                            // 满足条件，可以升级

                            const auto map_object_uuid = MapObjectUuid(legion_building->get_attribute(LegionBuildingAttributeIds::ID_MAPOBJECT_UUID));
                            const auto defense_building = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(map_object_uuid));
                            if(!defense_building){
                                return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
                            }
                            if(defense_building->get_owner_uuid() != account->get_account_uuid()){
                                return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<defense_building->get_owner_uuid();
                            }

                            const auto parent_object_uuid = defense_building->get_parent_object_uuid();
                            const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
                            if(!castle){
                                return Response(Msg::ERR_NO_SUCH_CASTLE) <<parent_object_uuid;
                            }

                            const auto task_box = TaskBoxMap::require(account->get_account_uuid());

                            defense_building->pump_status();

                            if(defense_building->get_mission() != WarehouseBuilding::MIS_NONE){
                                return Response(Msg::ERR_BUILDING_MISSION_CONFLICT);
                            }


                            const auto map_object_type_id = defense_building->get_map_object_type_id();

                        //    const auto duration = static_cast<std::uint64_t>(std::ceil(buildingingo->levelup_time * 60000.0 - 0.001));

                            const auto insuff_resource_id = try_decrement_resources(castle, task_box, { },
                                ReasonIds::ID_UPGRADE_BUILDING, map_object_type_id.get(), buildingingo->house_level, 0,
                                [&]{
                           //         defense_building->create_mission(WarehouseBuilding::Mission::MIS_UPGRADE, duration, { });

                                    // 更新消耗的军团属性
                                    legion->set_attributes(Attributes);

                                    // 更新建筑物属性
                                    boost::container::flat_map<LegionBuildingAttributeId, std::string> modifiers;

                                    const auto utc_now = Poseidon::get_utc_time();

                                    std::string strtime = boost::lexical_cast<std::string>(utc_now + 60 * 1000 * buildingingo->levelup_time);  // 升级时间

                                    strtime = strtime.substr(0,10);

                                    modifiers[LegionBuildingAttributeIds::ID_UPGRADE_TIME] = strtime;

                                    legion_building->set_attributes(std::move(modifiers));
                                    });
                            if(insuff_resource_id){
                                return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
                            }

                            return Response(Msg::ST_OK);
                        }
                        else
                        {
                            return Response(Msg::ERR_LEGION_BUILDING_UPGRADE_MAX);
                        }

                    }
                }
                else
                {
                    return Response(Msg::ERR_LEGION_BUILDING_CANNOT_FIND);
                }
			}

			return Response(Msg::ST_OK);
        }
        else
        {
            return Response(Msg::ERR_LEGION_CANNOT_FIND);
        }
    }
    else
    {
        // 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
    }

    return Response();
}


/*
PLAYER_SERVLET(Msg::CS_CancleUpgradeLegionBuildingMessage, account, session,  req )
{
    PROFILE_ME;

    const auto account_uuid = account->get_account_uuid();

	// 判断自己是否加入军团
	const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
	if(member)
	{
		// 检查军团是否存在
		const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
		if(legion)
		{
            // 查看member是否有权限
			const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_LEVELUPMINE))
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
			else
			{
                // 查看是否存在该建筑物
                const auto legion_building = LegionBuildingMap::find(LegionBuildingUuid(req.legion_building_uuid));
                if(legion_building)
                {
                        // 查看当前是否正在升级中
                        const auto upgradetime = legion_building->get_attribute(LegionBuildingAttributeIds::ID_UPGRADE_TIME);

                        if((!upgradetime.empty() || upgradetime != Poseidon::EMPTY_STRING))
                        {

                            const auto utc_now = Poseidon::get_utc_time();

                            const auto upgrdetime = boost::lexical_cast<boost::uint64_t>(upgradetime) * 1000;

                            if(utc_now > upgrdetime)
                            {
                                return Response(Msg::ERR_LEGION_BUILDING_NOTIN_UPGRADING);
                            }
                            else
                            {
                                const auto cur_level = legion_building->get_attribute(LegionBuildingAttributeIds::ID_LEVEL);
                                const auto buildingingo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(cur_level) + 1);
                                if(buildingingo)
                                {
                                    boost::container::flat_map<LegionAttributeId, std::string> Attributes;

                                    // 查看消耗资源 修改军团资金
                                    for(auto it = buildingingo->need_resource.begin(); it != buildingingo->need_resource.end(); ++it)
                                    {
                                        if(std::string(it->first) == "5500001")  // 军团资金
                                        {
                                            Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY)) + it->second * Data::Global::as_unsigned(Data::Global::SLOT_BUILDING_UPGRADE_CANCELLATION_REFUND_RATIO));
                                        }
                                    }
                                    // 更新消耗的军团属性
                                    legion->set_attributes(Attributes);

                                    // 更新建筑物属性
                                    boost::container::flat_map<LegionBuildingAttributeId, std::string> modifiers;
                                    modifiers[LegionBuildingAttributeIds::ID_UPGRADE_TIME] = "";
                                    legion_building->set_attributes(std::move(modifiers));

                                    return Response(Msg::ST_OK);
                                }
                            }
                        }
                        else
                        {
                            return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
                        }
                }
                else
                {
                    return Response(Msg::ERR_LEGION_BUILDING_CANNOT_FIND);
                }
			}
        }
        else
        {
            return Response(Msg::ERR_LEGION_CANNOT_FIND);
        }
    }
    else
    {
        // 没有加入军团
		return Response(Msg::ERR_LEGION_NOT_JOIN);
    }

    return Response();
}
*/

}
