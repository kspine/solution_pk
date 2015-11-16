#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_mail.hpp"
#include "../msg/sc_mail.hpp"
#include "../msg/cerr_mail.hpp"
#include "../singletons/mail_box_map.hpp"
#include "../mail_box.hpp"
#include "../mail_data.hpp"
#include "../item_box.hpp"
#include "../item_transaction_element.hpp"
#include "../singletons/item_box_map.hpp"
#include "../reason_ids.hpp"
#include <poseidon/endian.hpp>

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MailGetAllMails, account_uuid, session, /* req */){
	const auto mail_box = MailBoxMap::require(account_uuid);
	mail_box->pump_status(true);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailGetMailData, account_uuid, session, req){
	const auto mail_box = MailBoxMap::require(account_uuid);

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::CERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	const auto language_id = LanguageId(req.language_id);
	const auto mail_data = MailBoxMap::get_mail_data(mail_uuid, language_id);
	if(!mail_data){
		LOG_EMPERY_CENTER_DEBUG("Mail data not found: mail_uuid = ", mail_uuid, ", language_id = ", language_id);
		return Response(Msg::CERR_NO_SUCH_LANGUAGE_ID) <<mail_uuid;
	}

	Msg::SC_MailData msg;
	msg.mail_uuid     = mail_data->get_mail_uuid().str();
	msg.language_id   = mail_data->get_language_id().get();
	msg.created_time  = mail_data->get_created_time();
	msg.type          = mail_data->get_type();
	msg.subject       = mail_data->get_subject();
	msg.body          = mail_data->get_body();
	const auto &attachments = mail_data->get_attachments();
	msg.attachments.reserve(attachments.size());
	for(auto it = attachments.begin(); it != attachments.end(); ++it){
		msg.attachments.emplace_back();
		auto &attachment = msg.attachments.back();
		attachment.item_id    = it->first.get();
		attachment.item_count = it->second;
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailWriteToAccount, account_uuid, session, req){
	const auto to_account_uuid = AccountUuid(req.to_account_uuid);
	if(!AccountMap::has(to_account_uuid)){
		return Response(Msg::CERR_NO_SUCH_ACCOUNT) <<to_account_uuid;
	}

	const auto to_mail_box = MailBoxMap::require(to_account_uuid);

	const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
	const auto language_id = LanguageId(req.language_id);

	const auto default_expiry_duration = get_config<boost::uint64_t>("default_mail_expiry_duration", 604800000);
	const auto utc_now = Poseidon::get_utc_time();

	const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id,
		0, account_uuid, std::move(req.subject), std::move(req.body), boost::container::flat_map<ItemId, boost::uint64_t>());
	MailBoxMap::insert_mail_data(mail_data);
	to_mail_box->insert(mail_data, saturated_add(utc_now, default_expiry_duration));

	const auto to_session = PlayerSessionMap::get(to_account_uuid);
	if(to_session){
		try {
			to_session->send(Msg::SC_MailNewMailReceived(mail_uuid.str()));
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			to_session->shutdown(e.what());
		}
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailMarkAsRead, account_uuid, session, req){
	const auto mail_box = MailBoxMap::require(account_uuid);

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::CERR_NO_SUCH_MAIL) <<mail_uuid;
	}

	Poseidon::add_flags(info.flags, MailBox::FL_READ);
	mail_box->update(std::move(info));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailFetchAttachments, account_uuid, session, req){
	const auto mail_box = MailBoxMap::require(account_uuid);

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::CERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	if(Poseidon::has_any_flags_of(info.flags, MailBox::FL_ATTACHMENTS_FETCHED)){
		return Response(Msg::CERR_ATTACHMENTS_FETCHED) <<mail_uuid;
	}

	const auto item_box = ItemBoxMap::require(account_uuid);

	const auto language_id = LanguageId(req.language_id);
	const auto mail_data = MailBoxMap::require_mail_data(mail_uuid, language_id);

	const auto &attachments = mail_data->get_attachments();
	LOG_EMPERY_CENTER_DEBUG("Unpacking mail attachments: account_uuid = ", account_uuid,
		", mail_uuid = ", mail_uuid, ", language_id = ", language_id);

	const auto mail_uuid_tail = Poseidon::load_be(reinterpret_cast<const boost::uint64_t *>(mail_uuid.get().end())[-1]);
	std::vector<ItemTransactionElement> transaction;
	transaction.reserve(attachments.size());
	for(auto it = attachments.begin(); it != attachments.end(); ++it){
		transaction.emplace_back(ItemTransactionElement::OP_ADD, it->first, it->second,
			ReasonIds::ID_MAIL_ATTACHMENTS, mail_uuid_tail, language_id.get(), 0);
	}
	Poseidon::add_flags(info.flags, MailBox::FL_ATTACHMENTS_FETCHED);
	item_box->commit_transaction(transaction.data(), transaction.size(),
		[&]{ mail_box->update(std::move(info)); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailDelete, account_uuid, session, req){
	const auto mail_box = MailBoxMap::require(account_uuid);

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::CERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	if(Poseidon::has_none_flags_of(info.flags, MailBox::FL_READ)){
		return Response(Msg::CERR_MAIL_IS_UNREAD) <<mail_uuid;
	}
	if(Poseidon::has_none_flags_of(info.flags, MailBox::FL_ATTACHMENTS_FETCHED)){
		return Response(Msg::CERR_MAIL_HAS_ATTACHMENTS) <<mail_uuid;
	}

	mail_box->remove(mail_uuid);

	return Response();
}

}
