#include "../precompiled.hpp"
#include "league_map.hpp"
#include "../mmain.hpp"
#include "../mysql/league.hpp"
#include "../league.hpp"
#include "../league_attribute_ids.hpp"
#include "../string_utilities.hpp"
#include "league_member_map.hpp"
#include "league_map.hpp"
#include "../league_member.hpp"
#include "league_map.hpp"
#include "../league_member_attribute_ids.hpp"
#include "../data/league_power.hpp"
#include <poseidon/async_job.hpp>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <tuple>

namespace EmperyLeague {

namespace {
	struct LeagueMemberElement {
		boost::shared_ptr<LeagueMember> member;

		LeagueUuid league_uuid;
		LegionUuid legion_uuid;

		explicit LeagueMemberElement(boost::shared_ptr<LeagueMember> member_)
			: member(std::move(member_))
			, league_uuid(member->get_league_uuid())
			, legion_uuid(member->get_legion_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(LeagueMemberContainer, LeagueMemberElement,
		MULTI_MEMBER_INDEX(league_uuid)
		MULTI_MEMBER_INDEX(legion_uuid)
	)

	boost::shared_ptr<LeagueMemberContainer> g_LeagueMember_map;

	enum CacheType : std::uint64_t {
		CT_NONE       = 0, // Silent the warning.
		CT_QUIT       = 1,
		CT_DISBAND    = 2,
	};

	struct InfoCacheElement {
		std::tuple<LeagueUuid, boost::weak_ptr<LeagueMember>, CacheType> key;
		std::uint64_t expiry_time;

		InfoCacheElement(LeagueUuid league_uuid_, const boost::shared_ptr<LeagueMember> &member_, CacheType type_,
			std::uint64_t expiry_time_)
			: key(league_uuid_, member_, type_), expiry_time(expiry_time_)
		{
		}
	};

	MULTI_INDEX_MAP(InfoCacheContainer, InfoCacheElement,
		UNIQUE_MEMBER_INDEX(key)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<InfoCacheContainer> g_info_cache_map;

	void gc_member_proc(std::uint64_t now){
		PROFILE_ME;
	//	LOG_EMPERY_CENTER_ERROR("league member map gc timer: now =================================================== ", now);

		const auto &account_map = g_LeagueMember_map;
		if(!account_map){
			return;
		}

		LeagueMemberMap::check_in_waittime();
	}

	void gc_member_reset_proc(std::uint64_t now){
		PROFILE_ME;

		const auto &account_map = g_LeagueMember_map;
		if(!account_map){
			return;
		}

		LeagueMemberMap::check_in_resetime();
	}

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// LeagueMember
		struct TempLeagueMemberElement {
			boost::shared_ptr<MySql::League_Member> obj;
			std::vector<boost::shared_ptr<MySql::League_MemberAttribute>> attributes;
			std::vector<boost::shared_ptr<LeagueMember>> members;
		};
		std::map<LegionUuid, TempLeagueMemberElement> temp_account_map;


		LOG_EMPERY_LEAGUE_INFO("Loading LeagueMembers...");
		conn->execute_sql("SELECT * FROM `League_Member`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::League_Member>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto legion_uuid = LegionUuid(obj->get_legion_uuid());
			temp_account_map[legion_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_LEAGUE_INFO("Loaded ", temp_account_map.size(), " LeagueMember(s).");

		LOG_EMPERY_LEAGUE_INFO("Done loading league members.",temp_account_map.size());

	//	sleep(5000);

		LOG_EMPERY_LEAGUE_INFO("Loading LeagueMember attributes...");
		conn->execute_sql("SELECT * FROM `League_MemberAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::League_MemberAttribute>();
			obj->fetch(conn);
			const auto legion_uuid = LegionUuid(obj->unlocked_get_legion_uuid());
			const auto it = temp_account_map.find(legion_uuid);
			if(it == temp_account_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.attributes.emplace_back(std::move(obj));
		}

		LOG_EMPERY_LEAGUE_INFO("Done loading LeagueMember attributes.");

		const auto LeagueMember_map = boost::make_shared<LeagueMemberContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			auto member = boost::make_shared<LeagueMember>(std::move(it->second.obj), it->second.attributes);

			LeagueMember_map->insert(LeagueMemberElement(std::move(member)));
		}
		g_LeagueMember_map = LeagueMember_map;
		handles.push(LeagueMember_map);

		const auto info_cache_map = boost::make_shared<InfoCacheContainer>();
		g_info_cache_map = info_cache_map;
		handles.push(info_cache_map);

	//	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 10*1000);
		// 1秒检测
		auto timer = Poseidon::TimerDaemon::register_timer(0, 1*1000,
			std::bind(&gc_member_proc, std::placeholders::_2));
		handles.push(timer);

		/*
		// 30秒检测
		auto timer2 = Poseidon::TimerDaemon::register_timer(0, 30*1000,
			std::bind(&gc_member_reset_proc, std::placeholders::_2));
		handles.push(timer2);
		*/
	}

	boost::shared_ptr<LeagueMember> reload_account_aux(boost::shared_ptr<MySql::League_Member> obj){
		PROFILE_ME;

		std::deque<boost::shared_ptr<const Poseidon::JobPromise>> promises;

		const auto attributes = boost::make_shared<std::vector<boost::shared_ptr<MySql::League_MemberAttribute>>>();

#define RELOAD_PART_(sink_, table_)	\
		{	\
			std::ostringstream oss;	\
			const auto account_uuid = obj->unlocked_get_account_uuid();	\
			oss <<"SELECT * FROM `" #table_ "` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid);	\
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(	\
				[sink_](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){	\
					auto obj = boost::make_shared<MySql:: table_ >();	\
					obj->fetch(conn);	\
					obj->enable_auto_saving();	\
					(sink_)->emplace_back(std::move(obj));	\
				}, #table_, oss.str());	\
			promises.emplace_back(std::move(promise));	\
		}
//=============================================================================
	//	RELOAD_PART_(attributes,         League_MemberAttribute)
//=============================================================================
		for(const auto &promise : promises){
			Poseidon::JobDispatcher::yield(promise, true);
		}
#undef RELOAD_PART_

		return boost::make_shared<LeagueMember>(std::move(obj), *attributes);
	}

	template<typename IteratorT>
	void really_append_account(std::vector<boost::shared_ptr<LeagueMember>> &ret,
		const std::pair<IteratorT, IteratorT> &range, std::uint64_t begin, std::uint64_t count)
	{
		PROFILE_ME;

		auto lower = range.first;
		for(std::uint64_t i = 0; i < begin; ++i){
			if(lower == range.second){
				break;
			}
			++lower;
		}

		auto upper = lower;
		for(std::uint64_t i = 0; i < count; ++i){
			if(upper == range.second){
				break;
			}
			++upper;
		}

		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(lower, upper)));
		for(auto it = lower; it != upper; ++it){
			ret.emplace_back(it->member);
		}
	}
}

bool LeagueMemberMap::is_holding_controller_token(LeagueUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("league map not loaded.");
		return false;
	}

	/*
	const auto controller = ControllerClient::require();

	Msg::ST_LeagueMemberQueryToken treq;
	treq.account_uuid = account_uuid.str();
	auto tresult = controller->send_and_wait(treq);
	LOG_EMPERY_LEAGUE_DEBUG("Controller response: code = ", tresult.first, ", msg = ", tresult.second);
	if(tresult.first != 0){
		return false;
	}
	*/
	return true;
}


boost::shared_ptr<LeagueMember> LeagueMemberMap::get(LeagueUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("league map not loaded.");
		return { };
	}

