#ifndef EMPERY_PROMOTION_SINGLETONS_ACCOUNT_MAP_HPP_
#define EMPERY_PROMOTION_SINGLETONS_ACCOUNT_MAP_HPP_

#include <string>
#include <vector>
#include <cstddef>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include "../id_types.hpp"

namespace EmperyPromotion {

struct AccountMap {
	enum {
		FL_VALID                    = 0x0001,
		FL_ROBOT                    = 0x0002,

		MAX_ATTR_LEN                = 4096,

		ATTR_GENDER                 = 1,
		ATTR_COUNTRY                = 2,
//		ATTR_PHONE_NUMBER           = 3,
		ATTR_BANK_ACCOUNT_NAME      = 4,
		ATTR_BANK_NAME              = 5,
		ATTR_BANK_ACCOUNT_NUMBER    = 6,
		ATTR_BANK_SWIFT_CODE        = 7,
		ATTR_REMARKS                = 8,

		ATTR_ACCOUNT_LEVEL          = 100,
	};

	struct AccountInfo {
		AccountId accountId;
		std::string loginName;
		std::string phoneNumber;
		std::string nick;
		std::string passwordHash;
		std::string dealPasswordHash;
		AccountId referrerId;
		boost::uint64_t flags;
		boost::uint64_t bannedUntil;
		boost::uint64_t createdTime;
		std::string createdIp;
	};

	static bool has(AccountId accountId);
	static AccountInfo get(AccountId accountId);
	static AccountInfo require(AccountId accountId);

	static boost::uint64_t getCount();
	static void getAll(std::vector<AccountInfo> &ret, boost::uint64_t begin = 0, boost::uint64_t max = (boost::uint64_t)-1);

	static AccountInfo getByLoginName(const std::string &loginName);

	static void getByPhoneNumber(std::vector<AccountInfo> &ret, const std::string &phoneNumber);
	static void getByReferrerId(std::vector<AccountInfo> &ret, AccountId referrerId);

	static std::string getPasswordHash(const std::string &password);

	static void setLoginName(AccountId accountId, std::string loginName);
	static void setPhoneNumber(AccountId accountId, std::string phoneNumber);
	static void setNick(AccountId accountId, std::string nick);
	static void setPassword(AccountId accountId, const std::string &password);
	static void setDealPassword(AccountId accountId, const std::string &dealPassword);
	static void setFlags(AccountId accountId, boost::uint64_t flags);
	static void setBannedUntil(AccountId accountId, boost::uint64_t bannedUntil);

	static AccountId create(std::string loginName, std::string phoneNumber, std::string nick,
		const std::string &password, const std::string &dealPassword, AccountId referrerId, boost::uint64_t flags, std::string createdIp);

	static const std::string &getAttribute(AccountId accountId, unsigned slot);
	static void getAttributes(std::vector<std::pair<unsigned, std::string>> &ret, AccountId accountId);
	static void touchAttribute(AccountId accountId, unsigned slot);
	static void setAttribute(AccountId accountId, unsigned slot, std::string value);

	template<typename T>
	static T castAttribute(AccountId accountId, unsigned slot){
		const auto &str = getAttribute(accountId, slot);
		if(str.empty()){
			return T();
		}
		return boost::lexical_cast<T>(str);
	}

private:
	AccountMap();
};

}

#endif
