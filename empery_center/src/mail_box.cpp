#include "precompiled.hpp"
#include "mail_box.hpp"
#include "msg/sc_mail.hpp"
#include "mysql/mail.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "checked_arithmetic.hpp"

namespace EmperyCenter {

namespace {
	void fill_mail_info(MailBox::MailInfo &info, const boost::shared_ptr<MySql::Center_Mail> &obj){
		PROFILE_ME;

		info.mail_uuid   = MailUuid(obj->get_mail_uuid());
		info.expiry_time = obj->get_expiry_time();
		info.flags       = obj->get_flags();
	}

	void fill_mail_message(Msg::SC_MailChanged &msg, const boost::shared_ptr<MySql::Center_Mail> &obj, boost::uint64_t utc_now){
		PROFILE_ME;

		msg.mail_uuid       = obj->get_mail_uuid().to_string();
		msg.expiry_duration = saturated_sub(utc_now, obj->get_expiry_time());
		msg.flags           = obj->get_flags();
	}
}

MailBox::MailBox(const AccountUuid &account_uuid)
	: m_account_uuid(account_uuid)
{
}
MailBox::MailBox(const AccountUuid &account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Mail>> &mails)
	: MailBox(account_uuid)
{
	for(auto it = mails.begin(); it != mails.end(); ++it){
		const auto &obj = *it;
		m_mails.emplace(MailUuid(obj->get_mail_uuid()), obj);
	}
}
MailBox::~MailBox(){
}

void MailBox::pump_status(bool force_synchronization_with_client){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	{
		auto it = m_mails.begin();
		while(it != m_mails.end()){
			if(it->second->get_expiry_time() < utc_now){
				it = m_mails.erase(it);
			} else {
				++it;
			}
		}
	}

	if(force_synchronization_with_client){
		const auto session = PlayerSessionMap::get(get_account_uuid());
		if(session){
			try {
				for(auto it = m_mails.begin(); it != m_mails.end(); ++it){
					Msg::SC_MailChanged msg;
					fill_mail_message(msg, it->second, utc_now);
					session->send(msg);
				}
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->force_shutdown();
			}
		}
	}
}

MailBox::MailInfo MailBox::get(const MailUuid &mail_uuid) const {
	PROFILE_ME;

	MailInfo info = { };
	info.mail_uuid = mail_uuid;

	const auto it = m_mails.find(mail_uuid);
	if(it == m_mails.end()){
		return info;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(it->second->get_expiry_time() < utc_now){
		return info;
	}
	fill_mail_info(info, it->second);
	return info;
}
void MailBox::get_all(std::vector<MailBox::MailInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_mails.size());
	const auto utc_now = Poseidon::get_utc_time();
	for(auto it = m_mails.begin(); it != m_mails.end(); ++it){
		if(it->second->get_expiry_time() < utc_now){
			continue;
		}
		MailInfo info;
		fill_mail_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

bool MailBox::create(const MailInfo &info){
	PROFILE_ME;

	bool created = false;
	auto it = m_mails.find(info.mail_uuid);
	if(it == m_mails.end()){
		auto obj = boost::make_shared<MySql::Center_Mail>(info.mail_uuid.get(), get_account_uuid().get(), 0, 0);
		obj->async_save(true);
		it = m_mails.emplace_hint(it, info.mail_uuid, std::move(obj));
		created = true;
	}
	const auto &obj = it->second;
	obj->set_expiry_time(info.expiry_time);
	obj->set_flags(info.flags);
	return created;
}
bool MailBox::remove(const MailUuid &mail_uuid) noexcept {
	PROFILE_ME;

	const auto it = m_mails.find(mail_uuid);
	if(it == m_mails.end()){
		return false;
	}
	it->second->set_expiry_time(0);
	m_mails.erase(it);
	return true;
}

}