	const auto it = account_map->find<0>(account_uuid);
	if(it == account_map->end<0>()){
		LOG_EMPERY_LEAGUE_TRACE("league not found: account_uuid = ", account_uuid);
		return { };
	}
	return it->member;
}
boost::shared_ptr<LeagueMember> LeagueMemberMap::require(LeagueUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMember not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("LeagueMember not found"));
	}
	return account;
}

boost::shared_ptr<LeagueMember> LeagueMemberMap::get_by_legion_uuid(LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("league map not loaded.");
		return { };
	}

	const auto it = account_map->find<1>(legion_uuid);
	if(it == account_map->end<1>()){
		LOG_EMPERY_LEAGUE_TRACE("league not found: legion_uuid = ", legion_uuid);
		return { };
	}
	return it->member;
}

void LeagueMemberMap::get_member_by_titleid(std::vector<boost::shared_ptr<LeagueMember>> &ret, LeagueUuid league_uuid,std::uint64_t level)
{
	PROFILE_ME;
	std::vector<boost::shared_ptr<LeagueMember>> members;
	get_by_league_uuid(members,league_uuid);
	ret.reserve(ret.size() + members.size());
	for(auto it = members.begin(); it != members.end(); ++it)
	{
		if(boost::lexical_cast<uint>((*it)->get_attribute(LeagueMemberAttributeIds::ID_TITLEID)) == level)
		{
			ret.reserve(ret.size() + 1);
			ret.emplace_back(*it);
		}
	}
}

