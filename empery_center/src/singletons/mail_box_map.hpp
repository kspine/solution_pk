#ifndef EMPERY_CENTER_SINGLETONS_MAIL_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_MAIL_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class MailBox;

struct MailBoxMap {
	static boost::shared_ptr<MailBox> get(const AccountUuid &account_uuid);
	static boost::shared_ptr<MailBox> require(const AccountUuid &account_uuid);

	static boost::shared_ptr<MailBox> get_global();
	static boost::shared_ptr<MailBox> require_global();

	struct MailData {
		MailUuid mail_uuid;
		LanguageId language_id;
		boost::uint64_t created_time;
		unsigned type;
		AccountUuid from_account_uuid;
		std::string subject;
		std::string body;
		boost::container::flat_map<ItemId, boost::uint64_t> attachments;
	};

	static boost::shared_ptr<const MailData> get_mail_data(const MailUuid &mail_uuid, LanguageId language_id);
	// 覆盖任何现有数据。
	static void set_mail_data(const MailData &mail_data);

private:
	MailBoxMap() = delete;
};

}

#endif
