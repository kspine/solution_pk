#ifndef EMPERY_CENTER_SINGLETONS_MAIL_BOX_MAP_HPP_
#define EMPERY_CENTER_SINGLETONS_MAIL_BOX_MAP_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/container/flat_map.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

class MailBox;
class MailData;

struct MailBoxMap {
	static boost::shared_ptr<MailBox> get(AccountUuid account_uuid);
	static boost::shared_ptr<MailBox> require(AccountUuid account_uuid);
	static void unload(AccountUuid account_uuid);

	static boost::shared_ptr<MailBox> get_global();
	static boost::shared_ptr<MailBox> require_global();

	static boost::shared_ptr<MailData> get_mail_data(MailUuid mail_uuid, LanguageId language_id);
	static boost::shared_ptr<MailData> require_mail_data(MailUuid mail_uuid, LanguageId language_id);
	static void insert_mail_data(boost::shared_ptr<MailData> mail_data);

private:
	MailBoxMap() = delete;
};

}

#endif
