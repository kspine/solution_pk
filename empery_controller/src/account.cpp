#include "precompiled.hpp"
#include "account.hpp"
#include "../../empery_center/src/mysql/account.hpp"
#include "singletons/account_map.hpp"

namespace EmperyController {

Account::Account(boost::shared_ptr<EmperyCenter::MySql::Center_Account> obj)
	: m_obj(std::move(obj))
{
}
Account::~Account(){
}

AccountUuid Account::get_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_account_uuid());
}
PlatformId Account::get_platform_id() const {
	return PlatformId(m_obj->get_platform_id());
}
const std::string &Account::get_login_name() const {
	return m_obj->unlocked_get_login_name();
}
AccountUuid Account::get_referrer_uuid() const {
	return AccountUuid(m_obj->unlocked_get_referrer_uuid());
}

unsigned Account::get_promotion_level() const {
	return m_obj->get_promotion_level();
}
std::uint64_t Account::get_created_time() const {
	return m_obj->get_created_time();
}
const std::string &Account::get_nick() const {
	return m_obj->unlocked_get_nick();
}
bool Account::has_been_activated() const {
	return m_obj->get_activated();
}
std::uint64_t Account::get_banned_until() const {
	return m_obj->get_banned_until();
}
std::uint64_t Account::get_quieted_until() const {
	return m_obj->get_quieted_until();
}

void Account::set_controller(const boost::shared_ptr<ControllerSession> &controller){
	PROFILE_ME;

	m_controller = controller;

	AccountMap::update(virtual_shared_from_this<Account>(), false);
}
boost::shared_ptr<ControllerSession> Account::try_set_controller(const boost::shared_ptr<ControllerSession> &controller){
	PROFILE_ME;

	auto new_controller = m_controller.lock();
	if(!new_controller){
		set_controller(controller);
		new_controller = controller;
	}
	return new_controller;
}

}
