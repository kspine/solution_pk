#ifndef EMPERY_CENTER_MAIL_BOX_HPP_
#define EMPERY_CENTER_MAIL_BOX_HPP_

#include "abstract_data_object.hpp"
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Mail;
}

class MailData;

class MailBox : public virtual AbstractDataObject {
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
	void pump_status() override;

	void synchronize_with_client(const boost::shared_ptr<PlayerSession> &session) const;

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	MailInfo get(MailUuid mail_uuid) const;
	void get_all(std::vector<MailInfo> &ret) const;

	void insert(const boost::shared_ptr<MailData> &mail_data, boost::uint64_t expiry_time);
	void update(MailInfo info, bool throws_if_not_exists = true);
	bool remove(MailUuid mail_uuid) noexcept;
};

inline void synchronize_mail_box_with_client(const boost::shared_ptr<const MailBox> &mail_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	mail_box->synchronize_with_client(session);
}
inline void synchronize_mail_box_with_client(const boost::shared_ptr<MailBox> &mail_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	mail_box->synchronize_with_client(session);
}

}

#endif