void LeagueMemberMap::get_by_league_uuid(std::vector<boost::shared_ptr<LeagueMember>> &ret, LeagueUuid league_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("league map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<0>(league_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->member);
	}
}

std::uint64_t LeagueMemberMap::get_count(){
	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("Center_LeagueMember map not loaded.");
		return 0;
	}

	return account_map->size();
}

std::uint64_t LeagueMemberMap::get_league_member_count(LeagueUuid league_uuid)
{
	std::vector<boost::shared_ptr<LeagueMember>> members;
	get_by_league_uuid(members, league_uuid);

	return members.size();

}

void LeagueMemberMap::get_all(std::vector<boost::shared_ptr<LeagueMember>> &ret, std::uint64_t begin, std::uint64_t count){
	PROFILE_ME;

	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMember map not loaded.");
		return;
	}

	really_append_account(ret,
		std::make_pair(account_map->begin<0>(), account_map->end<0>()),
		begin, count);
}


void LeagueMemberMap::insert(const boost::shared_ptr<LeagueMember> &league, const std::string &remote_ip){
	PROFILE_ME;

	const auto &league_map = g_LeagueMember_map;
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMember map not loaded.");
		DEBUG_THROW(Exception, sslit("LeagueMember map not loaded"));
	}


	const auto legion_uuid = league->get_legion_uuid();

	// 判断是否已存在
	const auto member = get_by_legion_uuid(legion_uuid);
	if(member)
	{
		LOG_EMPERY_LEAGUE_WARNING("LeagueMember already exists: legion_uuid = ", legion_uuid);
		DEBUG_THROW(Exception, sslit("LeagueMember already exists"));
	}
	else
	{
		league_map->insert(LeagueMemberElement(league));

		LOG_EMPERY_LEAGUE_DEBUG("Inserting league: legion_uuid = ", legion_uuid);
	}

	LOG_EMPERY_LEAGUE_DEBUG("Inserting LeagueMemberMap size = ", get_league_member_count(league->get_league_uuid()));

}

void LeagueMemberMap::deletemember(LegionUuid legion_uuid,bool bdeletemap)
{
	// 先从内存中删，然后删数据库的
	PROFILE_ME;

	const auto &league_map = g_LeagueMember_map;
	if(!league_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMember map not loaded.");
		DEBUG_THROW(Exception, sslit("LeagueMember map not loaded"));
	}

	const auto it = league_map->find<1>(legion_uuid);
	if(it != league_map->end<1>()){

		it->member->leave();

		if(bdeletemap)
			league_map->erase<1>(it);

		// 从数据库中删除该成员
		std::string strsql = "DELETE FROM League_Member WHERE legion_uuid='";
		strsql += legion_uuid.str();
		strsql += "';";

		Poseidon::MySqlDaemon::enqueue_for_deleting("League_Member",strsql);
	}

}


