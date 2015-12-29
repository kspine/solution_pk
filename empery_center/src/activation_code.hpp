#ifndef EMPERY_CENTER_ACTIVATION_CODE_HPP_
#define EMPERY_CENTER_ACTIVATION_CODE_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_ActivationCode;
}

class ActivationCode : public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_ActivationCode> m_obj;

public:
	ActivationCode(std::string code, std::uint64_t created_time, std::uint64_t expiry_time);
	explicit ActivationCode(boost::shared_ptr<MySql::Center_ActivationCode> obj);
	~ActivationCode();

public:
	const std::string &get_code() const;
	std::uint64_t get_created_time() const;

	std::uint64_t get_expiry_time() const;
	void set_expiry_time(std::uint64_t expiry_time);

	AccountUuid get_used_by_account() const;
	void set_used_by_account(AccountUuid account_uuid);

	bool is_virtually_removed() const;
};

}

#endif
