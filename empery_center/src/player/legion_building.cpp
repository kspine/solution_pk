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
#include "../attribute_ids.hpp"
#include "../data/legion_corps_level.hpp"
#include "../msg/sc_legion.hpp"
#include "../legion_log.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include "../legion_log.hpp"


namespace EmperyCenter {


PLAYER_SERVLET(Msg::CS_GetLegionBuildingInfoMessage, account, session,  req )
{
    PROFILE_ME;

    LOG_EMPERY_CENTER_INFO("CS_GetLegionBuildingInfoMessage*******************");

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
                if(buildings.size() >= 1)
                {
                    return Response(Msg::ERR_LEGION_BUILDING_CREATE_LIMIT);
                }
                else
                {
                    // 检查所需消耗的资源、道具是否满足
                    const auto buildingingo = Data::LegionBuilding::get(1);
                    if(buildingingo)
                    {
                        // 查看消耗资源
                        boost::container::flat_map<LegionAttributeId, std::string> Attributes;

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
                        // 屏蔽掉势利范围的检测
                        /*const auto distance = get_distance_of_coords(coord, castle->get_coord());
                        if(distance > castle_upgrade_data->max_map_cell_distance){
                            return Response(Msg::ERR_DEFENSE_BUILDING_IS_TOO_FAR_AWAY) <<distance;
                        }
                        */
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

                                // 更新消耗的军团属性
								const auto old_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
                                legion->set_attributes(Attributes);
								const auto new_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
								if(old_money != new_money){
									LegionLog::LegionMoneyTrace(member->get_legion_uuid(),old_money,new_money,ReasonIds::ID_LEGION_CREATE_BUILDING,req.map_object_type_id,0,0);
								}

                                const auto defense_building = boost::make_shared<WarehouseBuilding>(defense_building_uuid, map_object_type_id,
                                    account_uuid, MapObjectUuid(), std::string(), coord, utc_now,member->get_legion_uuid());
                                defense_building->pump_status();
                                copy_buff(defense_building, utc_now, castle, BuffIds::ID_CASTLE_PROTECTION_PREPARATION);
                                copy_buff(defense_building, utc_now, castle, BuffIds::ID_CASTLE_PROTECTION);
                                copy_buff(defense_building, utc_now, castle, BuffIds::ID_NOVICIATE_PROTECTION);
                                defense_building->create_mission(WarehouseBuilding::MIS_CONSTRUCT, duration, { });
                                WorldMap::insert_map_object(defense_building);

                                auto pair = LegionBuilding::async_create(legion_building_uuid,member->get_legion_uuid(),defense_building_uuid,req.map_object_type_id);
                                Poseidon::JobDispatcher::yield(pair.first, true);

                                auto building = std::move(pair.second);
                                // 初始化军团建筑物的属性
                                //building->InitAttributes(defense_building_uuid.str());

                                LegionBuildingMap::insert(building);

                                LegionBuildingMap::synchronize_with_player(member->get_legion_uuid(),session);

                                // 广播通知下
                                Msg::SC_LegionNoticeMsg msg;
                                msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_MINE_STATUS_CHANGE;
                                msg.nick = account->get_nick();
                                msg.ext1 = "";
                                legion->sendNoticeMsg(msg);

                                LOG_EMPERY_CENTER_DEBUG("Created defense building: defense_building_uuid = ", defense_building_uuid,
                                    ", map_object_type_id = ", map_object_type_id, ", account_uuid = ", account->get_account_uuid());
                            });
                        if(insuff_resource_id){
                            return Response(Msg::ERR_CASTLE_NO_ENOUGH_RESOURCES) <<insuff_resource_id;
                        }

                        // 日志数据埋点
                        LegionLog::CreateWarehouseBuildingTrace(account_uuid,legion->get_legion_uuid(),req.coord_x, req.coord_y,utc_now);

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
                    const auto map_object_uuid = MapObjectUuid(legion_building->get_map_object_uuid());

                    const auto defense_building = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(map_object_uuid));
                    if(!defense_building){
                        return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
                    }

                    /*
                    if(defense_building->get_owner_uuid() != account->get_account_uuid()){
                        return Response(Msg::ERR_NOT_YOUR_MAP_OBJECT) <<defense_building->get_owner_uuid();
                    }
                    */
                    // 检查当前矿井是否还有剩余资源
                    if(defense_building->get_output_amount() > 0)
                    {
                        return Response(Msg::ERR_LEGION_UPGRADE_GRUBE_LEFT);
                    }

