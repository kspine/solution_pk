#include "../precompiled.hpp"
#include "legion_invitejoin_map.hpp"
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
		boost::shared_ptr<MySql::Center_LegionInviteJoin> account;

		LegionUuid legion_uuid;
		AccountUuid account_uuid;
		AccountUuid invited_uuid;

		explicit LegionElement(boost::shared_ptr<MySql::Center_LegionInviteJoin> account_)
			: account(std::move(account_))
			, legion_uuid(account->get_legion_uuid())
			, account_uuid(account->get_account_uuid())
			, invited_uuid(account->get_invited_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(LegionContainer, LegionElement,
		MULTI_MEMBER_INDEX(legion_uuid)
		MULTI_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(invited_uuid)
	)

	boost::shared_ptr<LegionContainer> g_invitejoin_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// Legion
		struct TempLegionElement {
			boost::shared_ptr<MySql::Center_LegionInviteJoin> obj;
		};
		std::map<LegionUuid, TempLegionElement> temp_account_map;

		LOG_EMPERY_CENTER_INFO("Loading Center_LegionInviteJoin...");
		conn->execute_sql("SELECT * FROM `Center_LegionInviteJoin`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_LegionInviteJoin>();
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
		g_invitejoin_map = legion_map;
		handles.push(legion_map);

	}

}


void LegionInviteJoinMap::insert(const boost::shared_ptr<MySql::Center_LegionInviteJoin> &legion){
	PROFILE_ME;

	const auto &legion_map = g_invitejoin_map;
	if(!legion_map){
		LOG_EMPERY_CENTER_WARNING("LegionInviteJoinMap  not loaded.");
		DEBUG_THROW(Exception, sslit("LegionInviteJoinMap not loaded"));
	}


	const auto legion_uuid = legion->get_account_uuid();

	LOG_EMPERY_CENTER_DEBUG("Inserting legion: account_uuid = ", legion_uuid);

	if(!legion_map->insert(LegionElement(legion)).second){
		LOG_EMPERY_CENTER_WARNING("Legion already exists: account_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("Legion already exists"));
	}
}


boost::shared_ptr<MySql::Center_LegionInviteJoin> LegionInviteJoinMap::find(AccountUuid account_uuid,LegionUuid legion_uuid,AccountUuid invited_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("g_invitejoin_map not loaded.");
		return { };
	}

	LOG_EMPERY_CENTER_DEBUG("LegionInviteJoinMap find: account_uuid = ", account_uuid,"  legion_uuid:",legion_uuid, "  invited_uuid:",invited_uuid);

	const auto range = account_map->equal_range<0>(legion_uuid);
	for(auto it = range.first; it != range.second; ++it){
		LOG_EMPERY_CENTER_DEBUG("LegionInviteJoinMap find*********************** account_uuid = ", AccountUuid(it->account->unlocked_get_account_uuid()),"  legion_uuid:",LegionUuid(it->account->unlocked_get_legion_uuid()), "  invited_uuid:",AccountUuid(it->account->unlocked_get_invited_uuid()));
		if(AccountUuid(it->account->unlocked_get_account_uuid()) == account_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid && (AccountUuid(it->account->unlocked_get_invited_uuid()) == invited_uuid))
		{
			LOG_EMPERY_CENTER_WARNING("g_invitejoin_map  找到了.");
			return it->account;
		}
	}


	return { };
}

void LegionInviteJoinMap::get_by_invited_uuid(std::vector<boost::shared_ptr<MySql::Center_LegionInviteJoin>> &ret, AccountUuid invited_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("g_invitejoin_map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<2>(invited_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->account);
	}
}

boost::shared_ptr<MySql::Center_LegionInviteJoin> LegionInviteJoinMap::find_inviteinfo_by_user(AccountUuid account_uuid,LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<2>(account_uuid);
	for(auto it = range.first; it != range.second; ++it){
		LOG_EMPERY_CENTER_DEBUG("LegionInviteJoinMap find*********************** ", "  legion_uuid:",LegionUuid(it->account->unlocked_get_legion_uuid()), "  invited_uuid:",AccountUuid(it->account->unlocked_get_invited_uuid()));
		if(LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid && (AccountUuid(it->account->unlocked_get_invited_uuid()) == account_uuid))
		{
			LOG_EMPERY_CENTER_WARNING("g_invitejoin_map  找到了.");
			return it->account;
		}
	}

	return { };

}

