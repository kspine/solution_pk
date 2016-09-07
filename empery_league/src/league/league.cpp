#include "../precompiled.hpp"
#include "common.hpp"
#include "../singletons/league_map.hpp"
#include "../league.hpp"
#include "../checked_arithmetic.hpp"
#include "../../../empery_center/src/msg/sl_league.hpp"
#include "../../../empery_center/src/msg/ls_league.hpp"
#include "../../../empery_center/src/msg/err_league.hpp"
#include "../singletons/league_member_map.hpp"
#include "../singletons/league_map.hpp"
#include "../league_member.hpp"
#include "../league.hpp"
#include "../singletons/league_invitejoin_map.hpp"
#include "../singletons/league_applyjoin_map.hpp"
#include "../league_member_attribute_ids.hpp"
#include <poseidon/async_job.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>

namespace EmperyLeague {

LEAGUE_SERVLET(Msg::SL_LeagueCreated, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);
	auto account_uuid = AccountUuid(req.account_uuid);
	const auto name     	= 	req.name;
	const auto icon        	= 	req.icon;
	const auto content       = 	req.content;
	const auto language 	= 	req.language;
	/*
	auto dungeon_uuid = LeagueUuid(req.dungeon_uuid);
	auto founder_uuid = AccountUuid(req.founder_uuid);
	//boost::shared_ptr<League> new_league = boost::make_shared<League>(dungeon_uuid, league_client,founder_uuid,utc_now);
	//new_dungeon->set_dungeon_duration(1000*60*3);
   */
    // 判断军团是否已经加入到联盟中
	std::vector<boost::shared_ptr<League>> leagues;
	LeagueMap::get_by_legion_uuid(leagues,legion_uuid);
	if(!leagues.empty())
	{
		/*
		EmperyCenter::Msg::LS_CreateLeagueRes msg;
		msg.account_uuid = account_uuid.str();
		msg.res = Msg::ERR_ACCOUNT_HAVE_LEAGUE;
		league_client->send(msg);
		*/
		return Response(Msg::ERR_ACCOUNT_HAVE_LEAGUE);
	}
	// 判断玩家是否已有联盟

	// 判断名字是否重复
	leagues.clear();
	LeagueMap::get_by_nick(leagues,req.name);
	if(!leagues.empty())
	{
		/*
		EmperyCenter::Msg::LS_CreateLeagueRes msg;
		msg.account_uuid = account_uuid.str();
		msg.res = Msg::ERR_LEAGUE_CREATE_HOMONYM;
		league_client->send(msg);
		*/
		return Response(Msg::ERR_LEAGUE_CREATE_HOMONYM);
	}

	// 创建军团
	const auto utc_now = Poseidon::get_utc_time();

	const auto league_uuid = LeagueUuid(Poseidon::Uuid::random());
	auto pair = League::async_create(league_uuid,legion_uuid, std::move(name),account_uuid, utc_now);
	Poseidon::JobDispatcher::yield(pair.first, true);

	auto league = std::move(pair.second);
	// 初始化联盟的一些属性
	league->InitAttributes(legion_uuid,std::move(content), std::move(language), std::move(icon),req.bautojoin);

	// 增加联盟成员
	league->AddMember(legion_uuid,account_uuid,1,utc_now);

	LeagueMap::insert(league);

//	count = LeagueMap::get_count();
//	LOG_EMPERY_CENTER_INFO("SL_LeagueCreated count:", count);

	league->synchronize_with_player(league_client,account_uuid,legion_uuid);

	// 如果还请求加入过其他联盟，也需要做善后操作，删除其他所有加入请求
	LeagueApplyJoinMap::deleteInfo(legion_uuid,league_uuid,true);

	// 别人邀请加入的也要清空
	LeagueInviteJoinMap::deleteInfo_by_legion_uuid(legion_uuid);

	return Response(Msg::ST_OK);
}

LEAGUE_SERVLET(Msg::SL_LeagueInfo, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);

	std::vector<boost::shared_ptr<League>> leagues;
	LeagueMap::get_by_legion_uuid(leagues,legion_uuid);
	if(leagues.empty())
	{
		return Response(Msg::ERR_LEAGUE_CANNOT_FIND);
	}

	const auto& league = leagues.at(0);

	league->synchronize_with_player(league_client,AccountUuid(req.account_uuid),legion_uuid);

	return Response(Msg::ST_OK);

}

