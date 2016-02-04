#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_mail.hpp"
#include "../msg/sc_mail.hpp"
#include "../msg/err_mail.hpp"
#include "../msg/err_chat.hpp"
#include "../singletons/mail_box_map.hpp"
#include "../mail_box.hpp"
#include "../mail_data.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../singletons/item_box_map.hpp"
#include "../reason_ids.hpp"
#include "../data/global.hpp"
#include "../chat_message_type_ids.hpp"
#include "../chat_message_slot_ids.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MailGetAllMails, account, session, /* req */){
	const auto mail_box = MailBoxMap::require(account->get_account_uuid());
	mail_box->pump_status();
	mail_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailGetMailData, account, session, req){
	const auto mail_box = MailBoxMap::require(account->get_account_uuid());
	mail_box->pump_status();

	for(auto it = req.mails.begin(); it != req.mails.end(); ++it){
		const auto mail_uuid = MailUuid(it->mail_uuid);

		auto info = mail_box->get(mail_uuid);
		if(info.expiry_time == 0){
			continue;
		}

		const auto language_id = LanguageId(it->language_id);
		const auto mail_data = MailBoxMap::get_mail_data(mail_uuid, language_id);
		if(!mail_data){
			LOG_EMPERY_CENTER_ERROR("Mail data not found: mail_uuid = ", mail_uuid, ", language_id = ", language_id);
			continue;
		}

		if(mail_data->get_attachments().empty()){
			if(!info.attachments_fetched){
				LOG_EMPERY_CENTER_DEBUG("Mail has no attachments. Lazy update: account_uuid = ", account->get_account_uuid(),
					", mail_uuid = ", mail_uuid, ", language_id = ", language_id);
				info.attachments_fetched = true;
				mail_box->update(std::move(info));
			}
		}
		synchronize_mail_data_with_player(mail_data, session);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailWriteToAccount, account, session, req){
	const auto to_account_uuid = AccountUuid(req.to_account_uuid);

	const auto to_account = AccountMap::get(to_account_uuid);
	if(!to_account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<to_account_uuid;
	}

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	segments.reserve(req.segments.size());
	for(auto it = req.segments.begin(); it != req.segments.end(); ++it){
		const auto slot = ChatMessageSlotId(it->slot);
		if((slot != ChatMessageSlotIds::ID_TEXT) && (slot != ChatMessageSlotIds::ID_SMILEY)){
			return Response(Msg::ERR_INVALID_CHAT_MESSAGE_SLOT) <<slot;
		}
		segments.emplace_back(slot, std::move(it->value));
	}

	boost::container::flat_map<ItemId, std::uint64_t> attachments;

	const auto to_mail_box = MailBoxMap::require(to_account->get_account_uuid());
	to_mail_box->pump_status();

	const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
	const auto language_id = LanguageId(req.language_id);

	const auto default_mail_expiry_duration = Data::Global::as_unsigned(Data::Global::SLOT_DEFAULT_MAIL_EXPIRY_DURATION);
	const auto expiry_duration = checked_mul(default_mail_expiry_duration, (std::uint64_t)60000);
	const auto utc_now = Poseidon::get_utc_time();

	const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
		ChatMessageTypeIds::ID_PLAIN, account->get_account_uuid(), std::move(req.subject), std::move(segments), std::move(attachments));
	MailBoxMap::insert_mail_data(mail_data);

	MailBox::MailInfo mail_info = { };
	mail_info.mail_uuid   = mail_uuid;
	mail_info.expiry_time = saturated_add(utc_now, expiry_duration);
	to_mail_box->insert(std::move(mail_info));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailMarkAsRead, account, session, req){
	const auto mail_box = MailBoxMap::require(account->get_account_uuid());
	mail_box->pump_status();

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::ERR_NO_SUCH_MAIL) <<mail_uuid;
	}

	info.read = true;
	mail_box->update(std::move(info));

	return Response();
}

