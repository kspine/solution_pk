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
		FL_VALID                    = 0x0001,
		FL_ROBOT                    = 0x0002,

		ATTR_CUSTOM_PUBLIC_END      = 100,
		ATTR_CUSTOM_END             = 200,
		ATTR_PUBLIC_END             = 300,
		ATTR_END                    = 400,

		MAX_NICK_LEN                =  255,
		MAX_ATTRIBUTE_LEN           = 4096,

		ATTR_GENDER                 =   1,
		ATTR_AVATAR                 =   2,

		ATTR_TIME_LAST_LOGGED_IN    = 300,
		ATTR_TIME_LAST_LOGGED_OUT   = 301,
		ATTR_TIME_CREATED           = 302,
	};

	struct AccountInfo {
		AccountUuid accountUuid;
		std::string loginName;
		std::string nick;
		boost::uint64_t flags;
		boost::uint64_t createdTime;
	};

	static bool has(const AccountUuid &accountUuid);
	static AccountInfo get(const AccountUuid &accountUuid);
	static AccountInfo require(const AccountUuid &accountUuid);
	static void getByNick(std::vector<AccountInfo> &ret, const std::string &nick);

	struct LoginInfo {
		PlatformId platformId;
		std::string loginName;
		AccountUuid accountUuid;
		boost::uint64_t flags;
		std::string loginToken;
		boost::uint64_t loginTokenExpiryTime;
		boost::uint64_t bannedUntil;
	};

	static LoginInfo getLoginInfo(const AccountUuid &accountUuid);
	static LoginInfo getLoginInfo(PlatformId platformId, const std::string &loginName);

	static std::pair<AccountUuid, bool> create(PlatformId platformId, std::string loginName,
		std::string nick, boost::uint64_t flags, std::string remoteIp);

	static void setNick(const AccountUuid &accountUuid, std::string nick);
	static void setLoginToken(const AccountUuid &accountUuid, std::string loginToken, boost::uint64_t expiryTime);
	static void setBannedUntil(const AccountUuid &accountUuid, boost::uint64_t until);
	static void setFlags(const AccountUuid &accountUuid, boost::uint64_t flags);

	static const std::string &getAttribute(const AccountUuid &accountUuid, unsigned slot);
	static void getAttributes(boost::container::flat_map<unsigned, std::string> &ret,
		const AccountUuid &accountUuid, unsigned beginSlot, unsigned endSlot);
	static void touchAttribute(const AccountUuid &accountUuid, unsigned slot);
	static void setAttribute(const AccountUuid &accountUuid, unsigned slot, std::string value);

	template<typename T, typename DefaultT = T>
	static T castAttribute(const AccountUuid &accountUuid, unsigned slot, const DefaultT def = DefaultT()){
		const auto &str = getAttribute(accountUuid, slot);
		if(str.empty()){
			return T(def);
		}
		return boost::lexical_cast<T>(str);
	}

private:
	AccountMap();
};

}

#endif