LEAGUE_SERVLET(Msg::SL_GetAllLeagueInfo, league_client, req){
	PROFILE_ME;

	LOG_EMPERY_LEAGUE_ERROR("SL_GetAllLeagueInfo = ",req.account_uuid);

	const auto account_uuid = req.account_uuid;

	std::vector<boost::shared_ptr<League>> leagues;
	LeagueMap::get_all(leagues, 0, 1000);

	EmperyCenter::Msg::LS_Leagues msg;
	msg.account_uuid = account_uuid;
	msg.leagues.reserve(leagues.size());

	for(auto it = leagues.begin(); it != leagues.end(); ++it )
	{
		auto &elem = *msg.leagues.emplace(msg.leagues.end());
		auto league = *it;

		elem.league_uuid = league->get_league_uuid().str();
		elem.league_name = league->get_nick();
		elem.league_icon = league->get_attribute(LeagueAttributeIds::ID_ICON);
		elem.league_notice = league->get_attribute(LeagueAttributeIds::ID_NOTICE);
		elem.league_leader_uuid = league->get_create_uuid().str();
		elem.autojoin = league->get_attribute(LeagueAttributeIds::ID_AUTOJOIN);

		elem.league_create_time =  boost::lexical_cast<std::string>(league->get_create_league_time());
		elem.isapplyjoin = "0";

		// 根据account_uuid查找是否有军团
		std::vector<boost::shared_ptr<LeagueMember>> members;
		LeagueMemberMap::get_by_league_uuid(members, league->get_league_uuid());

		elem.legions.reserve(members.size());
		for(auto it = members.begin(); it != members.end(); ++it )
		{
			auto &legion = *elem.legions.emplace(elem.legions.end());
			auto info = *it;

			legion.legion_uuid = info->get_legion_uuid().str();

		}

		/*
		// 是否请求加入过
		const auto apply = LegionApplyJoinMap::find(account_uuid,legion->get_legion_uuid());
		if(apply)
		{
			elem.isapplyjoin = "1";
		}
		else
		{
			elem.isapplyjoin = "0";
		}
		*/
	}

	league_client->send(msg);

	return Response(Msg::ST_OK);

}

void vectorbingji(const std::vector<boost::shared_ptr<League>>& v1,const std::vector<boost::shared_ptr<League>>& v2,std::vector<boost::shared_ptr<League>>& des)//求并集
{
	PROFILE_ME;
	unsigned i,j;
	i = j = 0;//定位到2个有序向量的头部

	des.clear();
	//必有一个向量插入完成
	while(i < v1.size() && j < v2.size())
	{
		if(v1[i]->get_league_uuid() == v2[j]->get_league_uuid())
		{
			des.push_back(v1[i]);//相等则只压入容器一次，标记均后移一个位置，避免元素重复
			i += 1;
			j += 1;
		}
		else if(v1[i]->get_league_uuid() < v2[j]->get_league_uuid())//不相等则压入较小元素，标记后移一个位置
		{
			des.push_back(v1[i]);
			i += 1;
		}
		else
		{
			des.push_back(v2[j]);
			j += 1;
		}
	}

	//对于部分未能压入向量中的元素，此处继续操作
	while(i < v1.size())
	{
		des.push_back(v1[i]);
		i++;
	}
	while(j < v2.size())
	{
		des.push_back(v2[j]);
		j++;
	}
}

