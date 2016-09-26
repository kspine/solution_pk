#ifndef EMPERY_LEAGUE_LEAGUE_HPP_
#define EMPERY_LEAGUE_LEAGUE_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <poseidon/fwd.hpp>
#include <boost/container/flat_map.hpp>
#include "id_types.hpp"
#include "league_attribute_ids.hpp"
#include "mysql/league.hpp"
#include "league_attribute_ids.hpp"
#include "league_session.hpp"
#include "../../empery_center/src/chat_message_type_ids.hpp"



namespace EmperyLeague {

namespace MySql {
	class League_Info;
	class League_LeagueAttribute;
}

class LeagueSession;

class League : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum LEAGUE_POWER
	{
		LEAGUE_POWER_APPOINT 		= 1,	// 任命
		LEAGUE_POWER_KICKMEMBER 	= 2,	// 移除军团
		LEAGUE_POWER_INVITE 		= 3,	// 邀请加入
		LEAGUE_POWER_APPROVE 		= 4,	// 审批加入
		LEAGUE_POWER_CHAT 			= 5,	// 禁言
		LEAGUE_POWER_QUIT 			= 6,	// 离开联盟
		LEAGUE_POWER_SENDEMAIL 		= 7,	// 发送全联盟邮件
		LEAGUE_POWER_ATTORN 		= 8,	// 转让盟主
		LEAGUE_POWER_GRADEUP 		= 9,	// 可被升级
		LEAGUE_POWER_GRADEDOWN 		= 10,	// 可被降级
		LEAGUE_POWER_CHANGENOTICE 	= 11,	// 修改联盟公告
		LEAGUE_POWER_DISBAND 		= 12,	// 解散联盟
		LEAGUE_POWER_EXPAND 		= 13,	// 联盟扩张
	};

	enum LEAGUE_NOTICE_MSG_TYPE
	{
		LEAGUE_NOTICE_MSG_TYPE_JOIN 			= 1,	// 有成员加入
		LEAGUE_NOTICE_MSG_TYPE_QUIT 			= 2,	// 有成员离开
		LEAGUE_NOTICE_MSG_TYPE_KICK 			= 3,	// 有成员被踢出
		LEAGUE_NOTICE_MSG_TYPE_GRADE_UP 		= 4,	// 成员职位升级
		LEAGUE_NOTICE_MSG_TYPE_GRADE_DOWN 		= 5,	// 成员职位降级
		LEAGUE_NOTICE_MSG_TYPE_ATTORN 			= 6,	// 转让盟主
		LEAGUE_NOTICE_MSG_TYPE_SPEACK 			= 7,	// 禁言
		LEAGUE_NOTICE_MSG_TYPE_EXPAND  			= 8,	// 联盟扩张
		LEAGUE_NOTICE_MSG_TYPE_DISBAND  		= 9,	// 联盟解散
		LEAGUE_NOTICE_MSG_CREATE_SUCCESS  		= 10,	// 联盟创建成功
	};

public:

	static std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<League>> async_create(
		LeagueUuid league_uuid,  LegionUuid legion_uuid,std::string league_name, AccountUuid account_uuid, std::uint64_t created_time);

private:
	const boost::shared_ptr<MySql::League_Info> m_obj;

	boost::container::flat_map<LeagueAttributeId,
		boost::shared_ptr<MySql::League_LeagueAttribute>> m_attributes;

	boost::weak_ptr<LeagueSession> m_leaguesession;

public:
	League(boost::shared_ptr<MySql::League_Info> obj,
		const std::vector<boost::shared_ptr<MySql::League_LeagueAttribute>> &attributes);
	~League();

public:
	virtual void pump_status();

	LeagueUuid get_league_uuid() const {
		return LeagueUuid(std::move(m_obj->unlocked_get_league_uuid()));
	}

	LegionUuid get_legion_uuid() const {
		return LegionUuid(std::move(m_obj->unlocked_get_legion_uuid()));
	}

	AccountUuid get_create_uuid() const {
		return AccountUuid(std::move(m_obj->unlocked_get_creater_uuid()));
	}
	void set_founder_uuid(AccountUuid founder_uuid);

	std::uint64_t get_create_league_time(){
		return m_obj->unlocked_get_created_time();
	}

	const std::string &get_nick() const;

	const boost::weak_ptr<LeagueSession> &get_weak_controller() const {
		return m_leaguesession;
	}
	boost::shared_ptr<LeagueSession> get_controller() const {
		return m_leaguesession.lock();
	}
	void set_controller(const boost::shared_ptr<LeagueSession> &controller);

	// 初始化属性
	void InitAttributes(LegionUuid legion_uuid,std::string content, std::string language, std::string icon,unsigned bautojoin);
	// 增加成员
	void AddMember(LegionUuid legion_uuid,AccountUuid account_uuid,unsigned level,std::uint64_t join_time);

	// 联盟解散的善后操作
	void disband();

	// 发送通知消息
	void sendNoticeMsg(std::uint64_t msgtype,std::string nick,std::string ext1);

	// 发邮件
	void sendemail(EmperyCenter::ChatMessageTypeId ntype,LegionUuid legion_uuid,std::string strnick,std::string str_account_uuid = "");

	const std::string &get_attribute(LeagueAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<LeagueAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<LeagueAttributeId, std::string> modifiers);

	void synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client, AccountUuid account_uuid,LegionUuid legion_uuid, std::string str_league_uuid = "") const;

	/*
	boost::shared_ptr<LeagueClient> get_league_client() const {
		return m_league_client.lock();
	}
	*/
};

}

#endif
