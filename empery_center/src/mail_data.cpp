#include "precompiled.hpp"
#include "mail_data.hpp"
#include <poseidon/json.hpp>
#include "mysql/mail.hpp"

namespace EmperyCenter {

namespace {
	std::string encode_attachments(const boost::container::flat_map<ItemId, boost::uint64_t> &attachments){
		PROFILE_ME;

		if(attachments.empty()){
			return { };
		}
		Poseidon::JsonObject root;
		for(auto it = attachments.begin(); it != attachments.end(); ++it){
			char str[256];
			auto len = (unsigned)std::sprintf(str, "%lu", static_cast<unsigned long>(it->first.get()));
			root[SharedNts(str, len)] = it->second;
		}
		std::ostringstream oss;
		root.dump(oss);
		return oss.str();
	}
	boost::container::flat_map<ItemId, boost::uint64_t> decode_attachments(const std::string &str){
		PROFILE_ME;

		boost::container::flat_map<ItemId, boost::uint64_t> attachments;
		if(str.empty()){
			return attachments;
		}
		std::istringstream iss(str);
		auto root = Poseidon::JsonParser::parse_object(iss);
		attachments.reserve(root.size());
		for(auto it = root.begin(); it != root.end(); ++it){
			const auto item_id = boost::lexical_cast<ItemId>(it->first);
			const auto item_count = static_cast<boost::uint64_t>(it->second.get<double>());
			attachments[item_id] = item_count;
		}
		return attachments;
	}
}

MailData::MailData(MailUuid mail_uuid, LanguageId language_id,
	unsigned type, AccountUuid from_account_uuid, std::string subject, std::string body,
	boost::container::flat_map<ItemId, boost::uint64_t> attachments)
	: m_obj([&]{
		auto obj = boost::make_shared<MySql::Center_MailData>(mail_uuid.get(), language_id.get(),
			Poseidon::get_utc_time(), type, from_account_uuid.get(), std::move(subject), std::move(body), encode_attachments(attachments));
		obj->async_save(true);
		return obj;
	}())
	, m_attachments(std::move(attachments))
{
}
MailData::MailData(boost::shared_ptr<MySql::Center_MailData> obj)
	: m_obj(std::move(obj))
	, m_attachments(decode_attachments(m_obj->unlocked_get_attachments()))
{
}
MailData::~MailData(){
}

MailUuid MailData::get_mail_uuid() const {
	return MailUuid(m_obj->unlocked_get_mail_uuid());
}
LanguageId MailData::get_language_id() const {
	return LanguageId(m_obj->get_language_id());
}
boost::uint64_t MailData::get_created_time() const {
	return m_obj->get_created_time();
}

unsigned MailData::get_type() const {
	return m_obj->get_type();
}
void MailData::set_type(unsigned type){
	m_obj->set_type(type);
}

AccountUuid MailData::get_from_account_uuid() const {
	return AccountUuid(m_obj->unlocked_get_from_account_uuid());
}
void MailData::set_from_account_uuid(AccountUuid account_uuid){
	m_obj->set_from_account_uuid(account_uuid.get());
}

const std::string &MailData::get_subject() const {
	return m_obj->unlocked_get_subject();
}
void MailData::set_subject(std::string subject){
	m_obj->set_subject(std::move(subject));
}

const std::string &MailData::get_body() const {
	return m_obj->unlocked_get_body();
}
void MailData::set_body(std::string body){
	m_obj->set_body(std::move(body));
}

const boost::container::flat_map<ItemId, boost::uint64_t> &MailData::get_attachments() const {
	return m_attachments;
}
void MailData::set_attachments(boost::container::flat_map<ItemId, boost::uint64_t> attachments){
	auto str = encode_attachments(attachments);
	m_attachments = std::move(attachments);
	m_obj->set_attachments(std::move(str));
}

}