                    // 查看当前是否正在升级中
                    const auto mission = defense_building->get_mission();
                    if(mission == WarehouseBuilding::Mission::MIS_CONSTRUCT || mission == WarehouseBuilding::Mission::MIS_UPGRADE)
                    {
                        return Response(Msg::ERR_LEGION_BUILDING_UPGRADING);
                    }
                    else
                    {
                        const auto cur_level = defense_building->get_level();
                        const auto buildingingo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(cur_level) + 1);
                        if(buildingingo)
                        {
                            // 查看升级条件
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


                            // 查看消耗资源
							boost::container::flat_map<LegionAttributeId, std::string> Attributes;

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
                  //          const auto parent_object_uuid = defense_building->get_parent_object_uuid();  
                            const auto castle_uuid = MapObjectUuid(req.castle_uuid);
                            const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(castle_uuid));
                            if(!castle){
                                return Response(Msg::ERR_NO_SUCH_CASTLE) <<castle_uuid;
                            }
                            /*
                            const auto castle = boost::dynamic_pointer_cast<Castle>(WorldMap::get_map_object(parent_object_uuid));
                            if(!castle){
                                return Response(Msg::ERR_NO_SUCH_CASTLE) <<parent_object_uuid;
                            }
                            */
                            const auto task_box = TaskBoxMap::require(account->get_account_uuid());

                            defense_building->pump_status();

                            if(defense_building->get_mission() != WarehouseBuilding::MIS_NONE){
                                return Response(Msg::ERR_BUILDING_MISSION_CONFLICT);
                            }


                            const auto map_object_type_id = defense_building->get_map_object_type_id();

                            const auto duration = static_cast<std::uint64_t>(std::ceil(buildingingo->levelup_time * 60000.0 - 0.001));

                            const auto insuff_resource_id = try_decrement_resources(castle, task_box, { },
                                ReasonIds::ID_UPGRADE_BUILDING, map_object_type_id.get(), buildingingo->house_level, 0,
                                [&]{
                                    defense_building->create_mission(WarehouseBuilding::Mission::MIS_UPGRADE, duration, { });
									const auto old_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
                                    // 更新消耗的军团属性
                                    legion->set_attributes(Attributes);
									const auto new_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
									if(old_money != new_money){
										LegionLog::LegionMoneyTrace(member->get_legion_uuid(),old_money,new_money,ReasonIds::ID_UPGRADE_BUILDING,map_object_type_id.get(),buildingingo->house_level,0);
									}

                                    LegionBuildingMap::synchronize_with_player(member->get_legion_uuid(),session);

                                        // 广播通知下
                                        Msg::SC_LegionNoticeMsg msg;
                                        msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_MINE_STATUS_CHANGE;
                                        msg.nick = account->get_nick();
                                        msg.ext1 = "";
                                        legion->sendNoticeMsg(msg);
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

PLAYER_SERVLET(Msg::CS_OpenLegionGrubeMessage, account, session,  req )
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
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_OPENMINE))
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
			else
			{
                // 查看是否存在该建筑物
                const auto legion_building = LegionBuildingMap::find(LegionBuildingUuid(req.legion_building_uuid));
                if(legion_building)
                {
                     const auto map_object_uuid = MapObjectUuid(legion_building->get_map_object_uuid());

                    const auto &warehouse_building = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(map_object_uuid));
                    if(!warehouse_building){
                        return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
                    }

          //          warehouse_building->on_hp_change(6000);

         //          return Response();


                     const auto cur_level = warehouse_building->get_level();
                     const auto buildingingo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(cur_level));
                     if(!buildingingo)
                        return Response(Msg::ERR_LEGION_CANNOT_FIND_DATA);

