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



namespace EmperyLeague {

namespace MySql {
	class League_Info;
	class League_LeagueAttribute;
}


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

public:

	static std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<League>> async_create(
		LeagueUuid league_uuid,  LegionUuid legion_uuid,std::string league_name, AccountUuid account_uuid, std::uint64_t created_time);

private:
	const boost::shared_ptr<MySql::League_Info> m_obj;

	boost::container::flat_map<LeagueAttributeId,
		boost::shared_ptr<MySql::League_LeagueAttribute>> m_attributes;

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

	// 初始化属性
	void InitAttributes(LegionUuid legion_uuid,std::string content, std::string language, std::string icon,unsigned bautojoin);
	// 增加成员
	void AddMember(LegionUuid legion_uuid,AccountUuid account_uuid,unsigned level,std::uint64_t join_time);

	const std::string &get_attribute(LeagueAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<LeagueAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<LeagueAttributeId, std::string> modifiers);

	void synchronize_with_player(const boost::shared_ptr<LeagueSession>& league_client, AccountUuid account_uuid,LegionUuid legion_uuid) const;

	/*
	boost::shared_ptr<LeagueClient> get_league_client() const {
		return m_league_client.lock();
	}
	*/
};

}

#endif
