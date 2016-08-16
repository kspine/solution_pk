#include "../precompiled.hpp"
#include "legion_building_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <tuple>
#include "../player_session.hpp"
#include "../account.hpp"
#include "../string_utilities.hpp"
#include "../msg/sc_legion_building.hpp"
#include "../mysql/legion.hpp"
#include "../attribute_ids.hpp"

namespace EmperyCenter {

namespace {
	struct LegionBuildingElement {
		boost::shared_ptr<MySql::Center_LegionBuilding> building;

		LegionBuildingUuid legion_building_uuid;
		LegionUuid legion_uuid;

		explicit LegionBuildingElement(boost::shared_ptr<MySql::Center_LegionBuilding> building_)
			: building(std::move(building_))
			, legion_building_uuid(building->get_legion_building_uuid())
			, legion_uuid(building->get_legion_uuid())

		{
		}

	};

	MULTI_INDEX_MAP(LegionContainer, LegionBuildingElement,
		MULTI_MEMBER_INDEX(legion_building_uuid)
		MULTI_MEMBER_INDEX(legion_uuid)
	)

	boost::shared_ptr<LegionContainer> g_building_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// Legion
		struct TempLegionBuildingElement {
			boost::shared_ptr<MySql::Center_LegionBuilding> obj;
		};
		std::map<LegionUuid, TempLegionBuildingElement> temp_account_map;

		LOG_EMPERY_CENTER_INFO("Loading Center_LegionBuilding...");
		conn->execute_sql("SELECT * FROM `Center_LegionBuilding`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_LegionBuilding>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto legion_uuid = LegionUuid(obj->get_legion_uuid());
			temp_account_map[legion_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_account_map.size(), " Legion(s).");

		const auto legion_map = boost::make_shared<LegionContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			legion_map->insert(LegionBuildingElement(std::move(it->second.obj)));
		}
		g_building_map = legion_map;
		handles.push(legion_map);

	}

}


void LegionBuildingMap::insert(const boost::shared_ptr<MySql::Center_LegionBuilding> &building){
	PROFILE_ME;

	const auto &legion_map = g_building_map;
	if(!legion_map){
		LOG_EMPERY_CENTER_WARNING("LegionBuildingMap  not loaded.");
		DEBUG_THROW(Exception, sslit("LegionBuildingMap not loaded"));
	}


	const auto legion_uuid = building->get_legion_uuid();

	LOG_EMPERY_CENTER_DEBUG("Inserting legion building: account_uuid = ", legion_uuid);

	if(!legion_map->insert(LegionBuildingElement(building)).second){
		LOG_EMPERY_CENTER_WARNING("Legion building already exists: account_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("Legion building already exists"));
	}
}


boost::shared_ptr<MySql::Center_LegionBuilding> LegionBuildingMap::find(LegionUuid legion_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_building_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

/*
	const auto range = account_map->equal_range<1>(account_uuid);
	for(auto it = range.first; it != range.second; ++it){
		if(AccountUuid(it->account->unlocked_get_account_uuid()) == account_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid)
		{
			return it->account;
		}
	}
*/

	return { };
}

std::uint64_t LegionBuildingMap::get_apply_count(LegionUuid account_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_building_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<1>(account_uuid);

	return static_cast<std::uint64_t>(std::distance(range.first, range.second));
}

void LegionBuildingMap::synchronize_with_player(LegionUuid legion_uuid,const boost::shared_ptr<PlayerSession> &session)
{
	PROFILE_ME;
	const auto &account_map = g_building_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		Msg::SC_LegionBuildings msg;
		msg.buildings.reserve(0);
		session->send(msg);
		return ;
	}

	Msg::SC_LegionBuildings msg;
	const auto range = account_map->equal_range<1>(legion_uuid);
	msg.buildings.reserve(static_cast<std::uint64_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it)
	{
		auto &elem = *msg.buildings.emplace(msg.buildings.end());
		const auto info = it->building;

		elem.legion_building_uuid = AccountUuid(info->unlocked_get_legion_building_uuid()).str();

		elem.type = boost::lexical_cast<std::string>(info->unlocked_get_ntype());

		elem.level = "1";

	}

	LOG_EMPERY_CENTER_WARNING("legion buildings count:",msg.buildings.size());

	session->send(msg);
}

void LegionBuildingMap::deleteInfo(LegionUuid legion_uuid,AccountUuid account_uuid,bool bAll)
{
	PROFILE_ME;
	/*
	const auto &account_map = g_building_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion apply join map not loaded.");
		return;
	}

	std::string strsql = "";
	if(bAll)
	{
		// 全部删除该军团的数据,先删除内存中的,因为访问的是内存中的数据
		const auto range = account_map->equal_range<1>(account_uuid);
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<1>(it);
		}


		// 再删除数据库中的
		strsql = "DELETE FROM Center_LegionBuilding WHERE account_uuid='";
		strsql += account_uuid.str();
		strsql += "';";

	}
	else
	{
		const auto range = account_map->equal_range<1>(account_uuid);
		for(auto it = range.first; it != range.second; ++it)
		{
			if(AccountUuid(it->account->unlocked_get_account_uuid()) == account_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid)
			{
				account_map->erase<1>(it);
				strsql = "DELETE FROM Center_LegionBuilding WHERE account_uuid='";
				strsql += account_uuid.str();
				strsql += "' AND legion_uuid='";
				strsql += legion_uuid.str();
				strsql += "';";

				break;

			}
		}
	}

	LOG_EMPERY_CENTER_DEBUG("Reclaiming legion apply join map strsql = ", strsql);

	Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_LegionBuilding",strsql);

	*/
}

void LegionBuildingMap::deleteInfo_by_legion_uuid(LegionUuid legion_uuid)
{
	PROFILE_ME;
	/*
	const auto &account_map = g_building_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion apply join map not loaded.");
		return;
	}

	// 全部删除该军团的数据,先删除内存中的,因为访问的是内存中的数据
	const auto range = account_map->equal_range<0>(legion_uuid);
	for(auto it = range.first; it != range.second;){
		it = account_map->erase<0>(it);
	}


	// 再删除数据库中的
	std::string strsql = "DELETE FROM Center_LegionBuilding WHERE legion_uuid='";
	strsql += legion_uuid.str();
	strsql += "';";


	Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_LegionBuilding",strsql);
	*/

}

void  LegionBuildingMap::find_by_type(std::vector<boost::shared_ptr<MySql::Center_LegionBuilding>> &buildings,LegionUuid legion_uuid,std::uint64_t ntype)
{
	PROFILE_ME;
	const auto &account_map = g_building_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return;
	}


	const auto range = account_map->equal_range<1>(legion_uuid);

	for(auto it = range.first; it != range.second; ++it){
		if(it->building->unlocked_get_ntype() == ntype)
		{
			buildings.reserve(buildings.size() + 1);
			buildings.emplace_back(it->building);
		}
	}


	return;
}


}
