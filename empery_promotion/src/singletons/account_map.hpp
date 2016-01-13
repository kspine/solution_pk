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
		FL_VALID                          = 0x0001,
		FL_ROBOT                          = 0x0002,
		FL_DEACTIVATED                    = 0x0004,

		MAX_ATTR_LEN                      = 4096,

		ATTR_GENDER                       =  1,
		ATTR_COUNTRY                      =  2,
//		ATTR_PHONE_NUMBER                 =  3,
		ATTR_BANK_ACCOUNT_NAME            =  4,
		ATTR_BANK_NAME                    =  5,
		ATTR_BANK_ACCOUNT_NUMBER          =  6,
		ATTR_BANK_SWIFT_CODE              =  7,
		ATTR_REMARKS                      =  8,
		ATTR_MAX_VISIBLE_SUBORD_DEPTH     =  9,
		ATTR_CAN_VIEW_ACCOUNT_PERFORMANCE = 10,
		ATTR_AUCTION_CENTER_ENABLED       = 11,
	};

	struct AccountInfo {
		AccountId account_id;
		std::string login_name;
		std::string phone_number;
		std::string nick;
		std::string password_hash;
		std::string deal_password_hash;
		AccountId referrer_id;
		std::uint64_t level;
		std::uint64_t max_level;
		std::uint64_t subordinate_count;
		std::uint64_t performance;
		std::uint64_t flags;
		std::uint64_t banned_until;
		std::uint64_t created_time;
		std::string created_ip;
	};

	static bool has(AccountId account_id);
	static AccountInfo get(AccountId account_id);
	static AccountInfo require(AccountId account_id);

	static std::uint64_t get_count();
	static void get_all(std::vector<AccountInfo> &ret, std::uint64_t begin = 0, std::uint64_t max = (std::uint64_t)-1);

	static AccountInfo get_by_login_name(const std::string &login_name);

	static void get_by_phone_number(std::vector<AccountInfo> &ret, const std::string &phone_number);
	static void get_by_referrer_id(std::vector<AccountInfo> &ret, AccountId referrer_id);

	static std::string get_password_hash(const std::string &password);

	static void set_login_name(AccountId account_id, std::string login_name);
	static void set_phone_number(AccountId account_id, std::string phone_number);
	static void set_nick(AccountId account_id, std::string nick);
	static void set_password(AccountId account_id, const std::string &password);
	static void set_deal_password(AccountId account_id, const std::string &deal_password);
	static void set_level(AccountId account_id, std::uint64_t level);
	static void set_flags(AccountId account_id, std::uint64_t flags);
	static void set_banned_until(AccountId account_id, std::uint64_t banned_until);
	static void accumulate_performance(AccountId account_id, std::uint64_t amount);

	static AccountId create(std::string login_name, std::string phone_number, std::string nick,
		const std::string &password, const std::string &deal_password, AccountId referrer_id, std::uint64_t flags, std::string created_ip);

	static const std::string &get_attribute(AccountId account_id, unsigned slot);
	static void get_attributes(std::vector<std::pair<unsigned, std::string>> &ret, AccountId account_id);
	static void touch_attribute(AccountId account_id, unsigned slot);
	static void set_attribute(AccountId account_id, unsigned slot, std::string value);

	template<typename T>
	static T cast_attribute(AccountId account_id, unsigned slot){
		const auto &str = get_attribute(account_id, slot);
		if(str.empty()){
			return T();
		}
		return boost::lexical_cast<T>(str);
	}

private:
	AccountMap() = delete;
};

}

#endif
