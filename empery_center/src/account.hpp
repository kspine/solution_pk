#ifndef EMPERY_CENTER_ACCOUNT_HPP_
#define EMPERY_CENTER_ACCOUNT_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/lexical_cast.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Account;
	class Center_AccountAttribute;
}

class Account : public virtual Poseidon::VirtualSharedFromThis {
public:
	enum {
		FL_ROBOT            = 0x0001,

		MAX_NICK_LEN        =  255,
		MAX_ATTRIBUTE_LEN   = 4096,
	};

private:
	boost::shared_ptr<MySql::Center_Account> m_obj;
	boost::container::flat_map<AccountAttributeId,
		boost::shared_ptr<MySql::Center_AccountAttribute>> m_attributes;

public:
	Account(AccountUuid account_uuid, PlatformId platformId, std::string login_name, boost::uint64_t created_time,
		std::string nick, boost::uint64_t flags);
	Account(boost::shared_ptr<MySql::Center_Account> obj,
		const std::vector<boost::shared_ptr<MySql::Center_AccountAttribute>> &attributes);
	~Account();

public:
	AccountUuid get_account_uuid() const;
	PlatformId get_platform_id() const;
	const std::string &get_login_name() const;

	const std::string &get_nick() const;
	void set_nick(std::string nick);

	boost::uint64_t get_flags() const;
	void set_flags(boost::uint64_t flags);

	const std::string &get_login_token() const;
	boost::uint64_t get_login_token_expiry_time() const;
	void set_login_token(std::string login_token, boost::uint64_t login_token_expiry_time);

	boost::uint64_t get_banned_until() const;
	void set_banned_until(boost::uint64_t banned_until);

	const std::string &get_attribute(AccountAttributeId account_attribute_id) const;
	void get_attributes(boost::container::flat_map<AccountAttributeId, std::string> &ret) const;
	void set_attributes(boost::container::flat_map<AccountAttributeId, std::string> modifiers);

	template<typename T, typename DefaultT = T>
	T cast_attribute(AccountAttributeId account_attribute_id, const DefaultT def = DefaultT()){
		const auto &str = get_attribute(account_attribute_id);
		if(str.empty()){
			return def;
		}
		return boost::lexical_cast<T>(str);
	}
};

}

#endif
