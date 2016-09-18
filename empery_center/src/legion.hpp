#ifndef EMPERY_CENTER_LEGION_HPP_
#define EMPERY_CENTER_LEGION_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/lexical_cast.hpp>
#include "id_types.hpp"
#include "msg/sc_legion.hpp"
#include "singletons/legion_member_map.hpp"
#include "legion_member_attribute_ids.hpp"
#include "legion_member.hpp"
#include "singletons/player_session_map.hpp"
#include "data/legion_corps_power.hpp"
#include "player_session.hpp"


namespace EmperyCenter {

namespace MySql {
	class Center_Legion;
	class Center_LegionAttribute;
	class Center_Legion_Member;
}

class PlayerSession;
class LegionMember;
class Account;

class Legion : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum LEGION_POWER
	{
		LEGION_POWER_ATTORN 		= 1,	// 转让军团长
		LEGION_POWER_DISBAND 		= 2,	// 解散军团
		LEGION_POWER_SENDEMAIL	 	= 3,	// 全军团发送邮件
		LEGION_POWER_MEMBERGRADE	= 4,	// 升降级成员级别
		LEGION_POWER_BUILDMINE  	= 5,	// 建造矿井
		LEGION_POWER_LEVELUPMINE 	= 6,	// 升级矿井
		LEGION_POWER_OPENMINE 		= 7,	// 开启矿井
		LEGION_POWER_REPAIRMINE 	= 8,	// 维修矿井
		LEGION_POWER_INVITE 		= 9,	// 邀请玩家
		LEGION_POWER_APPROVE 		= 10,	// 审批玩家
		LEGION_POWER_KICKMEMBER 	= 11,	// 移除玩家
		LEGION_POWER_OPENCOPY 		= 12,	// 开启军团副本
		LEGION_POWER_GRADE 			= 13,	// 升级军团
		LEGION_POWER_QUIT 			= 14,	// 离开军团
		LEGION_POWER_TASK 			= 15,	// 领取军团任务
		LEGION_POWER_GIFT 			= 16,	// 领取军团礼包
		LEGION_POWER_JOINCOPY       = 17,	// 参与军团副本
		LEGION_POWER_GATHER 		= 18,	// 采集军团资源
		LEGION_POWER_CHANGENOTICE 	= 19,	// 修改军团公告
		LEGION_POWER_CHAT 			= 20,	// 禁言
		LEGION_POWER_GRADEUP 		= 21,	// 可被升级
		LEGION_POWER_GRADEDOWN 		= 22,	// 可被降级
		LEGION_POWER_LEAGUE_INVITE 	= 23,	// 可以处理邀请加入军团
		LEGION_POWER_DEMOLISH_MINE 	= 24,	// 拆除军团矿井
	};

	enum LEGION_NOTICE_MSG_TYPE
	{
		LEGION_NOTICE_MSG_TYPE_JOIN 			= 1,	// 有成员加入
		LEGION_NOTICE_MSG_TYPE_QUIT 			= 2,	// 有成员离开
		LEGION_NOTICE_MSG_TYPE_KICK 			= 3,	// 有成员被踢出
		LEGION_NOTICE_MSG_TYPE_GRADE 			= 4,	// 成员职位变动
		LEGION_NOTICE_MSG_TYPE_ATTORN 			= 5,	// 转让军团长
		LEGION_NOTICE_MSG_TYPE_SPEACK 			= 6,	// 禁言
		LEGION_NOTICE_MSG_TYPE_CHAT 			= 7,	// 聊天
		LEGION_NOTICE_MSG_TYPE_DONATE 			= 8,	// 个人捐献
		LEGION_NOTICE_MSG_TYPE_LEGION_GRADE  	= 9,	// 军团升级
		LEGION_NOTICE_MSG_TYPE_CHANGENOTICE  	= 10,	// 修改军团公告
		LEGION_NOTICE_MSG_TYPE_LEAGUE_KICK  	= 11,	// 军团被踢出联盟
		LEGION_NOTICE_MSG_TYPE_TASK_CHANGE  	= 12,	// 任务进度有改变
		LEGION_NOTICE_MSG_CREATE_SUCCESS  	    = 13,	// 创建成功
	};
public:

	static std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<Legion>> async_create(
		LegionUuid legion_uuid,  std::string legion_name, AccountUuid account_uuid, std::uint64_t created_time);

private:
	const boost::shared_ptr<MySql::Center_Legion> m_obj;

	boost::container::flat_map<LegionAttributeId,
		boost::shared_ptr<MySql::Center_LegionAttribute>> m_attributes;

public:
	Legion(boost::shared_ptr<MySql::Center_Legion> obj,
		const std::vector<boost::shared_ptr<MySql::Center_LegionAttribute>> &attributes);
	~Legion();

public:
	LegionUuid get_legion_uuid() const;
	AccountUuid get_account_uuid() const;
//	const std::string &get_login_name() const;
	LegionUuid get_creater_uuid() const;

//	unsigned get_promotion_level() const;
	void set_promotion_level(unsigned promotion_level);

	std::uint64_t get_created_time() const;

	const std::string &get_nick() const;
	void set_nick(std::string nick);

	boost::shared_ptr<LegionMember> getmember(AccountUuid accout_uuid)  const;
	// 初始化属性
	void InitAttributes(AccountUuid accountid,std::string content, std::string language, std::string icon,unsigned bshenhe);
	// 增加军团成员
	void AddMember(boost::shared_ptr<Account> account,unsigned level,std::uint64_t join_time);
	// 军团解散的善后操作
	void disband();
	// 发送通知消息
	void sendNoticeMsg(Msg::SC_LegionNoticeMsg msg);

	// 发邮件
	void sendmail(boost::shared_ptr<Account> account, ChatMessageTypeId ntype,std::string);
	// 设置军团成员的联盟uuid字段
	void set_member_league_uuid(std::string str_league_uuid);

/*	bool has_been_activated() const;
	void set_activated(bool activated);

	std::uint64_t get_banned_until() const;
	void set_banned_until(std::uint64_t banned_until);

	std::uint64_t get_quieted_until() const;
	void set_quieted_until(std::uint64_t quieted_until);
*/
	const std::string &get_attribute(LegionAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<LegionAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<LegionAttributeId, std::string> modifiers);

	template<typename T, typename DefaultT = T>
	T cast_attribute(LegionAttributeId account_attribute_id, const DefaultT def = DefaultT()){
		const auto &str = get_attribute(account_attribute_id);
		if(str.empty()){
			return def;
		}
		return boost::lexical_cast<T>(str);
	}


	void synchronize_with_player(AccountUuid account_uuid,const boost::shared_ptr<PlayerSession> &session) const;
	void broadcast_to_members(std::uint16_t message_id, const Poseidon::StreamBuffer &payload);
	template<class MessageT>
	void broadcast_to_members(const MessageT &msg){
		broadcast_to_members(MessageT::ID, Poseidon::StreamBuffer(msg));
	}

	template<class MessageT>
	void send_msg_by_power(const MessageT &msg,unsigned powerid)
	{
		PROFILE_ME;
		std::vector<boost::shared_ptr<LegionMember>> members;
		LegionMemberMap::get_by_legion_uuid(members, get_legion_uuid());

		for(auto it = members.begin(); it != members.end(); ++it)
		{
			// 判断玩家是否在线
			auto member = *it;
			const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
			if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),powerid))
			{
				const auto session = PlayerSessionMap::get((*it)->get_account_uuid());
				if(session)
				{
					try {
						session->send(msg);
					} catch(std::exception &e){
						LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
						session->shutdown(e.what());
					}
				}
			}
		}
	}
};

}

#endif
