#ifndef EMPERY_CONTROLLER_ACCOUNT_HPP_
#define EMPERY_CONTROLLER_ACCOUNT_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Account;
}

}

namespace EmperyController {

class ControllerSession;

class Account : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<EmperyCenter::MySql::Center_Account> m_obj;

	boost::weak_ptr<ControllerSession> m_controller;
	std::uint64_t m_locked_until = 0;

public:
	explicit Account(boost::shared_ptr<EmperyCenter::MySql::Center_Account> obj);
	~Account();

public:
	AccountUuid get_account_uuid() const;
	PlatformId get_platform_id() const;
	const std::string &get_login_name() const;
	AccountUuid get_referrer_uuid() const;

	unsigned get_promotion_level() const;
	std::uint64_t get_created_time() const;
	const std::string &get_nick() const;
	bool has_been_activated() const;
	std::uint64_t get_banned_until() const;
	std::uint64_t get_quieted_until() const;

	const boost::weak_ptr<ControllerSession> &get_weak_controller() const {
		return m_controller;
	}
	boost::shared_ptr<ControllerSession> get_controller() const {
		return m_controller.lock();
	}
	void set_controller(const boost::shared_ptr<ControllerSession> &controller);
	boost::shared_ptr<ControllerSession> try_set_controller(const boost::shared_ptr<ControllerSession> &controller);

	std::uint64_t get_locked_until() const {
		return m_locked_until;
	}
	void set_locked_until(std::uint64_t locked_until){
		m_locked_until = locked_until;
	}
};

}

#endif
