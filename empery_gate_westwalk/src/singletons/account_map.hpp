#ifndef EMPERY_GATE_WESTWALK_SINGLETONS_ACCOUNT_MAP_HPP_
#define EMPERY_GATE_WESTWALK_SINGLETONS_ACCOUNT_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/shared_ptr.hpp>

namespace EmperyGateWestwalk {

struct AccountMap {
	enum {
		FL_VALID                    = 0x0001,
		FL_ROBOT                    = 0x0002,
		FL_FROZEN                   = 0x0004,

		MAX_ACCOUNT_NAME_LEN        = 31,
	};

	struct AccountInfo {
		std::string account_name;
		std::string created_ip;
		boost::uint64_t created_time;
		std::string remarks;
		std::string password_hash;
		std::string disposable_password_hash;
		boost::uint64_t disposable_password_expiry_time;
		boost::uint64_t password_regain_cooldown_time;
		std::string token;
		boost::uint64_t token_expiry_time;
		std::string last_login_ip;
		boost::uint64_t last_login_time;
		boost::uint64_t banned_until;
		boost::uint32_t retry_count;
		boost::uint64_t flags;
	};

	static bool has(const std::string &account_name);
	static AccountInfo get(const std::string &account_name);
	static AccountInfo require(const std::string &account_name);
	static void get_all(std::vector<AccountInfo> &ret);

	static std::string random_password();
	static std::string get_password_hash(const std::string &password);

	static void set_password(const std::string &account_name, const std::string &password);
	static void set_disposable_password(const std::string &account_name, const std::string &password, boost::uint64_t expiry_time);
	static void commit_disposable_password(const std::string &account_name);
	static void set_password_regain_cooldown_time(const std::string &account_name, boost::uint64_t expiry_time);
	static void set_token(const std::string &account_name, std::string token, boost::uint64_t expiry_time);
	static void set_last_login(const std::string &account_name, std::string last_login_ip, boost::uint64_t last_login_time);
	static void set_banned_until(const std::string &account_name, boost::uint64_t banned_until);
	static boost::uint32_t decrement_password_retry_count(const std::string &account_name);
	static boost::uint32_t reset_password_retry_count(const std::string &account_name);

	static void create(std::string account_name, std::string token, std::string remote_ip, std::string remarks, boost::uint64_t flags);

private:
	AccountMap() = delete;
};

}

#endif