LEAGUE_SERVLET(EmperyCenter::Msg::SL_SearchLeague, league_client, req){
	PROFILE_ME;

//	LOG_EMPERY_LEAGUE_ERROR("SL_SearchLeague = ",req.account_uuid, " req.league_name:",req.league_name, " legion size:",req.legions.size());

	const auto account_uuid = req.account_uuid;

	std::vector<boost::shared_ptr<League>> leagues;

	if(!req.league_name.empty())
	{
		// 按照联盟名称来查找
		LeagueMap::get_by_nick(leagues, req.league_name);

		if(!req.legions.empty())
		{
			// 存在按照军团搜索的
			for(auto it = leagues.begin(); it != leagues.end(); )
			{
				auto &league = *it;
				bool bfind = false;
				for(auto lit = req.legions.begin(); lit != req.legions.end(); ++lit)
				{
					if(league->get_legion_uuid() == LegionUuid((*lit).legion_uuid))
					{
						bfind = true;
						break;
					}
				}

				if(!bfind)
					it = leagues.erase(it);
				else
					it++;
			}
		}
	}
	else
	{
		// 没有联盟名称，只按照军团来搜索
		for(auto lit = req.legions.begin(); lit != req.legions.end(); ++lit)
		{
			auto legion_uuid = LegionUuid((*lit).legion_uuid);
			std::vector<boost::shared_ptr<League>> temp_leagues;
			LeagueMap::get_by_legion_uuid(temp_leagues,legion_uuid);

	//		LOG_EMPERY_LEAGUE_ERROR("search temp_leagues size ================================= ",temp_leagues.size());

			vectorbingji(leagues,temp_leagues,leagues);
		}
	}

//	LOG_EMPERY_LEAGUE_ERROR("search size ================================= ",leagues.size());

	EmperyCenter::Msg::LS_Leagues msg;
	msg.account_uuid = account_uuid;

	msg.leagues.reserve(leagues.size());

	for(auto it = leagues.begin(); it != leagues.end(); ++it )
	{
		auto &elem = *msg.leagues.emplace(msg.leagues.end());
		auto league = *it;

		elem.league_uuid = league->get_league_uuid().str();
		elem.league_name = league->get_nick();
		elem.league_icon = league->get_attribute(LeagueAttributeIds::ID_ICON);
		elem.league_notice = league->get_attribute(LeagueAttributeIds::ID_NOTICE);
		elem.league_leader_uuid = league->get_create_uuid().str();
		elem.autojoin = league->get_attribute(LeagueAttributeIds::ID_AUTOJOIN);

		elem.league_create_time =  boost::lexical_cast<std::string>(league->get_create_league_time());
		elem.isapplyjoin = "0";

		// 根据account_uuid查找是否有军团
		std::vector<boost::shared_ptr<LeagueMember>> members;
		LeagueMemberMap::get_by_league_uuid(members, league->get_league_uuid());

		elem.legions.reserve(members.size());
		for(auto it = members.begin(); it != members.end(); ++it )
		{
			auto &legion = *elem.legions.emplace(elem.legions.end());
			auto info = *it;

			legion.legion_uuid = info->get_legion_uuid().str();

		}

		/*
		// 是否请求加入过
		const auto apply = LegionApplyJoinMap::find(account_uuid,legion->get_legion_uuid());
		if(apply)
		{
			elem.isapplyjoin = "1";
		}
		else
		{
			elem.isapplyjoin = "0";
		}
		*/
	}

	league_client->send(msg);

	return Response(Msg::ST_OK);


}


