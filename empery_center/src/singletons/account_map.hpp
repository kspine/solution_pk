#ifndef EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_ACCOUNT_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class PlayerSession;

struct AccountMap {
	enum {
		FL_VALID                        = 0x0001,
		FL_ROBOT                        = 0x0002,

		ATTR_CUSTOM_PUBLIC_END          = 100,
		ATTR_CUSTOM_END                 = 200,
		ATTR_PUBLIC_END                 = 300,
		ATTR_END                        = 500,

		MAX_NISK_LEN                    =  255,
		MAX_ATTRIBUTE_LEN               = 4096,

		ATTR_GENDER                     =   1,
		ATTR_AVATAR                     =   2,

		ATTR_LAST_LOGGED_IN_TIME        = 300,
		ATTR_LAST_LOGGED_OUT_TIME       = 301,
		ATTR_LAST_SIGNED_IN_TIME        = 302,
		ATTR_SEQUENTIAL_SIGNED_IN_DAYS  = 303,
	};

	struct AccountInfo {
		AccountUuid account_uuid;
		std::string login_name;
		std::string nick;
		boost::uint64_t flags;
		boost::uint64_t created_time;
	};

	static bool has(AccountUuid account_uuid);
	static AccountInfo get(AccountUuid account_uuid);
	static AccountInfo require(AccountUuid account_uuid);
	static void get_by_nick(std::vector<AccountInfo> &ret, const std::string &nick);

	struct LoginInfo {
		PlatformId platform_id;
		std::string login_name;
		AccountUuid account_uuid;
		boost::uint64_t flags;
		std::string login_token;
		boost::uint64_t login_token_expiry_time;
		boost::uint64_t banned_until;
	};

	static LoginInfo get_login_info(AccountUuid account_uuid);
	static LoginInfo get_login_info(PlatformId platform_id, const std::string &login_name);

	static std::pair<AccountUuid, bool> create(PlatformId platform_id, std::string login_name,
		std::string nick, boost::uint64_t flags, std::string remote_ip);

	static void set_nick(AccountUuid account_uuid, std::string nick);
	static void set_login_token(AccountUuid account_uuid, std::string login_token, boost::uint64_t expiry_time);
	static void set_banned_until(AccountUuid account_uuid, boost::uint64_t until);
	static void set_flags(AccountUuid account_uuid, boost::uint64_t flags);

	static const std::string &get_attribute(AccountUuid account_uuid, unsigned slot);
	static void get_attributes(boost::container::flat_map<unsigned, std::string> &ret,
		AccountUuid account_uuid, unsigned begin_slot, unsigned end_slot);
	static void touch_attribute(AccountUuid account_uuid, unsigned slot);
	static void set_attribute(AccountUuid account_uuid, unsigned slot, std::string value);

	template<typename T, typename DefaultT = T>
	static T get_attribute_gen(AccountUuid account_uuid, unsigned slot, const DefaultT def = DefaultT()){
		const auto &str = get_attribute(account_uuid, slot);
		if(str.empty()){
			return T(def);
		}
		return boost::lexical_cast<T>(str);
	}
	template<typename T>
	static void set_attribute_gen(AccountUuid account_uuid, unsigned slot, const T &val){
		set_attribute(account_uuid, slot, boost::lexical_cast<std::string>(val));
	}

	static void send_attributes_to_client(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
		bool wants_nick, bool wants_attributes, bool wants_private_attributes, bool wants_items);
	static void combined_send_attributes_to_client(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session);

private:
	AccountMap() = delete;
};

}

#endif
