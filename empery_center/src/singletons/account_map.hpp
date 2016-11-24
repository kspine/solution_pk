#ifndef EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class Account;
class PlayerSession;
class ItemBox;

struct AccountMap {
	static bool is_holding_controller_token(AccountUuid account_uuid);
	static void require_controller_token(AccountUuid account_uuid, const std::string &remote_ip);

	static boost::shared_ptr<Account> get(AccountUuid account_uuid);
	static boost::shared_ptr<Account> require(AccountUuid account_uuid);
	static boost::shared_ptr<Account> get_or_reload(AccountUuid account_uuid);
	static boost::shared_ptr<Account> forced_reload(AccountUuid account_uuid);
	static boost::shared_ptr<Account> get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name);
	static boost::shared_ptr<Account> forced_reload_by_login_name(PlatformId platform_id, const std::string &login_name);

	static std::uint64_t get_count();
	static void get_all(std::vector<boost::shared_ptr<Account>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_banned(std::vector<boost::shared_ptr<Account>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_quieted(std::vector<boost::shared_ptr<Account>> &ret, std::uint64_t begin, std::uint64_t count);
	static void get_by_nick(std::vector<boost::shared_ptr<Account>> &ret, const std::string &nick);
	static void get_by_referrer(std::vector<boost::shared_ptr<Account>> &ret, AccountUuid referrer_uuid);

	static void insert(const boost::shared_ptr<Account> &account, const std::string &remote_ip);
	static void update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists = true);

	static void synchronize_account_with_player_all(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
		bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box);

	static void cached_synchronize_account_with_player_all(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
		bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box);
	static void cached_synchronize_account_with_player_all(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session);
	
	static bool is_friendly(AccountUuid account_uuid,AccountUuid other_uuid);
	
	static void synchronize_account_legion_with_player_all(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session);
	static void synchronize_account_league_with_player_all(AccountUuid account_uuid,AccountUuid to_account_uuid);

	static void synchronize_account_online_state_with_relate_player_all(AccountUuid account_uuid,bool online,std::uint64_t utc_now);

private:
	AccountMap() = delete;
};

}

#endif