void LeagueMemberMap::disband_league(LeagueUuid league_uuid)
{
	std::vector<boost::shared_ptr<LeagueMember>> members;
	get_by_league_uuid(members, league_uuid);

	for(auto it = members.begin(); it != members.end();)
	{
		const auto &member = *it;
		deletemember(member->get_legion_uuid());
		it = members.erase(it);
	}

}

void LeagueMemberMap::update(const boost::shared_ptr<LeagueMember> &account, bool throws_if_not_exists){
	PROFILE_ME;

	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMember map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("LeagueMember map not loaded"));
		}
		return;
	}


	const auto legion_uuid = account->get_legion_uuid();

	const auto it = account_map->find<1>(legion_uuid);
	if(it == account_map->end<1>()){
		LOG_EMPERY_LEAGUE_WARNING("LeagueMember not found: legion_uuid = ", legion_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("LeagueMember not found"));
		}
		return;
	}
	if(it->member != account){
		LOG_EMPERY_LEAGUE_DEBUG("LeagueMember expired: legion_uuid = ", legion_uuid);
		return;
	}

	LOG_EMPERY_LEAGUE_DEBUG("Updating LeagueMember: legion_uuid = ", legion_uuid);
	account_map->replace<1>(it, LeagueMemberElement(account));

	/*
	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			account->synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_LEAGUE_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	*/
}


bool LeagueMemberMap::is_in_same_league(LegionUuid legion_uuid,LegionUuid other_uuid)
{
	PROFILE_ME;

	const auto member = get_by_legion_uuid(legion_uuid);
	const auto member2 = get_by_legion_uuid(other_uuid);
	if(member && member2 && member->get_league_uuid() == member2->get_league_uuid())
	{
		return true;
	}

	return false;
}

