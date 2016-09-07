#ifndef EMPERY_LEAGUE_SINGLETONS_LEAGUE_MEMBER_MAP_HPP_
#define EMPERY_LEAGUE_SINGLETONS_LEAGUE_MEMBER_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"


namespace EmperyLeague {

class LeagueMember;

struct LeagueMemberMap {
	static bool is_holding_controller_token(LeagueUuid account_uuid);

	static boost::shared_ptr<LeagueMember> get(LeagueUuid account_uuid);
	static boost::shared_ptr<LeagueMember> require(LeagueUuid account_uuid);

	static boost::shared_ptr<LeagueMember> get_by_legion_uuid(LegionUuid legion_uuid);
	static void get_by_lea_uuid(std::vector<boost::shared_ptr<LeagueMember>> &ret, LeagueUuid legion_uuid);

//	static boost::shared_ptr<LeagueMember> get_or_reload(LeagueUuid account_uuid);
//	static boost::shared_ptr<LeagueMember> forced_reload(LeagueUuid account_uuid);
//	static boost::shared_ptr<LeagueMember> get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name);
//	static boost::shared_ptr<LeagueMember> forced_reload_by_login_name(PlatformId platform_id, const std::string &login_name);

	static std::uint64_t get_count();
	static std::uint64_t get_league_member_count(LeagueUuid legion_uuid);
	static void get_all(std::vector<boost::shared_ptr<LeagueMember>> &ret, std::uint64_t begin, std::uint64_t count);

	static void get_by_league_uuid(std::vector<boost::shared_ptr<LeagueMember>> &ret, LeagueUuid league_uuid);
	static void get_member_by_titleid(std::vector<boost::shared_ptr<LeagueMember>> &ret, LeagueUuid legion_uuid,std::uint64_t level);
	static void insert(const boost::shared_ptr<LeagueMember> &account, const std::string &remote_ip);
	static void deletemember(LegionUuid legion_uuid ,bool bdeletemap=true);
	static void update(const boost::shared_ptr<LeagueMember> &account, bool throws_if_not_exists = true);
	// 联盟解散后的善后操作
	static void disband_league(LeagueUuid legion_uuid);
	// 检查是否处于同一个联盟
	static bool is_in_same_league(AccountUuid account_uuid,AccountUuid other_uuid);
	// 检查是否有退出、被踢出、转让联盟长的等待时间，如果有进行相应处理
	static void check_in_waittime();
	// 检查是否有需要重置的内容
	static void check_in_resetime();
	// 联盟成员聊天
//	static int chat(const boost::shared_ptr<LeagueMember> &member,const boost::shared_ptr<ChatMessage> &message);


private:
	LeagueMemberMap() = delete;
};

}

#endif
