#include "precompiled.hpp"
#include "mail_box.hpp"
#include "msg/sc_mail.hpp"
#include "mysql/mail.hpp"
#include "singletons/mail_box_map.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

namespace {
	void fill_mail_info(MailBox::MailInfo &info, const boost::shared_ptr<MySql::Center_Mail> &obj){
		PROFILE_ME;

		info.mail_uuid           = MailUuid(obj->get_mail_uuid());
		info.expiry_time         = obj->get_expiry_time();
		info.system              = obj->get_system();
		info.read                = obj->get_read();
		info.attachments_fetched = obj->get_attachments_fetched();
	}

	void fill_mail_message(Msg::SC_MailChanged &msg, const boost::shared_ptr<MySql::Center_Mail> &obj, std::uint64_t utc_now){
		PROFILE_ME;

		enum {
			FL_SYSTEM               = 0x0001,
			FL_READ                 = 0x0010,
			FL_ATTACHMENTS_FETCHED  = 0x0020,
		};

		std::uint64_t flags = 0;
		if(obj->get_system()){
			flags |= FL_SYSTEM;
		}
		if(obj->get_read()){
			flags |= FL_READ;
		}
		if(obj->get_attachments_fetched()){
			flags |= FL_ATTACHMENTS_FETCHED;
		}

		msg.mail_uuid       = obj->unlocked_get_mail_uuid().to_string();
		msg.expiry_duration = saturated_sub(obj->get_expiry_time(), utc_now);
		msg.flags           = flags;
	}
}

MailBox::MailBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Mail>> &mails)
	: m_account_uuid(account_uuid)
{
	for(auto it = mails.begin(); it != mails.end(); ++it){
		const auto &obj = *it;
		m_mails.emplace(MailUuid(obj->get_mail_uuid()), obj);
	}
}
MailBox::~MailBox(){
}

void MailBox::pump_status(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	const auto global_mail_box = MailBoxMap::get_global();
	if(global_mail_box && (global_mail_box.get() != this)){
		LOG_EMPERY_CENTER_TRACE("Checking for system mails: account_uuid = ", get_account_uuid());

		for(auto it = global_mail_box->m_mails.begin(); it != global_mail_box->m_mails.end(); ++it){
			const auto &src = it->second;
			if(src->get_expiry_time() < utc_now){
				continue;
			}
			const auto my_it = m_mails.find(it->first);
			if(my_it != m_mails.end()){
				continue;
			}
			LOG_EMPERY_CENTER_DEBUG("> Creating system mail: account_uuid = ", get_account_uuid(), ", mail_uuid = ", it->first);
			auto obj = boost::make_shared<MySql::Center_Mail>(it->first.get(), get_account_uuid().get(),
				src->get_expiry_time(), true, false, false);
			obj->async_save(true);
			m_mails.emplace(it->first, std::move(obj));
		}
	}

	LOG_EMPERY_CENTER_TRACE("Checking for expired mails: account_uuid = ", get_account_uuid());
	{
		auto it = m_mails.begin();
		while(it != m_mails.end()){
			const auto &obj = it->second;
			if(obj->get_system() || (utc_now < obj->get_expiry_time()) || !obj->get_read() || !obj->get_attachments_fetched()){
				++it;
			} else {
				LOG_EMPERY_CENTER_DEBUG("> Removing expired mail: account_uuid = ", get_account_uuid(), ", mail_uuid = ", it->first);
				it = m_mails.erase(it);
			}
		}
	}
}

MailBox::MailInfo MailBox::get(MailUuid mail_uuid) const {
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

void MailBox::insert(MailInfo info){
	PROFILE_ME;

	const auto mail_uuid = info.mail_uuid;
	const auto it = m_mails.find(mail_uuid);
	if(it != m_mails.end()){
		LOG_EMPERY_CENTER_WARNING("Mail exists: account_uuid = ", get_account_uuid(), ", mail_uuid = ", mail_uuid);
		DEBUG_THROW(Exception, sslit("Mail exists"));
	}

	const auto utc_now = Poseidon::get_utc_time();

	const auto obj = boost::make_shared<MySql::Center_Mail>(mail_uuid.get(), get_account_uuid().get(),
		info.expiry_time, info.system, info.read, info.attachments_fetched);
	obj->async_save(true);
	m_mails.emplace(mail_uuid, obj);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_MailChanged cmsg;
			fill_mail_message(cmsg, obj, utc_now);
			session->send(cmsg);

			Msg::SC_MailNewMailReceived rmsg;
			rmsg.mail_uuid = mail_uuid.str();
			session->send(rmsg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void MailBox::update(MailInfo info, bool throws_if_not_exists){
	PROFILE_ME;

	const auto it = m_mails.find(info.mail_uuid);
	if(it == m_mails.end()){
		LOG_EMPERY_CENTER_WARNING("Mail not found: account_uuid = ", get_account_uuid(), ", mail_uuid = ", info.mail_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Mail not found"));
		}
		return;
	}
	const auto &obj = it->second;

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_expiry_time(info.expiry_time);
	obj->set_system(info.system);
	obj->set_read(info.read);
	obj->set_attachments_fetched(info.attachments_fetched);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_MailChanged msg;
			fill_mail_message(msg, obj, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
bool MailBox::remove(MailUuid mail_uuid) noexcept {
	PROFILE_ME;

	const auto it = m_mails.find(mail_uuid);
	if(it == m_mails.end()){
		return false;
	}
	const auto obj = it->second;
	// 为系统邮件保留一个标记。
	if(!obj->get_system()){
		m_mails.erase(it);
	}

	const auto utc_now = Poseidon::get_utc_time();

	obj->set_expiry_time(0);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_MailChanged msg;
			fill_mail_message(msg, obj, utc_now);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return true;
}

void MailBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	for(auto it = m_mails.begin(); it != m_mails.end(); ++it){
		const auto &obj = it->second;
		if(utc_now >= obj->get_expiry_time()){
			continue;
		}
		Msg::SC_MailChanged msg;
		fill_mail_message(msg, it->second, utc_now);
		session->send(msg);
	}
}

}
