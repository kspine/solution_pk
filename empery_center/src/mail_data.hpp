#ifndef EMPERY_CENTER_MAIL_DATA_HPP_
#define EMPERY_CENTER_MAIL_DATA_HPP_

#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_util.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_MailData;
}

class MailData : NONCOPYABLE {
private:
	const boost::shared_ptr<MySql::Center_MailData> m_obj;

	boost::container::flat_map<ItemId, boost::uint64_t> m_attachments;

public:
	MailData(MailUuid mail_uuid, LanguageId language_id,
		unsigned type, AccountUuid from_account_uuid, std::string subject, std::string body,
		boost::container::flat_map<ItemId, boost::uint64_t> attachments);
	explicit MailData(boost::shared_ptr<MySql::Center_MailData> obj);
	~MailData();

public:
	MailUuid get_mail_uuid() const;
	LanguageId get_language_id() const;
	boost::uint64_t get_created_time() const;

	unsigned get_type() const;
	void set_type(unsigned type);

	AccountUuid get_from_account_uuid() const;
	void set_from_account_uuid(AccountUuid account_uuid);

	const std::string &get_subject() const;
	void set_subject(std::string subject);

	const std::string &get_body() const;
	void set_body(std::string body);

	const boost::container::flat_map<ItemId, boost::uint64_t> &get_attachments() const;
	void set_attachments(boost::container::flat_map<ItemId, boost::uint64_t> attachments);
};

}

#endif
