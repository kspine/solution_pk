#ifndef EMPERY_CENTER_MAIL_BOX_HPP_
#define EMPERY_CENTER_MAIL_BOX_HPP_

#include <poseidon/virtual_shared_from_this.hpp>
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
class PlayerSession;

class MailBox : public virtual Poseidon::VirtualSharedFromThis {
public:
	enum : std::uint64_t {
		FL_SYSTEM               = 0x0001,
		FL_READ                 = 0x0010,
		FL_ATTACHMENTS_FETCHED  = 0x0020,
	};

	struct MailInfo {
		MailUuid mail_uuid;
		std::uint64_t expiry_time;
		std::uint64_t flags;
	};

private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<MailUuid,
		boost::shared_ptr<MySql::Center_Mail>> m_mails;

public:
	MailBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Mail>> &mails);
	~MailBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	MailInfo get(MailUuid mail_uuid) const;
	void get_all(std::vector<MailInfo> &ret) const;

	void insert(const boost::shared_ptr<MailData> &mail_data, std::uint64_t expiry_time, boost::uint64_t flags);
	void update(MailInfo info, bool throws_if_not_exists = true);
	bool remove(MailUuid mail_uuid) noexcept;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_mail_box_with_player(const boost::shared_ptr<const MailBox> &mail_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	mail_box->synchronize_with_player(session);
}
inline void synchronize_mail_box_with_player(const boost::shared_ptr<MailBox> &mail_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	mail_box->synchronize_with_player(session);
}

}

#endif