void LeagueMemberMap::check_in_waittime()
{
	PROFILE_ME;

	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		return;
	}

	for(auto it = account_map->begin(); it != account_map->end();)
	{
		const auto &member = it->member;

		bool bdelete = false;
		// 是否有联盟
		const auto league = LeagueMap::get(LeagueUuid(member->get_league_uuid()));
		if(league)
		{
		//	const auto account = AccountMap::get(member->get_account_uuid());
			const auto titileid = member->get_attribute(LeagueMemberAttributeIds::ID_TITLEID);
		//	LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage titileid ================================ ",titileid,"  成员uuid：",member->get_account_uuid());
			// 查看member权限
			if(Data::LeaguePower::is_have_power(boost::lexical_cast<std::uint64_t>(titileid),League::LEAGUE_POWER::LEAGUE_POWER_QUIT))
			{
		//		LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage 可以退出================================ ");
				// 是否已经在退会等待中、被踢出等待中
				auto quittime = member->get_attribute(LeagueMemberAttributeIds::ID_QUITWAITTIME);
		//		LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage quittime= 111111111=============================== ",quittime);
				bool bkick = false;
				if(quittime.empty()  || quittime == Poseidon::EMPTY_STRING)
				{
					// 被踢出等待中
					quittime = member->get_attribute(LeagueMemberAttributeIds::ID_KICKWAITTIME);
					if(quittime != Poseidon::EMPTY_STRING)
						bkick = true;
				}
		//		LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage quittime= 2222222=============================== ",quittime);
		//		LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage quittime= 2222222=============================== ",quittime," bkick:",bkick);
				if(quittime != Poseidon::EMPTY_STRING)
				{
					const auto utc_now = Poseidon::get_utc_time();

					const auto leavetime = boost::lexical_cast<boost::uint64_t>(quittime) * 1000;

		//			LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage utc_now ================ ",utc_now," quittime==============",quittime, " leavetime==============",leavetime);

					if(utc_now > leavetime )
					{
						// 已经过了完全离开的时间，让玩家离开联盟
						/*
						const auto account_uuid = member->get_account_uuid();

						const auto target_account = AccountMap::get(account_uuid);
						if(target_account)
						{
							// 广播给联盟其他成员
							Msg::SC_leagueNoticeMsg msg;
							if(bkick)
							{
								// 被踢出时发邮件
								league->sendmail(target_account,ChatMessageTypeIds::ID_LEVEL_league_KICK,league->get_nick() + ","+ account->get_nick());
								msg.msgtype = league::league_NOTICE_MSG_TYPE::league_NOTICE_MSG_TYPE_KICK;
							}
							else
								msg.msgtype = league::league_NOTICE_MSG_TYPE::league_NOTICE_MSG_TYPE_QUIT;
							msg.nick = target_account->get_nick();
							msg.ext1 = "";
							league->sendNoticeMsg(msg);
						}
						*/
						// 广播通知下
						if(bkick)
						{
							// 发邮件
							league->sendemail(EmperyCenter::ChatMessageTypeIds::ID_LEVEL_LEAGUE_KICK,member->get_legion_uuid(),league->get_nick(), member->get_attribute(LeagueMemberAttributeIds::ID_KICK_MANDATOR));

							league->sendNoticeMsg(League::LEAGUE_NOTICE_MSG_TYPE::LEAGUE_NOTICE_MSG_TYPE_KICK,league->get_nick(),member->get_legion_uuid().str());
						}
						else
						{
							league->sendNoticeMsg(League::LEAGUE_NOTICE_MSG_TYPE::LEAGUE_NOTICE_MSG_TYPE_QUIT,"",member->get_legion_uuid().str());
						}

						LeagueMemberMap::deletemember(member->get_legion_uuid(),false);

						bdelete = true;
					}
				}
			}


			// 看下是否转让军团长等待时间已过的逻辑
			bool bAttorn = false;
			std::string strlead="";
			if(Data::LeaguePower::is_have_power(boost::lexical_cast<std::uint64_t>(titileid),League::LEAGUE_POWER::LEAGUE_POWER_ATTORN))
			{
				const auto utc_now = Poseidon::get_utc_time();
			//	LOG_EMPERY_LEAGUE_DEBUG("有转让权限 titileid ================================ ",titileid,"  成员uuid：", member->get_account_uuid());
				// 转让军团等待中
				const auto 	quittime = league->get_attribute(LeagueAttributeIds::ID_ATTORNTIME);
				if(!quittime.empty() ||  quittime != Poseidon::EMPTY_STRING)
				{
					// 先看下要转让的目标对象是否还在军团中
					const auto target_uuid = league->get_attribute(LeagueAttributeIds::ID_ATTORNLEADER);
			//		LOG_EMPERY_LEAGUE_DEBUG("存在转让等待时间=============================== ",target_uuid);
					// 查看两个人是否属于同一军团
					if(LeagueMemberMap::is_in_same_league(member->get_legion_uuid(),LegionUuid(target_uuid)))
					{
						const auto target_member =  LeagueMemberMap::get_by_legion_uuid(LegionUuid(target_uuid));
						// 设置两者新的等级关系
						if(target_member)
						{
							// 查看目标对象是否有离开倒计时或者被踢出的倒计时
							auto target_quittime = target_member->get_attribute(LeagueMemberAttributeIds::ID_QUITWAITTIME);
		//					LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage quittime= 111111111=============================== ",target_quittime);
							if(target_quittime.empty()  || target_quittime == Poseidon::EMPTY_STRING)
							{
								// 被踢出等待中
								target_quittime = target_member->get_attribute(LeagueMemberAttributeIds::ID_KICKWAITTIME);
							}
		//					LOG_EMPERY_LEAGUE_DEBUG("CS_GetleagueBaseInfoMessage quittime= 2222222=============================== ",target_quittime);
							if(!target_quittime.empty() || target_quittime != Poseidon::EMPTY_STRING)
							{
								// 因为目标对象要退出军团，所以无法转让
								LOG_EMPERY_LEAGUE_DEBUG("因为目标对象要退出军团，所以无法转让=============================== ");
								bAttorn = true;
							}
							else
							{
								// 查看时间是否已经超过转让等待时间
								const auto leavetime = boost::lexical_cast<boost::uint64_t>(quittime) * 1000;
								if(utc_now > leavetime )
								{
									// 设置各自的等级
									boost::container::flat_map<LeagueMemberAttributeId, std::string> Attributes;

									Attributes[LeagueMemberAttributeIds::ID_TITLEID] = "1";

									target_member->set_attributes(Attributes);

									// 设置原来盟主为普通成员
									boost::container::flat_map<LeagueMemberAttributeId, std::string> Attributes1;

									Attributes1[LeagueMemberAttributeIds::ID_TITLEID] = "3";

									member->set_attributes(Attributes1);

									/*
									// 广播给军团其他成员
									const auto target_account = AccountMap::get(target_member->get_account_uuid());
									if(target_account && account)
									{
										Msg::SC_leagueNoticeMsg msg;
										msg.msgtype = league::league_NOTICE_MSG_TYPE::league_NOTICE_MSG_TYPE_ATTORN;
										msg.nick = target_account->get_nick();
										msg.ext1 = account->get_nick();
										league->sendNoticeMsg(msg);
									}

									*/
									LOG_EMPERY_LEAGUE_DEBUG("成功转让=============================== ",member->get_legion_uuid(), "  目标对象：",target_member->get_legion_uuid());
									// 转让成功，重置转让等待时间
									bAttorn = true;
									strlead = LegionUuid(target_uuid).str();
									// 广播通知下
									league->sendNoticeMsg(League::LEAGUE_NOTICE_MSG_TYPE::LEAGUE_NOTICE_MSG_TYPE_ATTORN,member->get_legion_uuid().str(),target_member->get_legion_uuid().str());
								}
							}
						}
						else
						{
							// 目标不存在无法转让
							LOG_EMPERY_LEAGUE_DEBUG("没找到目标成员，所以无法转让=============================== ");
							bAttorn = true;
						}

					}
					else
					{
						// 要转让的目标已经和自己不属于同一军团了
						LOG_EMPERY_LEAGUE_DEBUG("要转让的目标已经和自己不属于同一军团了，所以无法转让=============================== ");
						bAttorn = true;
					}
				}
			}

			if(bAttorn)
			{
				// 重置转让信息
				boost::container::flat_map<LeagueAttributeId, std::string> Attributes;
				if(!strlead.empty())
					Attributes[LeagueAttributeIds::ID_LEADER] = strlead;
				Attributes[LeagueAttributeIds::ID_ATTORNTIME] = "";
				Attributes[LeagueAttributeIds::ID_ATTORNLEADER] = "";

				league->set_attributes(Attributes);

		//		break;
			}

		}

		if(bdelete)
		{
			// 如果需要清空
			it = account_map->erase(it);
		}
		else
		{
			++it;
		}
	}
}