                    // 查看当前是否正在建造升级中
                    const auto mission = warehouse_building->get_mission();
                    if(mission == WarehouseBuilding::Mission::MIS_CONSTRUCT || mission == WarehouseBuilding::Mission::MIS_UPGRADE)
                    {
                        return Response(Msg::ERR_LEGION_BUILDING_UPGRADING);
                    }
                    else if(mission == WarehouseBuilding::Mission::MIS_DESTRUCT)
                    {
                        return Response(Msg::ERR_LEGION_OPEN_GRUBE_DESTRUCT);
                    }
                    else
                    {
                        // 检查是否过了开启CD时间
                  //      const auto cdtime = legion_building->get_attribute(LegionBuildingAttributeIds::ID_CD_TIME);

                        const auto cdtime = warehouse_building->get_cd_time();

                //        LOG_EMPERY_CENTER_INFO("CS_OpenLegionGrubeMessage*******************",cdtime);

                        const auto utc_now = Poseidon::get_utc_time();

                    //    const auto ncdtime = boost::lexical_cast<boost::uint64_t>(cdtime) * 1000;

                        if(utc_now < cdtime)
                        {
                            return Response(Msg::ERR_LEGION_OPEN_GRUBE_INCDTIMR);
                        }

                        // 检查当前矿井是否还有剩余资源
                        if(warehouse_building->get_output_amount() > 0)
                        {
                            return Response(Msg::ERR_LEGION_OPEN_GRUBE_RESIDUE);
                        }
                        // 检查是否满足开启条件
                        boost::container::flat_map<LegionAttributeId, std::string> Attributes;

             //           LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage req.ntype *******************",req.ntype);
                        const auto it = buildingingo->open_effect.find(boost::lexical_cast<std::uint64_t>(req.ntype));
                        if(it == buildingingo->open_effect.end())
                        {
          //                  LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage req.ntype 没找到 *******************");
                            return Response(Msg::ERR_LEGION_CANNOT_FIND_DATA);
                        }
                        else
                        {
          //                  LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage req.ntype 找到了 *******************");
                            const auto &info = it->second;
                             const auto cit = info.find(boost::lexical_cast<std::uint64_t>(req.consume));
                             if(cit == info.end())
                             {
         //                       LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage req.ntype 没找到数量 *******************",req.consume);
                                return Response(Msg::ERR_LEGION_CANNOT_FIND_DATA);
                             }
                             else
                             {
         //                        LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage req.ntype 找到数量 *******************",req.consume);

                                 const auto &need = cit->second;

                                 for(auto nit = need.begin(); nit != need.end(); ++nit)
                                {
        //                            LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage nit->first *******************",nit->first);

                                    if(nit->first == 5500001)  // 军团资金
                                    {
                                        const auto legion_momey = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
       //                                 LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage legion_momey *******************",legion_momey);
                                        if(legion_momey < nit->second)
                                        {
                                            return Response(Msg::ERR_LEGION_OPEN_GRUBE_LACK);
                                        }
                                        else
                                        {
       //                                   LOG_EMPERY_CENTER_ERROR("CS_OpenLegionGrubeMessage req.ntype 军团资金够 扣除*******************",nit->second);
                                            Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY)) - nit->second);
                                        }
                                    }
                                }
                             }
                        }

                        LOG_EMPERY_CENTER_DEBUG("CS_OpenLegionGrubeMessage req.ntype 满足条件，可以开启 *******************");

                        // 满足条件，可以开启
						const auto old_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
                        legion->set_attributes(Attributes);   // 更新消耗的军团属性
						const auto new_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
						if(new_money != old_money){
							LegionLog::LegionMoneyTrace(member->get_legion_uuid(),old_money,new_money,ReasonIds::ID_LEGION_OPEN_GRUBE,0,0,0);
						}
                        // 更新建筑物属性
                   //     boost::container::flat_map<LegionBuildingAttributeId, std::string> modifiers;

                  //      const auto utc_now = Poseidon::get_utc_time();

                  //      std::string strtime = boost::lexical_cast<std::string>(utc_now + 60 * 1000 * buildingingo->open_time);  // 开启CD时间

                 //       strtime = strtime.substr(0,10);

                  //      modifiers[LegionBuildingAttributeIds::ID_CD_TIME] = strtime;

//                        modifiers[LegionBuildingAttributeIds::ID_OUTPUT_TYPE] = req.ntype;

//                        modifiers[LegionBuildingAttributeIds::ID_OUTPUT_AMOUNT] = 100;

