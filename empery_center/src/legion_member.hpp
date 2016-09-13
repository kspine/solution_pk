#ifndef EMPERY_CENTER_LEGION_MEMBER_HPP_
#define EMPERY_CENTER_LEGION_MEMBER_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/lexical_cast.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Legion;
	class Center_LegionMemberAttribute;
	class Center_Legion_Member;
}

class PlayerSession;
class Account;
class ChatMessage;

class LegionMember : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:

	static std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<LegionMember>> async_create(
		LegionUuid legion_uuid,   AccountUuid account_uuid, std::uint64_t created_time);

private:
	const boost::shared_ptr<MySql::Center_Legion_Member> m_obj;

	boost::container::flat_map<LegionMemberAttributeId,
		boost::shared_ptr<MySql::Center_LegionMemberAttribute>> m_attributes;


public:
	LegionMember(boost::shared_ptr<MySql::Center_Legion_Member> obj,
		const std::vector<boost::shared_ptr<MySql::Center_LegionMemberAttribute>> &attributes);
	~LegionMember();

public:
	LegionUuid get_legion_uuid() const;
	AccountUuid get_account_uuid() const;

	std::uint64_t get_created_time() const;

	// 初始化属性
	void InitAttributes(boost::shared_ptr<Account> account,unsigned nTitleid);
	// 离开善后操作
	void leave();
	const std::string &get_attribute(LegionMemberAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<LegionMemberAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<LegionMemberAttributeId, std::string> modifiers);

	// 设置联盟uuid
	void set_league_uuid(std::string str_league_uuid);

	template<typename T, typename DefaultT = T>
	T cast_attribute(LegionMemberAttributeId account_attribute_id, const DefaultT def = DefaultT()){
		const auto &str = get_attribute(account_attribute_id);
		if(str.empty()){
			return def;
		}
		return boost::lexical_cast<T>(str);
	}

	// 军团成员在联盟中聊天
	int league_chat(const boost::shared_ptr<ChatMessage> &message);

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
