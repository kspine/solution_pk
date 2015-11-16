#ifndef EMPERY_CENTER_MAIL_BOX_HPP_
#define EMPERY_CENTER_MAIL_BOX_HPP_

#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_util.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Mail;
}

class MailData;

class MailBox : NONCOPYABLE {
public:
	enum : boost::uint64_t {
		FL_SYSTEM               = 0x0001,
		FL_READ                 = 0x0010,
		FL_ATTACHMENTS_FETCHED  = 0x0020,
	};

	struct MailInfo {
		MailUuid mail_uuid;
		boost::uint64_t expiry_time;
		boost::uint64_t flags;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<MailUuid,
		boost::shared_ptr<MySql::Center_Mail>> m_mails;

public:
	explicit MailBox(AccountUuid account_uuid);
	MailBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Mail>> &mails);
	~MailBox();

public:
	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void pump_status(bool force_synchronization_with_client = false);

	MailInfo get(MailUuid mail_uuid) const;
	void get_all(std::vector<MailInfo> &ret) const;

	void insert(const boost::shared_ptr<MailData> &mail_data, boost::uint64_t expiry_time);
	void update(MailInfo info);
	bool remove(MailUuid mail_uuid) noexcept;
};

}

#endif