LEAGUE_SERVLET(Msg::SL_ApplyJoinLeague, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);
	const auto league_uuid = LeagueUuid(req.league_uuid);
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto member = LeagueMemberMap::get_by_legion_uuid(legion_uuid);

	if(member)
	{
		// 已加入联盟
		return Response(Msg::ERR_ACCOUNT_HAVE_LEAGUE);
	}
	else
	{
		const auto& league = LeagueMap::get(league_uuid);
		if(league)
		{
			// 是否已经申请过
			const auto& apply = LeagueApplyJoinMap::find(legion_uuid,league_uuid);
			if(apply)
			{
				if(req.bCancle)
				{
					// 取消申请
					LeagueApplyJoinMap::deleteInfo(legion_uuid,league_uuid,false);

					return Response(Msg::ST_OK);
				}
				else
				{
					return Response(Msg::ERR_LEAGUE_ALREADY_APPLY);
				}
			}
			else
			{
				if(req.bCancle)
				{
					return Response(Msg::ERR_LEAGUE_CANNOT_FIND_DATA);
				}
				else
				{
					// 检查下最大允许申请的数量
				//	if(LeagueApplyJoinMap::get_apply_count(legion_uuid) >= Data::Global::as_unsigned(Data::Global::SLOT_LEAGUE_ENABLE_APPLY_NUMBER))
					if(LeagueApplyJoinMap::get_apply_count(legion_uuid) >= 2)
					{
						return Response(Msg::ERR_LEAGUE_APPLY_COUNT_LIMIT);
					}
				}
			}

			// 成员数
		/*	const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(boost::lexical_cast<uint32_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL))));
			std::uint64_t limit = 2;
			if(levelinfo)
			{
				limit = levelinfo->legion_member_max;
			}
			else
			{
				return Response(Msg::ERR_LEAGUE_CONFIG_CANNOT_FIND);
			}
			*/
			std::uint64_t limit = 2;
			const auto count = LeagueMemberMap::get_league_member_count(league_uuid);
			if(count < limit)
			{
				const auto utc_now = Poseidon::get_utc_time();
				const auto autojoin = league->get_attribute(LeagueAttributeIds::ID_AUTOJOIN);
				if(autojoin == "0")
				{
					// 不用审核，直接加入
					league->AddMember(legion_uuid,account_uuid,999,utc_now);

					league->synchronize_with_player(league_client,account_uuid,legion_uuid);

					// 如果还请求加入过其他联盟，也需要做善后操作，删除其他所有加入请求
					LeagueApplyJoinMap::deleteInfo(legion_uuid,league_uuid,true);

					// 别人邀请加入的也要清空
					LeagueInviteJoinMap::deleteInfo_by_legion_uuid(legion_uuid);

					// 发邮件告诉结果
					//league->sendmail(account,ChatMessageTypeIds::ID_LEVEL_LEGION_JOIN,legion->get_nick());
					/*
					Msg::SC_LegionEmailMessage msg;
					msg.legion_uuid = LegionUuid(req.legion_uuid).str();
					msg.legion_name = legion->get_nick();
					msg.ntype = Legion::LEGION_EMAIL_TYPE::LEGION_EMAIL_JOIN;
					session->send(msg);
					*/
					return Response(Msg::ST_OK);
				}
				else
				{
					// 添加到待审批的列表中
					auto obj = boost::make_shared<MySql::League_LeagueApplyJoin>( legion_uuid.get(),league_uuid.get(),utc_now);
					obj->enable_auto_saving();
					auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);

					LeagueApplyJoinMap::insert(obj);

					LOG_EMPERY_LEAGUE_DEBUG("CS_ApplyJoinLegionMessage apply_count = ",LeagueApplyJoinMap::get_apply_count(legion_uuid));

					return Response(Msg::ST_OK);
				}
			}
			else
			{
				return Response(Msg::ERR_LEAGUE_MEMBER_FULL);
			}

			   return Response(Msg::ST_OK);
			}
		else
			return Response(Msg::ERR_LEAGUE_CANNOT_FIND);
	}

	return Response(Msg::ST_OK);
}


LEAGUE_SERVLET(Msg::SL_GetApplyJoinLeague, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);

	const auto member = LeagueMemberMap::get_by_legion_uuid(legion_uuid);
	if(member)
	{
		// 查看联盟是否存在
		const auto league = LeagueMap::get(LeagueUuid(member->get_league_uuid()));
		if(league)
		{
			// 查看member是否有权限
			const auto titileid = member->get_attribute(LeagueMemberAttributeIds::ID_TITLEID);
		//	if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_APPROVE))
			if(titileid == "1")
			{
				LeagueApplyJoinMap::synchronize_with_player(league_client,LeagueUuid(member->get_league_uuid()),req.account_uuid);
			}
			else
			{
				return Response(Msg::ERR_LEAGUE_NO_POWER);
			}


			return Response(Msg::ST_OK);
		}
		else
			return Response(Msg::ERR_LEAGUE_CANNOT_FIND);
	}
	else
	{
		// 没有加入联盟
		return Response(Msg::ERR_LEAGUE_NOT_JOIN);
	}

	return Response(Msg::ST_OK);

}

