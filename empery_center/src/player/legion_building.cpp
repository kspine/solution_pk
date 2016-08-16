#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../mysql/legion.hpp"
#include "../msg/cs_legion_building.hpp"
#include "../data/legion_building_config.hpp"
#include "../account.hpp"
#include "../legion.hpp"
#include "../singletons/legion_map.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../singletons/legion_building_map.hpp"
#include "../data/global.hpp"
#include "../data/legion_corps_power.hpp"
#include "../msg/err_legion.hpp"
#include "../legion_member_attribute_ids.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>


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
                std::vector<boost::shared_ptr<MySql::Center_LegionBuilding>> buildings;
                LegionBuildingMap::find_by_type(buildings,LegionUuid(member->get_legion_uuid()),req.ntype);
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
                        // 查看消耗道具
                        for(auto it = buildingingo->need_resource.begin(); it != buildingingo->need_resource.end(); ++it)
                        {

                        }

                        // 满足条件，可以建造
                        const auto legion_building_uuid = LegionBuildingUuid(Poseidon::Uuid::random());
                        auto obj = boost::make_shared<MySql::Center_LegionBuilding>( LegionBuildingUuid(legion_building_uuid).get(),LegionUuid(member->get_legion_uuid()).get(),req.ntype);
                        // obi设置属性
					    obj->enable_auto_saving();
					    auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);

					    LegionBuildingMap::insert(obj);

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


}