std::uint64_t LegionInviteJoinMap::get_apply_count(AccountUuid account_uuid)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto range = account_map->equal_range<1>(account_uuid);

	return static_cast<std::uint64_t>(std::distance(range.first, range.second));
}

void LegionInviteJoinMap::synchronize_with_player(LegionUuid legion_uuid,const boost::shared_ptr<PlayerSession> &session)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
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
	//	elem.prosperity = "0";   //ID_PROSPERITY_POINTS

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

void LegionInviteJoinMap::deleteInfo(LegionUuid legion_uuid,AccountUuid account_uuid,bool bAll)
{
	PROFILE_ME;
	const auto &account_map = g_invitejoin_map;
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
		strsql = "DELETE FROM Center_LegionInviteJoin WHERE account_uuid='";
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
				strsql = "DELETE FROM Center_LegionInviteJoin WHERE account_uuid='";
				strsql += account_uuid.str();
				strsql += "' AND legion_uuid='";
				strsql += legion_uuid.str();
				strsql += "';";

				break;

			}
		}
	}

	LOG_EMPERY_CENTER_DEBUG("Reclaiming legion apply join map strsql = ", strsql);

	Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_LegionInviteJoin",strsql);
}

void LegionInviteJoinMap::deleteInfo_by_invited_uuid(AccountUuid account_uuid)
{
	PROFILE_ME;
	// 先从内存中删除
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion apply join map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<2>(account_uuid);
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<2>(it);
		}

	// 再从数据库中删除
	std::string strsql = "DELETE FROM Center_LegionInviteJoin WHERE invited_uuid='";
	strsql += account_uuid.str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_LegionInviteJoin",strsql);
}

void LegionInviteJoinMap::deleteInfo_by_legion_uuid(LegionUuid legion_uuid)
{
	PROFILE_ME;
	// 先从内存中删除
	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion apply join map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<0>(legion_uuid);
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<0>(it);
		}

	// 再从数据库中删除
	std::string strsql = "DELETE FROM Center_LegionInviteJoin WHERE legion_uuid='";
	strsql += legion_uuid.str();
	strsql += "';";

	Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_LegionInviteJoin",strsql);
}

void LegionInviteJoinMap::deleteInfo_by_invitedres_uuid(AccountUuid account_uuid,LegionUuid legion_uuid,bool bAll/* = false*/)
{
	PROFILE_ME;

	const auto &account_map = g_invitejoin_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion apply join map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<2>(account_uuid);
	std::string strsql = "";
	if(bAll)
	{
		// 先从内存中删除
		for(auto it = range.first; it != range.second;){
			it = account_map->erase<2>(it);
		}

		// 再从数据库中删除
		strsql = "DELETE FROM Center_LegionInviteJoin WHERE invited_uuid='";
		strsql += account_uuid.str();
		strsql += "';";
	}
	else
	{
		// 只删当前反馈的军团uuid和被邀请人的uuid相等的一条数据
		const auto range = account_map->equal_range<2>(account_uuid);
		for(auto it = range.first; it != range.second; ++it)
		{
			if(AccountUuid(it->account->unlocked_get_invited_uuid()) == account_uuid && LegionUuid(it->account->unlocked_get_legion_uuid()) == legion_uuid)
			{
				account_map->erase<2>(it);

				strsql = "DELETE FROM Center_LegionInviteJoin WHERE invited_uuid='";
				strsql += account_uuid.str();
				strsql += "' AND legion_uuid='";
				strsql += legion_uuid.str();
				strsql += "';";

				break;

			}
		}
	}

	LOG_EMPERY_CENTER_DEBUG("deleteInfo_by_invitedres_uuid strsql = ", strsql);

	Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_LegionInviteJoin",strsql);
}

}
