#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_league.hpp"
#include "../msg/sl_league.hpp"
#include "../msg/err_league.hpp"
#include "../msg/sc_league.hpp"
#include "../account.hpp"
#include "../singletons/player_session_map.hpp"
#include "../singletons/legion_member_map.hpp"
#include "../singletons/legion_map.hpp"
#include "../legion_member.hpp"
#include "../legion_member_attribute_ids.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../item_ids.hpp"
#include "../singletons/league_client.hpp"
#include "../data/legion_corps_level.hpp"
#include "../data/global.hpp"
#include "../account_attribute_ids.hpp"
#include "../singletons/world_map.hpp"
#include "../attribute_ids.hpp"
#include "../castle.hpp"
#include <poseidon/json.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>



namespace EmperyCenter {

    PLAYER_SERVLET(Msg::CS_LeagueCreateReqMessage, account, session, req)
    {
	    PROFILE_ME;

       	const auto name     	= 	req.name;
        const auto icon        	= 	req.icon;
        const auto content       = 	req.content;
        const auto language 	= 	req.language;
        const auto bshenhe 		= 	req.bautojoin;

        const auto account_uuid = account->get_account_uuid();

        LOG_EMPERY_CENTER_DEBUG("CS_LeagueCreateReqMessage name:", name," icon:",icon, " content:",content,"language:",language," bshenhe:",bshenhe);
        // 判断是否已经加入联盟

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 查看member是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有创建联盟的权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}
        }

        // 判断是否已经存在同名的联盟
        /*
        std::vector<boost::shared_ptr<Legion>> other_legiones;
        LegionMap::get_by_nick(other_legiones,std::move(name));
        if(!other_legiones.empty())
        {
            LOG_EMPERY_CENTER_DEBUG("CS_LeagueCreateReqMessage name = ",name);
            return Response(Msg::ERR_LEGION_CREATE_HOMONYM);
        }
        */

        // 判断创建条件是否满足
        const auto item_box = ItemBoxMap::require(account_uuid);
        std::vector<ItemTransactionElement> transaction;

        // 计算消耗
		const auto& createinfo = Data::Global::as_object(Data::Global::SLOT_LEGAUE_CREATE_NEED);
        for(auto it = createinfo.begin(); it != createinfo.end(); ++it)
        {
            auto str = std::string(it->first.get());
            if(str == boost::lexical_cast<std::string>(ItemIds::ID_DIAMONDS))
            {
                const auto  curDiamonds = item_box->get(ItemIds::ID_DIAMONDS).count;
                const auto creatediamond = boost::lexical_cast<std::uint64_t>(it->second.get<double>());
                if(curDiamonds < creatediamond)
                {
                    return Response(Msg::ERR_LEAGUE_CREATE_NOTENOUGH_MONEY);
                }
                else
                {
                    transaction.emplace_back(ItemTransactionElement::OP_REMOVE, ItemIds::ID_DIAMONDS, creatediamond,
                        ReasonIds::ID_CREATE_LEAGUE, 0, 0, creatediamond);
                }
            }
            else
            {
                LOG_EMPERY_CENTER_DEBUG("CS_LeagueCreateReqMessage: type unsupport  =================================================== ", it->first);
                return Response(Msg::ERR_LEAGUE_CREATE_NOTENOUGH_MONEY);
            }

        }

        // 先扣除资源
        const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
        [&]{
            // 发消息去联盟服务器league

            Msg::SL_LeagueCreated msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.name = req.name;
            msg.icon = req.icon;
            msg.content = req.content;
            msg.language = req.language;
            msg.bautojoin = req.bautojoin;


            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_LeagueCreateReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            if(tresult.first != Msg::ST_OK)
                PLAYER_THROW(tresult.first);

        });

        if(insuff_item_id)
        {
            return Response(Msg::ERR_LEAGUE_CREATE_NOTENOUGH_MONEY);
        }


        return Response(Msg::ST_OK);
    }


    PLAYER_SERVLET(Msg::CS_GetAllLeagueMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            Msg::SL_GetAllLeagueInfo msg;
            msg.account_uuid = account->get_account_uuid().str();
            msg.legion_uuid = member->get_legion_uuid().str();

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_GetAllLeagueMessage LeagueServer response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(Msg::ST_OK);
        }
    }

    PLAYER_SERVLET(Msg::CS_GetLeagueBaseInfoMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        const auto str_league_uuid = account->get_attribute(AccountAttributeIds::ID_LEAGUE_UUID);

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            if(!str_league_uuid.empty())
            {
                boost::container::flat_map<AccountAttributeId, std::string> Attributes;

                Attributes[AccountAttributeIds::ID_LEAGUE_UUID] = "";

                account->set_attributes(std::move(Attributes));
            }
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            Msg::SL_LeagueInfo msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.league_uuid = str_league_uuid;

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("LeagueServer response: code =================== ", tresult.first, ", msg = ", tresult.second);
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_SearchLeagueMessage, account, session, req)
    {
	    PROFILE_ME;

        LOG_EMPERY_CENTER_INFO("CS_SearchLeagueMessage ************************ req.league_name:",req.league_name," req.leader_name:",req.leader_name);

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {

            Msg::SL_SearchLeague msg;
            msg.account_uuid = account_uuid.str();
            msg.league_name = req.league_name;
            msg.legion_uuid = member->get_legion_uuid().str();
            if(!std::move(req.leader_name).empty())
            {
                // 按照盟主昵称来查找
                std::vector<boost::shared_ptr<Account>> accounts;
                AccountMap::get_by_nick(accounts,std::move(req.leader_name));
                LOG_EMPERY_CENTER_INFO("CS_SearchLegionMessage 按照军团长名称玩家数量 找到************************ ",accounts.size());

                for(auto it = accounts.begin(); it != accounts.end(); ++it)
                {
                    // 查看用户是否有军团
                    auto account_member = *it;
                    auto legion_member = LegionMemberMap::get_by_account_uuid(account_member->get_account_uuid());
                    if(legion_member)
                    {
                        msg.legions.reserve(msg.legions.size() + 1);

                        auto &elem = *msg.legions.emplace(msg.legions.end());

                        elem.legion_uuid = legion_member->get_legion_uuid().str();
                    }
                }

                if(msg.legions.empty())
                    return Response(Msg::ERR_LEAGUE_SEARCH_BY_LEADNAME_CANNOT_FIND);
            }

            if( !req.league_name.empty() ||  !msg.legions.empty())
            {
                const auto league = LeagueClient::require();

                auto tresult = league->send_and_wait(msg);
            }

        }

        return Response(Msg::ST_OK);

    }

    PLAYER_SERVLET(Msg::CS_ApplyJoinLeaguenMessage, account, session, req)
    {
	    PROFILE_ME;

        LOG_EMPERY_CENTER_INFO("CS_ApplyJoinLeaguenMessage ************************ req.league_uuid:",req.league_uuid," req.bCancle:",req.bCancle);

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限申请加入联盟
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有创建联盟的权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_ApplyJoinLeague msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.league_uuid = req.league_uuid;
            msg.bCancle = req.bCancle;

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_ApplyJoinLeaguenMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_GetApplyJoinLeague, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限申请加入联盟
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有联盟审批权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_GetApplyJoinLeague msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_GetApplyJoinLeague response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_LeagueAuditingResMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        const auto target_legion_uuid = LegionUuid(req.legion_uuid);

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限审批加入联盟
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有联盟审批权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 查看目标军团是否存在
            const auto& legion = LegionMap::get(target_legion_uuid);
            if(!legion)
            {
                // 军团不存在
				return Response(Msg::ERR_LEGION_CANNOT_FIND);
            }

            // 发消息去联盟服务器league
            Msg::SL_LeagueAuditingRes msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.target_legion_uuid = target_legion_uuid.str();
            msg.bagree = req.bagree;

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_LeagueAuditingResMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_LeagueInviteJoinReqMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限审批加入联盟
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有联盟审批权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 查看目标军团是否存在
            std::vector<boost::shared_ptr<Legion>> legions;
            LegionMap::get_by_nick( legions, std::move(req.legion_name));
            if(legions.empty())
            {
                // 军团不存在
				return Response(Msg::ERR_LEGION_CANNOT_FIND);
            }

            // 发消息去联盟服务器league
            Msg::SL_LeagueInviteJoin msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.target_legion_uuid = legions.at(0)->get_legion_uuid().str();

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_LeagueInviteJoinReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_GetLeagueInviteListMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限审批加入联盟
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有联盟审批权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_LeagueInviteList msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_GetLeagueInviteListMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_LeagueInviteJoinResMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限审批加入联盟
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有联盟审批权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_LeagueInviteJoinRes msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.league_uuid = req.league_uuid;
            msg.bagree = req.bagree;

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_LeagueInviteJoinResMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_ExpandLeague, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有联盟扩张权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_ExpandLeagueReq msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_ExpandLeague response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response(Msg::ST_OK);
    }

    PLAYER_SERVLET(Msg::CS_QuitLeagueReqMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_QuitLeagueReq msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.bCancle = req.bCancle;

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_QuitLeagueReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response();
    }

    PLAYER_SERVLET(Msg::CS_KickLeagueMemberReqMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_KickLeagueMemberReq msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();
            msg.target_legion_uuid = req.legion_uuid;
            msg.bCancle = req.bCancle;

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_KickLeagueMemberReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response();
    }

    PLAYER_SERVLET(Msg::CS_disbandLeagueMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 发消息去联盟服务器league
            Msg::SL_disbandLeague msg;
            msg.account_uuid = account_uuid.str();
            msg.legion_uuid = member->get_legion_uuid().str();

            const auto league = LeagueClient::require();

            auto tresult = league->send_and_wait(msg);

            LOG_EMPERY_CENTER_DEBUG("CS_disbandLeagueMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

            return Response(std::move(tresult.first));
        }

        return Response();
    }

    PLAYER_SERVLET(Msg::CS_AttornLeagueReqMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 指定的军团是否存在
            const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
            if(legion)
            {
                // 发消息去联盟服务器league
                Msg::SL_AttornLeagueReq msg;
                msg.account_uuid = account_uuid.str();
                msg.legion_uuid = member->get_legion_uuid().str();
                msg.target_legion_uuid = req.legion_uuid;
                msg.bCancle = req.bCancle;

                const auto league = LeagueClient::require();

                auto tresult = league->send_and_wait(msg);

                LOG_EMPERY_CENTER_DEBUG("CS_AttornLeagueReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

                return Response(std::move(tresult.first));
            }
            else
            {
                 return Response(Msg::ERR_LEGION_CANNOT_FIND);
            }

        }

        return Response();
    }

    PLAYER_SERVLET(Msg::CS_LeaguenMemberGradeReqMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto& member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 指定的军团是否存在
            const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
            if(legion)
            {
                // 发消息去联盟服务器league
                Msg::SL_LeaguenMemberGradeReq msg;
                msg.account_uuid = account_uuid.str();
                msg.legion_uuid = member->get_legion_uuid().str();
                msg.target_legion_uuid = req.legion_uuid;
                msg.bup = req.bup;

                const auto league = LeagueClient::require();

                auto tresult = league->send_and_wait(msg);

                LOG_EMPERY_CENTER_DEBUG("CS_LeaguenMemberGradeReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

                return Response(std::move(tresult.first));
            }
            else
            {
                 return Response(Msg::ERR_LEGION_CANNOT_FIND);
            }

        }

        return Response();
    }

    PLAYER_SERVLET(Msg::CS_banChatLeagueReqMessage, account, session, req)
    {
	    PROFILE_ME;

        const auto account_uuid = account->get_account_uuid();

        // 判断玩家是否已经有军团
        const auto& member = LegionMemberMap::get_by_account_uuid(account_uuid);
        if(!member)
        {
            return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
        }
        else
        {
            // 是否有权限
			const auto titileid = boost::lexical_cast<std::uint64_t>(member->get_attribute(LegionMemberAttributeIds::ID_TITLEID));
			if(titileid != 1)
			{
                // 不是军团长，没有权限
				return Response(Msg::ERR_LEAGUE_NO_POWER_LEGION);
			}

            // 指定的军团是否存在
            const auto& target_member = LegionMemberMap::get_by_account_uuid(AccountUuid(req.target_account_uuid));
            if(!target_member)
            {
                return Response(Msg::ERR_LEAGUE_NOT_JOIN_LEGION);
            }

            const auto& legion = LegionMap::get(member->get_legion_uuid());
            const auto& target_legion = LegionMap::get(target_member->get_legion_uuid());
            if(legion && target_legion)
            {
                // 发消息去联盟服务器league
                Msg::SL_banChatLeagueReq msg;
                msg.account_uuid = account_uuid.str();
                msg.legion_uuid = member->get_legion_uuid().str();
                msg.target_legion_uuid = target_member->get_legion_uuid().str();
                msg.target_account_uuid = req.target_account_uuid;
                msg.bban = req.bban;

                const auto league = LeagueClient::require();

                auto tresult = league->send_and_wait(msg);

                LOG_EMPERY_CENTER_DEBUG("CS_banChatLeagueReqMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

                return Response(std::move(tresult.first));
            }
            else
            {
                 return Response(Msg::ERR_LEGION_CANNOT_FIND);
            }

        }

        return Response();
    }

    PLAYER_SERVLET(Msg::CS_LookLeagueMembersMessage, account, session, req)
    {
	    PROFILE_ME;

        // 找到军团
		const auto& legion = LegionMap::get(LegionUuid(req.legion_uuid));
		if(legion)
		{
			std::vector<boost::shared_ptr<LegionMember>> members;
			LegionMemberMap::get_by_legion_uuid(members, legion->get_legion_uuid());

            Msg::SC_LookLeagueMembers msg;
            msg.members.reserve(members.size());
            for(auto it = members.begin(); it != members.end(); ++it )
            {
                auto &elem = *msg.members.emplace(msg.members.end());
                auto info = *it;

                elem.account_uuid = info->get_account_uuid().str();
                elem.titleid = info->get_attribute(LegionMemberAttributeIds::ID_TITLEID);

                const auto& legionmember = AccountMap::require(info->get_account_uuid());
                elem.nick = legionmember->get_nick();
                elem.icon = legionmember->get_attribute(AccountAttributeIds::ID_AVATAR);
                if(legionmember->get_attribute(AccountAttributeIds::ID_LEAGUE_CAHT_FALG) != Poseidon::EMPTY_STRING)
                    elem.speakflag = legionmember->get_attribute(AccountAttributeIds::ID_LEAGUE_CAHT_FALG);
                else
                     elem.speakflag = "0";

                const auto primary_castle =  WorldMap::get_primary_castle(info->get_account_uuid());
                if(primary_castle)
                {
                    elem.prosperity = boost::lexical_cast<std::uint64_t>(primary_castle->get_attribute(AttributeIds::ID_PROSPERITY_POINTS));
                }
                else
                {
                    LOG_EMPERY_CENTER_INFO("城堡繁荣度 没找到==============================================");
                    elem.prosperity = 0;   //ID_PROSPERITY_POINTS
                }


            }

            session->send(msg);

            return Response(Msg::ST_OK);

        }
        else
            return Response(Msg::ERR_LEGION_CANNOT_FIND);
    }

     PLAYER_SERVLET(Msg::CS_LookLeagueLegionsMessage, account, session, req)
    {
	    PROFILE_ME;

        // 发消息去联盟服务器league
        Msg::SL_LookLeagueLegionsReq msg;
        msg.account_uuid = account->get_account_uuid().str();
        msg.league_uuid = req.league_uuid;

        const auto league = LeagueClient::require();

        auto tresult = league->send_and_wait(msg);

        LOG_EMPERY_CENTER_DEBUG("CS_LookLeagueLegionsMessage response: code =================== ", tresult.first, ", msg = ", tresult.second);

        return Response(std::move(tresult.first));

    }

}
