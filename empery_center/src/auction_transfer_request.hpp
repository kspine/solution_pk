#ifndef EMPERY_CENTER_TRANSFER_REQUEST_HPP_
#define EMPERY_CENTER_TRANSFER_REQUEST_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include <poseidon/cxx_util.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_AuctionTransferRequest;
	class Center_AuctionTransferRequestItem;
}

class AuctionTransferRequest : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const boost::shared_ptr<MySql::Center_AuctionTransferRequest> m_obj;

	boost::container::flat_map<ItemId, boost::shared_ptr<MySql::Center_AuctionTransferRequestItem> m_items;

public:
	AuctionTransferRequest(AccountUuid account_uuid, std::uint64_t created_time,
		
		MailTypeId type, AccountUuid from_account_uuid, std::string subject,
		std::vector<std::pair<ChatMessageSlotId, std::string>> segments, boost::container::flat_map<ItemId, std::uint64_t> attachments);
	explicit MailData(boost::shared_ptr<MySql::Center_MailData> obj);
	~MailData();

public:
	MailUuid get_mail_uuid() const;
	LanguageId get_language_id() const;
	std::uint64_t get_created_time() const;

	MailTypeId get_type() const;
	void set_type(MailTypeId type);

	AccountUuid get_from_account_uuid() const;
	void set_from_account_uuid(AccountUuid account_uuid);

	const std::string &get_subject() const;
	void set_subject(std::string subject);

	const std::vector<std::pair<ChatMessageSlotId, std::string>> &get_segments() const;
	void set_segments(std::vector<std::pair<ChatMessageSlotId, std::string>> segments);

	const boost::container::flat_map<ItemId, std::uint64_t> &get_attachments() const;
	void set_attachments(boost::container::flat_map<ItemId, std::uint64_t> attachments);
};

}

#endif
