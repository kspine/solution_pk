#ifndef EMPERY_CENTER_MAIL_BOX_HPP_
#define EMPERY_CENTER_MAIL_BOX_HPP_

#include <poseidon/cxx_util.hpp>
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

class MailBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	struct MailInfo {
		MailUuid mail_uuid;
		std::uint64_t expiry_time;
		bool system;
		bool read;
		bool attachments_fetched;
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

	void insert(MailInfo info);
	void update(MailInfo info, bool throws_if_not_exists = true);
	bool remove(MailUuid mail_uuid) noexcept;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

}

#endif