//                        modifiers[LegionBuildingAttributeIds::ID_OUTPUT_EFFECT_TIME] = boost::lexical_cast<std::string>(utc_now + 60 * 1000 * buildingingo->effect_time);  // 开启CD时间;

                //        legion_building->set_attributes(std::move(modifiers));

                   //     obj->set_mission_time_end(saturated_add(utc_now, duration));

                        warehouse_building->open(req.ntype,req.consume,static_cast<std::uint64_t>(std::ceil(buildingingo->effect_time * 60000.0)), saturated_add(utc_now, static_cast<std::uint64_t>(std::ceil(buildingingo->open_time * 60000.0))));

                        // 广播通知下
                        Msg::SC_LegionNoticeMsg msg;
                        msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_MINE_STATUS_CHANGE;
                        msg.nick = account->get_nick();
                        msg.ext1 = "";
                        legion->sendNoticeMsg(msg);

                        // 日志数据埋点
                        const auto coord =  warehouse_building->get_coord();
                        LegionLog::OpenWarehouseBuildingTrace(account_uuid,legion->get_legion_uuid(),coord.x(), coord.y(),cur_level,utc_now);

                        return Response(Msg::ST_OK);
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


PLAYER_SERVLET(Msg::CS_RepairLegionGrubeMessage, account, session,  req )
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
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_REPAIRMINE))
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
			else
			{
                // 查看是否存在该建筑物
                const auto legion_building = LegionBuildingMap::find(LegionBuildingUuid(req.legion_building_uuid));
                if(legion_building)
                {
                     const auto map_object_uuid = MapObjectUuid(legion_building->get_map_object_uuid());

                    const auto &warehouse_building = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(map_object_uuid));
                    if(!warehouse_building){
                        return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
                    }

                     const auto cur_level = warehouse_building->get_level();
                     const auto buildingingo = Data::LegionBuilding::get(boost::lexical_cast<std::uint64_t>(cur_level));
                     if(!buildingingo)
                        return Response(Msg::ERR_LEGION_CANNOT_FIND_DATA);

                    // 最大血量
                    const auto maxhp = warehouse_building->get_max_hp();
                    // 当前血量
                    const auto curhp = warehouse_building->get_attribute(AttributeIds::ID_HP_TOTAL);
                    // 维修所需资源
                    const auto repairhp = maxhp * boost::lexical_cast<std::int64_t>(req.percent) / 100 - curhp;
                 //   LOG_EMPERY_CENTER_INFO("CS_RepairLegionGrubeMessage  maxhp:*******************",maxhp," curhp:",curhp," repairhp:",repairhp);
                    if(repairhp <= 0)
                    {
                        return Response(Msg::ERR_LEGION_REPAIR_GRUBE_HPERROR);
                    }

                    // 检查是否满足维修条件
                    boost::container::flat_map<LegionAttributeId, std::string> Attributes;
                    const auto repariinfo = Data::Global::as_object(Data::Global::SLOT_LEGION_RESTORE_DEPOT_BLOOD);
               //     LOG_EMPERY_CENTER_INFO("CS_RepairLegionGrubeMessage  repariinfo:*******************",repariinfo.dump());
                    for(auto it = repariinfo.begin(); it != repariinfo.end(); ++it)
                    {
                        const auto str = std::string(it->first.get());

                  //      LOG_EMPERY_CENTER_INFO("CS_RepairLegionGrubeMessage  str:*******************",str," type:",req.type);
                        if(str == req.type)
                        {
                            if(str == "5500001")
                            {
                                const auto legion_momey = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
                                const auto needmoney = static_cast<std::uint64_t>(it->second.get<double>()) * static_cast<std::uint64_t>(repairhp) / 10;
                     //           LOG_EMPERY_CENTER_INFO("CS_RepairLegionGrubeMessage  legion_momey:*******************",legion_momey," needmoney:",needmoney);
                                if(legion_momey <  needmoney)
                                {
                                    return Response(Msg::ERR_LEGION_OPEN_GRUBE_LACK);
                                }
                                else
                                {
                                    Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY)) - needmoney);
                                }
                            }
                        }
                    }


                    // 满足条件，可以维修
					const auto old_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
                    legion->set_attributes(Attributes);   // 更新消耗的军团属性
					const auto new_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
					const auto map_object_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(map_object_uuid.get()[0]));
					if(new_money != old_money){
						LegionLog::LegionMoneyTrace(member->get_legion_uuid(),old_money,new_money,ReasonIds::ID_LEGION_REPAIR_GRUBE,map_object_uuid_head,0,0);
					}

                    if(curhp == 0 && warehouse_building->get_mission() == WarehouseBuilding::Mission::MIS_DESTRUCT)
                    {
                        warehouse_building->cancel_mission();
                    }

                	boost::container::flat_map<AttributeId, std::int64_t> modifiers;

					modifiers[AttributeIds::ID_HP_TOTAL] = curhp+repairhp;

					warehouse_building->set_attributes(modifiers);

                    warehouse_building->on_hp_change(curhp+repairhp);

                    // 广播通知下
                    Msg::SC_LegionNoticeMsg msg;
                    msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_MINE_STATUS_CHANGE;
                    msg.nick = account->get_nick();
                    msg.ext1 = "";
                    legion->sendNoticeMsg(msg);

                    return Response(Msg::ST_OK);
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