uint64_t CaculateWeekDay(unsigned y, unsigned m, unsigned d)
{
    if(m==1||m==2) //把一月和二月换算成上一年的十三月和是四月
    {
        m+=12;
        y--;
    }
	// 返回1就是周一 返回7就是周日
    return (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7 + 1;
}

void LeagueMemberMap::check_in_resetime()
{
	PROFILE_ME;
	/*
	const auto &account_map = g_LeagueMember_map;
	if(!account_map){
		return;
	}


	const auto utc_now = Poseidon::get_utc_time();
	const auto dt = Poseidon::break_down_time(utc_now);

	for(auto it = account_map->begin(); it != account_map->end(); ++it)
	{
		const auto &member = it->member;

//		LOG_EMPERY_CENTER_ERROR("check_in_resetime dt hr=================================================== ",  dt.hr, "  min:",dt.min, " dt.sec:",dt.sec);
//		if(4 == dt.hr && 18 == dt.min )
		if(dt.hr == Data::Global::as_unsigned(Data::Global::SLOT_league_STORE_UPDATE_MINUTE) / 60 && dt.min == Data::Global::as_unsigned(Data::Global::SLOT_league_STORE_UPDATE_MINUTE) % 60)
		{
			// 日购买记录的清空重置
			const auto exchange_record = member->get_attribute(LeagueMemberAttributeIds::ID_league_STORE_EXCHANGE_RECORD);
		//	LOG_EMPERY_CENTER_ERROR("成员uuid：",member->get_account_uuid()," 购买记录:",exchange_record);
			if(!exchange_record.empty()  && exchange_record != Poseidon::EMPTY_STRING)
			{
				boost::container::flat_map<LeagueMemberAttributeId, std::string> Attributes;

				Attributes[LeagueMemberAttributeIds::ID_league_STORE_EXCHANGE_RECORD] = "";

				member->set_attributes(Attributes);

		//		LOG_EMPERY_CENTER_ERROR("时间到，清空购买记录的清空重置");
			}
		}
	}

	// 周捐献记录的清空重置
//	LOG_EMPERY_CENTER_ERROR("check_in_resetime check week ",  dt.yr, "  min:",dt.mon, " dt.sec:",dt.day, " CaculateWeekDay:",CaculateWeekDay(dt.yr, dt.mon,dt.day));
	if(CaculateWeekDay(dt.yr, dt.mon,dt.day) == Data::Global::as_unsigned(Data::Global::SLOT_league_WEEK_DONATE_DAY))
	{
		for(auto it = account_map->begin(); it != account_map->end(); ++it)
		{
			const auto &member = it->member;

			if(dt.hr == Data::Global::as_unsigned(Data::Global::SLOT_league_WEEK_DONATE_UPDATETIME) / 60 && dt.min == Data::Global::as_unsigned(Data::Global::SLOT_league_WEEK_DONATE_UPDATETIME) % 60)
			{
				const auto weejdonate = member->get_attribute(LeagueMemberAttributeIds::ID_WEEKDONATE);
			//	LOG_EMPERY_CENTER_ERROR("成员uuid：",member->get_account_uuid()," 周捐献:",weejdonate);
				if(!weejdonate.empty()  && weejdonate != Poseidon::EMPTY_STRING)
				{
					boost::container::flat_map<LeagueMemberAttributeId, std::string> Attributes;

					Attributes[LeagueMemberAttributeIds::ID_WEEKDONATE] = "";

					member->set_attributes(Attributes);

			//		LOG_EMPERY_CENTER_ERROR("时间到，清空周捐献记录的清空重置");
				}
			}
		}
	}
*/
}

/*
int LeagueMemberMap::chat(const boost::shared_ptr<LeagueMember> &member,const boost::shared_ptr<ChatMessage> &message)
{

	if(member->get_attribute(LeagueMemberAttributeIds::ID_SPEAKFLAG) == "1" )
	{
		return Msg::ERR_league_BAN_CHAT;
	}
	else
	{
		// 根据account_uuid查找是否有军团
		const auto league = leagueMap::get(LeagueUuid(member->get_league_uuid()));
		if(league)
		{
			// 根据account_uuid查找是否有军团
			std::vector<boost::shared_ptr<LeagueMember>> members;
			LeagueMemberMap::get_by_league_uuid(members, member->get_league_uuid());

			LOG_EMPERY_LEAGUE_INFO("get_by_league_uuid league members size*******************",members.size());

			if(!members.empty())
			{

				for(auto it = members.begin(); it != members.end(); ++it )
				{
					auto info = *it;

					// 判断是否在线
					const auto target_session = PlayerSessionMap::get(AccountUuid(info->get_account_uuid()));
					if(target_session)
					{
						try {
						Poseidon::enqueue_async_job(
							[=]() mutable {
								PROFILE_ME;
								const auto other_chat_box = ChatBoxMap::require(info->get_account_uuid());
								other_chat_box->insert(message);

				//				LOG_EMPERY_LEAGUE_INFO("LeagueMemberMap::chat 有人说话  内容已转发============");
							});
						} catch(std::exception &e){
							LOG_EMPERY_LEAGUE_WARNING("std::exception thrown: what = ", e.what());
							target_session->shutdown(e.what());
						}
					}
				}
			}
			else
			{
				return Msg::ERR_league_CANNOT_FIND;
			}
		}
		else
		{
			return Msg::ERR_league_CANNOT_FIND;
		}
	}


	return Msg::ST_OK;
}
*/


}
