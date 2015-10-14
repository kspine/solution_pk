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
		std::string accountName;
		std::string createdIp;
		boost::uint64_t createdTime;
		std::string remarks;
		std::string passwordHash;
		std::string disposablePasswordHash;
		boost::uint64_t disposablePasswordExpiryTime;
		boost::uint64_t passwordRegainCooldownTime;
		std::string token;
		boost::uint64_t tokenExpiryTime;
		std::string lastLoginIp;
		boost::uint64_t lastLoginTime;
		boost::uint64_t bannedUntil;
		boost::uint32_t retryCount;
		boost::uint64_t flags;
	};

	static bool has(const std::string &accountName);
	static AccountInfo get(const std::string &accountName);
	static AccountInfo require(const std::string &accountName);
	static void getAll(std::vector<AccountInfo> &ret);

	static std::string randomPassword();
	static std::string getPasswordHash(const std::string &password);

	static void setPassword(const std::string &accountName, const std::string &password);
	static void setDisposablePassword(const std::string &accountName, const std::string &password, boost::uint64_t expiryTime);
	static void commitDisposablePassword(const std::string &accountName);
	static void setPasswordRegainCooldownTime(const std::string &accountName, boost::uint64_t expiryTime);
	static void setToken(const std::string &accountName, std::string token, boost::uint64_t expiryTime);
	static void setLastLogin(const std::string &accountName, std::string lastLoginIp, boost::uint64_t lastLoginTime);
	static void setBannedUntil(const std::string &accountName, boost::uint64_t bannedUntil);
	static boost::uint32_t decrementPasswordRetryCount(const std::string &accountName);
	static boost::uint32_t resetPasswordRetryCount(const std::string &accountName);

	static void create(std::string accountName, std::string token, std::string remoteIp, std::string remarks, boost::uint64_t flags);

private:
	AccountMap();
};

}

#endif
