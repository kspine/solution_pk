#ifndef EMPERY_CENTER_SINGLETONS_LEGION_MEMBER_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_LEGION_MEMBER_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class LegionMember;
class PlayerSession;
class ItemBox;


struct LegionMemberMap {
	static bool is_holding_controller_token(LegionUuid account_uuid);
	static void require_controller_token(LegionUuid account_uuid);

	static boost::shared_ptr<LegionMember> get(LegionUuid account_uuid);
	static boost::shared_ptr<LegionMember> require(LegionUuid account_uuid);

	static boost::shared_ptr<LegionMember> get_by_account_uuid(AccountUuid account_uuid);
	static void get_by_legion_uuid(std::vector<boost::shared_ptr<LegionMember>> &ret, LegionUuid legion_uuid);

//	static boost::shared_ptr<LegionMember> get_or_reload(LegionUuid account_uuid);
//	static boost::shared_ptr<LegionMember> forced_reload(LegionUuid account_uuid);
//	static boost::shared_ptr<LegionMember> get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name);
//	static boost::shared_ptr<LegionMember> forced_reload_by_login_name(PlatformId platform_id, const std::string &login_name);

	static std::uint64_t get_count();
	static std::uint64_t get_legion_member_count(LegionUuid legion_uuid);
	static void get_all(std::vector<boost::shared_ptr<LegionMember>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_banned(std::vector<boost::shared_ptr<LegionMember>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_quieted(std::vector<boost::shared_ptr<LegionMember>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_by_nick(std::vector<boost::shared_ptr<LegionMember>> &ret, const std::string &nick);
	static void get_by_referrer(std::vector<boost::shared_ptr<LegionMember>> &ret, AccountUuid referrer_uuid);

	static void get_member_by_titleid(std::vector<boost::shared_ptr<LegionMember>> &ret, LegionUuid legion_uuid,std::uint64_t level);
	static void insert(const boost::shared_ptr<LegionMember> &account, const std::string &remote_ip);
	static void deletemember(AccountUuid account_uuid ,bool bdeletemap=true);
	static void update(const boost::shared_ptr<LegionMember> &account, bool throws_if_not_exists = true);
	// 军团解散后的善后操作
	static void disband_legion(LegionUuid legion_uuid);
	// 检查是否处于同一个军团
	static bool is_in_same_legion(AccountUuid account_uuid,AccountUuid other_uuid);
	// 检查是否有退出、被踢出、转让军团长的等待时间，如果由进行相应处理
	static void check_in_waittime();
	// 检查是否有需要重置的内容
	static void check_in_resetime();

	static void synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
		bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box);

	static void cached_synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
		bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box);
	static void cached_synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session);

private:
	LegionMemberMap() = delete;
};

}

#endif