PLAYER_SERVLET(Msg::CS_GradeLegionMessage, account, session, req)
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
			// 是否有权限设置
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID))),Legion::LEGION_POWER::LEGION_POWER_GRADE))
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
			else
			{
				// 获得下一级的配置
				const auto level = boost::lexical_cast<uint32_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL)) + 1;
				const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(level));
				if(levelinfo)
				{
					// 查看升级需要满足的建筑条件
					for(auto it = levelinfo->level_up_limit_map.begin(); it != levelinfo->level_up_limit_map.end(); ++it)
					{
						if(it->first == "5500002")
						{
							// 判断军团货仓等级
							bool bfind = false;
							std::vector<boost::shared_ptr<LegionBuilding>> buildings;
                			LegionBuildingMap::find_by_type(buildings,LegionUuid(member->get_legion_uuid()),5500002);
							for(auto bit = buildings.begin(); bit != buildings.end(); ++bit)
							{
								const auto legion_building = LegionBuildingMap::find((*bit)->get_legion_building_uuid());
								if(legion_building)
								{
									const auto map_object_uuid = legion_building->get_map_object_uuid();
									const auto defense_building = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(map_object_uuid));
									if(defense_building && boost::lexical_cast<std::uint64_t>(defense_building->get_level()) >= it->second)
									{
										bfind = true;
										break;
									}
								}
							}

							if(!bfind)
							{
								return Response(Msg::ERR_LEGION_UPGRADE_NEEDRESOURCE_ERROR);
							}
						}
					}

					// 查看升级所需资源
					boost::container::flat_map<LegionAttributeId, std::string> Attributes;
					for(auto it = levelinfo->level_up_cost_map.begin(); it != levelinfo->level_up_cost_map.end(); ++it)
					{
						if(it->first == "5500001")
						{
							// 判断军团资金
							 const auto legion_momey = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
							if(legion_momey < it->second)
							{
								return Response(Msg::ERR_LEGION_UPGRADE_NEED_LACK);
							}
							else
							{
								Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY)) - it->second);
							}
						}
					}
					const auto old_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
					// 条件满足，直接升级
					Attributes[LegionAttributeIds::ID_LEVEL] = boost::lexical_cast<std::string>(level);
					legion->set_attributes(Attributes);
					const auto new_money = boost::lexical_cast<uint64_t>(legion->get_attribute(LegionAttributeIds::ID_MONEY));
					const auto legion_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(member->get_legion_uuid().get()[0]));
					LegionLog::LegionMoneyTrace(member->get_legion_uuid(),old_money,new_money,ReasonIds::ID_LEGION_UPGRADE,legion_uuid_head,level,0);

                    // 广播给军团成员
                    Msg::SC_LegionNoticeMsg msg;
					msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_LEGION_GRADE;
					msg.nick = account->get_nick();
					msg.ext1 = boost::lexical_cast<std::string>(level);
					legion->sendNoticeMsg(msg);

					return Response(Msg::ST_OK);
				}
				else
				{
					return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
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

PLAYER_SERVLET(Msg::CS_DemolishLegionGrubeMessage, account, session,  req )
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
			if(!Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_DEMOLISH_MINE))
			{
				return Response(Msg::ERR_LEGION_NO_POWER);
			}
			else
			{
                // 查看是否存在该建筑物
                const auto legion_building = LegionBuildingMap::find(LegionBuildingUuid(req.legion_building_uuid));
                if(legion_building)
                {
                    const auto map_object_uuid = MapObjectUuid(legion_building->get_map_object_uuid());

                    const auto &warehouse_building = boost::dynamic_pointer_cast<WarehouseBuilding>(WorldMap::get_map_object(map_object_uuid));
                    if(!warehouse_building){
                        return Response(Msg::ERR_NO_SUCH_MAP_OBJECT) <<map_object_uuid;
                    }

                    // 从LegionBuildingMap中删除
                    LegionBuildingMap::deleteInfo_by_legion_building_uuid(LegionBuildingUuid(req.legion_building_uuid));

                    return Response(Msg::ST_OK);
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


}