namespace {
	void really_fetch_attachments(const boost::shared_ptr<ItemBox> &item_box,
		const boost::shared_ptr<MailBox> &mail_box, MailBox::MailInfo &info, LanguageId language_id)
	{
		PROFILE_ME;

		const auto &mail_uuid = info.mail_uuid;
		const auto mail_data = MailBoxMap::require_mail_data(mail_uuid, language_id);

		const auto &attachments = mail_data->get_attachments();
		LOG_EMPERY_CENTER_DEBUG("Unpacking mail attachments: account_uuid = ", mail_box->get_account_uuid(),
			", mail_uuid = ", mail_uuid, ", language_id = ", language_id);

		const auto mail_uuid_head = Poseidon::load_be(reinterpret_cast<const std::uint64_t &>(mail_uuid.get()[0]));
		std::vector<ItemTransactionElement> transaction;
		transaction.reserve(attachments.size());
		for(auto it = attachments.begin(); it != attachments.end(); ++it){
			transaction.emplace_back(ItemTransactionElement::OP_ADD, it->first, it->second,
				ReasonIds::ID_MAIL_ATTACHMENTS, mail_uuid_head, language_id.get(), mail_data->get_type().get());
		}
		item_box->commit_transaction(transaction, false,
			[&]{
				info.attachments_fetched = true;
				mail_box->update(std::move(info));
			});
	}
}

PLAYER_SERVLET(Msg::CS_MailFetchAttachments, account, session, req){
	const auto language_id = LanguageId(req.language_id);

	const auto mail_box = MailBoxMap::require(account->get_account_uuid());
	mail_box->pump_status();

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::ERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	if(info.attachments_fetched){
		return Response(Msg::ERR_ATTACHMENTS_FETCHED) <<mail_uuid;
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	really_fetch_attachments(item_box, mail_box, info, language_id);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailDelete, account, session, req){
	const auto mail_box = MailBoxMap::require(account->get_account_uuid());
	mail_box->pump_status();

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::ERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	if(!info.read){
		return Response(Msg::ERR_MAIL_IS_UNREAD) <<mail_uuid;
	}
	if(!info.attachments_fetched){
		return Response(Msg::ERR_MAIL_HAS_ATTACHMENTS) <<mail_uuid;
	}

	mail_box->remove(mail_uuid);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailBatchReadAndFetchAttachments, account, session, req){
	const auto language_id = LanguageId(req.language_id);

	const auto mail_box = MailBoxMap::require(account->get_account_uuid());
	mail_box->pump_status();

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	std::vector<MailBox::MailInfo> mail_infos;
	mail_box->get_all(mail_infos);
	for(auto it = mail_infos.begin(); it != mail_infos.end(); ++it){
		auto &info = *it;
		if(info.expiry_time == 0){
			continue;
		}
		if(info.read){
			continue;
		}

		info.read = true;
		mail_box->update(std::move(info));
	}
	for(auto it = mail_infos.begin(); it != mail_infos.end(); ++it){
		auto &info = *it;
		if(info.expiry_time == 0){
			continue;
		}
		if(info.attachments_fetched){
			continue;
		}

		really_fetch_attachments(item_box, mail_box, info, language_id);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailBatchDelete, account, session, /* req */){
	const auto mail_box = MailBoxMap::require(account->get_account_uuid());
	mail_box->pump_status();

	std::vector<MailBox::MailInfo> mail_infos;
	mail_box->get_all(mail_infos);
	for(auto it = mail_infos.begin(); it != mail_infos.end(); ++it){
		auto &info = *it;
		if(info.expiry_time == 0){
			continue;
		}
		if(!info.read){
			continue;
		}
		if(!info.attachments_fetched){
			continue;
		}

		const auto mail_uuid = info.mail_uuid;
		mail_box->remove(mail_uuid);
	}

	return Response();
}

}