LEAGUE_SERVLET(Msg::SL_LeagueAuditingRes, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);
	const auto target_legion_uuid = LegionUuid(req.target_legion_uuid);
	// 查看对方是否已经加入联盟
	const auto target_memberinfo = LeagueMemberMap::get_by_legion_uuid(target_legion_uuid);
	if(target_memberinfo)
	{
		return Response(Msg::ERR_ACCOUNT_HAVE_LEAGUE);
	}

	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto member = LeagueMemberMap::get_by_legion_uuid(legion_uuid);
	if(member)
	{
		// 查看是否存在该申请
		const auto league_uuid = member->get_league_uuid();
		const auto ApplyJoininfo = LeagueApplyJoinMap::find(target_legion_uuid,league_uuid);
		if(ApplyJoininfo)
		{
			// 查看联盟是否存在
			const auto league = LeagueMap::get(league_uuid);
			if(league)
			{
				// 查看member是否有权限
				const auto titileid = member->get_attribute(LeagueMemberAttributeIds::ID_TITLEID);
			//	if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_APPROVE))
				if(titileid != "1")
				{
					return Response(Msg::ERR_LEAGUE_NO_POWER);
				}
				else
				{
					// 判断成员数
					/*
					const auto levelinfo = Data::LegionCorpsLevel::get(LegionCorpsLevelId(boost::lexical_cast<uint32_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL))));
					std::uint64_t limit = 0;
					if(levelinfo)
					{
						limit = levelinfo->legion_member_max;
					}
					else
					{
						return Response(Msg::ERR_LEGION_CONFIG_CANNOT_FIND);
					}
					*/
					std::uint64_t limit = 2;
					const auto count = LeagueMemberMap::get_league_member_count(league_uuid);
					if(count >= limit)
					{
						return Response(Msg::ERR_LEAGUE_MEMBER_FULL);
					}

					// 处理审批结果
					bool bdeleteAll = false;
					if(req.bagree == 1)
					{
						// 同意加入,增加军团成员
						const auto utc_now = Poseidon::get_utc_time();

						league->AddMember(target_legion_uuid,account_uuid,999,utc_now);

						league->synchronize_with_player(league_client,account_uuid,target_legion_uuid);

						// 如果还请求加入过其他联盟，也需要做善后操作，删除其他所有加入请求
						//LeagueApplyJoinMap::deleteInfo(legion_uuid,league_uuid,true);

						// 别人邀请加入的也要清空
						LeagueInviteJoinMap::deleteInfo_by_legion_uuid(target_legion_uuid);

						bdeleteAll = true;
					}
					else
					{
						// 不同意加入
					}


					// 发邮件告诉结果
					/*
					if(req.bagree == 1)
						legion->sendmail(join_account,ChatMessageTypeIds::ID_LEVEL_LEGION_JOIN,legion->get_nick());
					else
					{
						legion->sendmail(join_account,ChatMessageTypeIds::ID_LEVEL_LEGION_REFUSE_APPLY,legion->get_nick() + ","+ account->get_nick());
					}
					*/

					// 删除该条请求数据
					LeagueApplyJoinMap::deleteInfo(target_legion_uuid,league_uuid,bdeleteAll);

				}

				return Response(Msg::ST_OK);
			}
			else
				return Response(Msg::ERR_LEAGUE_CANNOT_FIND);
		}
		else
		{
			return Response(Msg::ERR_LEAGUE_APPLYJOIN_CANNOT_FIND_DATA);
		}
	}
	else
	{
		// 没有加入联盟
		return Response(Msg::ERR_LEAGUE_NOT_JOIN);
	}

	return Response(Msg::ST_OK);

}

LEAGUE_SERVLET(Msg::SL_LeagueInviteJoin, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);
	const auto target_legion_uuid = LegionUuid(req.target_legion_uuid);

	// 查看对方是否已经加入联盟
	const auto target_memberinfo = LeagueMemberMap::get_by_legion_uuid(target_legion_uuid);
	if(target_memberinfo)
	{
		return Response(Msg::ERR_ACCOUNT_HAVE_LEAGUE);
	}



	const auto member = LeagueMemberMap::get_by_legion_uuid(legion_uuid);

	if(!member)
	{
		// 自己不在联盟中
		return Response(Msg::ERR_LEAGUE_NOT_JOIN);
	}
	else
	{
		const auto league_uuid = member->get_league_uuid();
		const auto& league = LeagueMap::get(league_uuid);
		if(league)
		{
			// 是否已经邀请过
			const auto invate = LeagueInviteJoinMap::find(league_uuid,target_legion_uuid);
			if(invate)
			{
				return Response(Msg::ERR_LEAGUE_ALREADY_INVATE);
			}
			else
			{
				// 成员数
				std::uint64_t limit = 2;
				const auto count = LeagueMemberMap::get_league_member_count(league_uuid);
				if(count < limit)
				{
					const auto utc_now = Poseidon::get_utc_time();
					// 添加到邀请列表中
					auto obj = boost::make_shared<MySql::League_LeagueInviteJoin>( target_legion_uuid.get(),league_uuid.get(),utc_now);
					obj->enable_auto_saving();
					auto promise = Poseidon::MySqlDaemon::enqueue_for_saving(obj, false, true);

					LeagueInviteJoinMap::insert(obj);

					// 通知给目标
					LeagueInviteJoinMap::synchronize_with_player(league_client,target_legion_uuid,AccountUuid());
				}
				else
				{
					return Response(Msg::ERR_LEAGUE_MEMBER_FULL);
				}
			}

			return Response(Msg::ST_OK);
		}
		else
			return Response(Msg::ERR_LEAGUE_CANNOT_FIND);
	}

	return Response(Msg::ST_OK);
}

