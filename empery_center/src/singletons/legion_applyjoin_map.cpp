#include "../precompiled.hpp"
#include "legion_applyjoin_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <tuple>
#include "../player_session.hpp"
#include "../account.hpp"
#include "../string_utilities.hpp"
#include "../msg/sc_legion.hpp"
#include "../mysql/legion.hpp"
#include "account_map.hpp"
#include "../account_attribute_ids.hpp"
#include "../singletons/world_map.hpp"
#include "../castle.hpp"
#include "../attribute_ids.hpp"

namespace EmperyCenter {

namespace {
	struct LegionElement {
		boost::shared_ptr<MySql::Center_LegionApplyJoin> account;

		LegionUuid legion_uuid;
		AccountUuid account_uuid;

		explicit LegionElement(boost::shared_ptr<MySql::Center_LegionApplyJoin> account_)
			: account(std::move(account_))
			, legion_uuid(account->get_legion_uuid())
			, account_uuid(account->get_account_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(LegionContainer, LegionElement,
		MULTI_MEMBER_INDEX(legion_uuid)
		MULTI_MEMBER_INDEX(account_uuid)
	)

	boost::shared_ptr<LegionContainer> g_applyjoin_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// Legion
		struct TempLegionElement {
			boost::shared_ptr<MySql::Center_LegionApplyJoin> obj;
		};
		std::map<LegionUuid, TempLegionElement> temp_account_map;

		LOG_EMPERY_CENTER_INFO("Loading Center_LegionApplyJoin...");
		conn->execute_sql("SELECT * FROM `Center_LegionApplyJoin`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_LegionApplyJoin>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto legion_uuid = LegionUuid(obj->get_legion_uuid());
			temp_account_map[legion_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_account_map.size(), " Legion(s).");

		const auto legion_map = boost::make_shared<LegionContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			legion_map->insert(LegionElement(std::move(it->second.obj)));
		}
		g_applyjoin_map = legion_map;
		handles.push(legion_map);

	}

}


void LegionApplyJoinMap::insert(const boost::shared_ptr<MySql::Center_LegionApplyJoin> &legion){
	PROFILE_ME;

	const auto &legion_map = g_applyjoin_map;
	if(!legion_map){
		LOG_EMPERY_CENTER_WARNING("LegionApplyJoinMap  not loaded.");
		DEBUG_THROW(Exception, sslit("LegionApplyJoinMap not loaded"));
	}


	const auto legion_uuid = legion->get_account_uuid();

	LOG_EMPERY_CENTER_DEBUG("Inserting legion: account_uuid = ", legion_uuid);

	if(!legion_map->insert(LegionElement(legion)).second){
		LOG_EMPERY_CENTER_WARNING("Legion already exists: account_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("Legion already exists"));
	}
}


boost::shared_ptr<MySql::Center_LegionApplyJoin> LegionApplyJoinMap::find(AccountUuid account_uuid,LegionUuid legion_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}


	const auto range = account_map->equal_range<1>(account_uuid);
	for(auto it = range.first; it != range.second; ++it){
		if(AccountUuid(it->account->unlocked_get_account_uuid()) == account_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid)
		{
			return it->account;
		}
	}


	return { };
}

std::uint64_t LegionApplyJoinMap::get_apply_count(AccountUuid account_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<1>(account_uuid);

	return static_cast<std::uint64_t>(std::distance(range.first, range.second));
}

void LegionApplyJoinMap::synchronize_with_player(LegionUuid legion_uuid,const boost::shared_ptr<PlayerSession> &session)
{
	PROFILE_ME;
	const auto &account_map = g_applyjoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		Msg::SC_ApplyList msg;
		msg.applies.reserve(0);
		session->send(msg);
		return ;
	}

	Msg::SC_ApplyList msg;
	const auto range = account_map->equal_range<0>(legion_uuid);
	msg.applies.reserve(static_cast<std::uint64_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it)
	{
		auto &elem = *msg.applies.emplace(msg.applies.end());
		const auto info = it->account;

		elem.account_uuid = AccountUuid(info->unlocked_get_account_uuid()).str();

		const auto account = AccountMap::require(AccountUuid(elem.account_uuid));

		elem.nick = account->get_nick();
		elem.icon = account->get_attribute(AccountAttributeIds::ID_AVATAR);

		const auto primary_castle =  WorldMap::get_primary_castle(AccountUuid(info->unlocked_get_account_uuid()));
		if(primary_castle)
		{
			elem.prosperity = primary_castle->get_attribute(AttributeIds::ID_PROSPERITY_POINTS);

			LOG_EMPERY_CENTER_INFO("城堡繁荣度==============================================",elem.prosperity);
		}
		else
		{
			LOG_EMPERY_CENTER_INFO("城堡繁荣度 没找到==============================================");
			elem.prosperity = "0";   //ID_PROSPERITY_POINTS
		}
	}

	LOG_EMPERY_CENTER_WARNING("legion applies count:",msg.applies.size());

	session->send(msg);
}

void LegionApplyJoinMap::deleteInfo(LegionUuid legion_uuid,AccountUuid account_uuid,bool bAll)
{
	PROFILE_ME;
	const auto &account_map = g_applyjoin_map;
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
		strsql = "DELETE FROM Center_LegionApplyJoin WHERE account_uuid='";
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
				strsql = "DELETE FROM Center_LegionApplyJoin WHERE account_uuid='";
				strsql += account_uuid.str();
				strsql += "' AND legion_uuid='";
				strsql += legion_uuid.str();
				strsql += "';";

				break;

			}
		}
	}

	LOG_EMPERY_CENTER_DEBUG("Reclaiming legion apply join map strsql = ", strsql);

	Poseidon::MySqlDaemon::enqueue_for_deleting("Center_LegionApplyJoin",strsql);
}

void LegionApplyJoinMap::deleteInfo_by_legion_uuid(LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_applyjoin_map;
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
	std::string strsql = "DELETE FROM Center_LegionApplyJoin WHERE legion_uuid='";
	strsql += legion_uuid.str();
	strsql += "';";


	Poseidon::MySqlDaemon::enqueue_for_deleting("Center_LegionApplyJoin",strsql);

}


}
