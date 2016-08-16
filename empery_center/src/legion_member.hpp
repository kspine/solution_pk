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
//	const std::string &get_login_name() const;
	LegionUuid get_creater_uuid() const;

//	unsigned get_promotion_level() const;
	void set_promotion_level(unsigned promotion_level);

	std::uint64_t get_created_time() const;

//	const std::string &get_nick() const;
//	void set_nick(std::string nick);

	// 初始化属性
	void InitAttributes(unsigned nTitleid,std::string  donate,std::string  weekdonate);
	// 离开善后操作
	void leave();

/*	bool has_been_activated() const;
	void set_activated(bool activated);

	std::uint64_t get_banned_until() const;
	void set_banned_until(std::uint64_t banned_until);

	std::uint64_t get_quieted_until() const;
	void set_quieted_until(std::uint64_t quieted_until);
*/
	const std::string &get_attribute(LegionMemberAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<LegionMemberAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<LegionMemberAttributeId, std::string> modifiers);

	template<typename T, typename DefaultT = T>
	T cast_attribute(LegionMemberAttributeId account_attribute_id, const DefaultT def = DefaultT()){
		const auto &str = get_attribute(account_attribute_id);
		if(str.empty()){
			return def;
		}
		return boost::lexical_cast<T>(str);
	}

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
