#ifndef EMPERY_LEAGUE_LEAGUE_MEMBER_HPP_
#define EMPERY_LEAGUE_LEAGUE_MEMBER_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/fwd.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/lexical_cast.hpp>
#include "id_types.hpp"

namespace EmperyLeague {

namespace MySql {
	class League_Member;
	class League_MemberAttribute;
}

class PlayerSession;
class Account;

class LeagueMember : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:

	static std::pair<boost::shared_ptr<const Poseidon::JobPromise>, boost::shared_ptr<LeagueMember>> async_create(
		LeagueUuid league_uuid,   LegionUuid legion_uuid,  std::uint64_t created_time);

private:
	const boost::shared_ptr<MySql::League_Member> m_obj;

	boost::container::flat_map<LeagueMemberAttributeId,
		boost::shared_ptr<MySql::League_MemberAttribute>> m_attributes;


public:
	LeagueMember(boost::shared_ptr<MySql::League_Member> obj,
		const std::vector<boost::shared_ptr<MySql::League_MemberAttribute>> &attributes);
	~LeagueMember();

public:
	LeagueUuid get_league_uuid() const;
	LegionUuid get_legion_uuid() const;

	std::uint64_t get_created_time() const;

	// 初始化属性
	void InitAttributes(LegionUuid legion_uuid,unsigned nTitleid);
	// 离开善后操作
	void leave();
	const std::string &get_attribute(LeagueMemberAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<LeagueMemberAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<LeagueMemberAttributeId, std::string> modifiers);

	template<typename T, typename DefaultT = T>
	T cast_attribute(LeagueMemberAttributeId account_attribute_id, const DefaultT def = DefaultT()){
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