LEAGUE_SERVLET(Msg::SL_LeagueInviteList, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);
	const auto account_uuid = AccountUuid(req.account_uuid);

	const auto member = LeagueMemberMap::get_by_legion_uuid(legion_uuid);

	if(member)
	{
		// 已加入联盟
		return Response(Msg::ERR_LEAGUE_NOT_JOIN);
	}
	else
	{
		LeagueInviteJoinMap::synchronize_with_player(league_client,legion_uuid,account_uuid);

		return Response(Msg::ST_OK);
	}

	return Response(Msg::ST_OK);
}

LEAGUE_SERVLET(Msg::SL_LeagueInviteJoinRes, league_client, req){
	PROFILE_ME;

	const auto legion_uuid = LegionUuid(req.legion_uuid);
	const auto account_uuid = AccountUuid(req.account_uuid);
	const auto league_uuid = LeagueUuid(req.league_uuid);

	const auto member = LeagueMemberMap::get_by_legion_uuid(legion_uuid);

	if(member)
	{
		// 已加入联盟
		return Response(Msg::ERR_LEAGUE_NOT_JOIN);
	}
	else
	{
		// 是否有该邀请
		const auto& invite = LeagueInviteJoinMap::find(league_uuid,legion_uuid);
		if(invite)
		{
			// 联盟是否存在
			const auto& league = LeagueMap::get(league_uuid);
			if(league)
			{
				if(!req.bagree)
				{
					// 不同意加入
					LOG_EMPERY_LEAGUE_DEBUG("SL_LeagueInviteJoinRes 不同意加入联盟 legion_uuid = ",legion_uuid," league_uuid:",league_uuid);

					// 删除该条记录
					LeagueInviteJoinMap::deleteInfo_by_invitedres_uuid(league_uuid,legion_uuid,false);
				}
				else
				{
					// 检查是否已经达到数量上限
					std::uint64_t limit = 2;
					const auto count = LeagueMemberMap::get_league_member_count(league_uuid);
					if(count >= limit)
						return Response(Msg::ERR_LEAGUE_MEMBER_FULL);
					else
					{
						const auto utc_now = Poseidon::get_utc_time();
						// 加入
						league->AddMember(legion_uuid,account_uuid,999,utc_now);

						league->synchronize_with_player(league_client,account_uuid,legion_uuid);

						// 如果还请求加入过其他联盟，也需要做善后操作，删除其他所有加入请求
						LeagueApplyJoinMap::deleteInfo(legion_uuid,league_uuid,true);

						// 删除所有和自己相关的邀请数据
						LeagueInviteJoinMap::deleteInfo_by_legion_uuid(legion_uuid);

						// 发邮件告诉结果
						//league->sendmail(account,ChatMessageTypeIds::ID_LEVEL_LEGION_JOIN,legion->get_nick());
						/*
						Msg::SC_LegionEmailMessage msg;
						msg.legion_uuid = LegionUuid(req.legion_uuid).str();
						msg.legion_name = legion->get_nick();
						msg.ntype = Legion::LEGION_EMAIL_TYPE::LEGION_EMAIL_JOIN;
						session->send(msg);
						*/
					}
				}

				return Response(Msg::ST_OK);
			}
			else
			{
				return Response(Msg::ERR_LEAGUE_CANNOT_FIND);
			}
		}
		else
		{
			return Response(Msg::ERR_LEGION_NOTFIND_INVITE_INFO);
		}

		return Response(Msg::ST_OK);
	}

	return Response(Msg::ST_OK);
}


}
