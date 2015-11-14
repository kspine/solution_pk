#ifndef EMPERY_CENTER_MAIL_BOX_HPP_
#define EMPERY_CENTER_MAIL_BOX_HPP_

#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_util.hpp>
#include <boost/function.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Mail;
}

class MailBox : NONCOPYABLE {
public:
	enum : boost::uint64_t {
		FL_SYSTEM               = 0x0001,
		FL_READ                 = 0x0010,
		FL_ATTACHMENT_FETCHED   = 0x0020,
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
	explicit MailBox(const AccountUuid &account_uuid);
	MailBox(const AccountUuid &account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Mail>> &mails);
	~MailBox();

public:
	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void pump_status(bool force_synchronization_with_client = false);

	MailInfo get(const MailUuid &mail_uuid) const;
	void get_all(std::vector<MailInfo> &ret) const;

	bool create(const MailInfo &info);
	bool remove(const MailUuid &mail_uuid) noexcept;
};

}

#endif
