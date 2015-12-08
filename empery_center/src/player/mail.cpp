#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/endian.hpp>
#include "../msg/cs_mail.hpp"
#include "../msg/sc_mail.hpp"
#include "../msg/err_mail.hpp"
#include "../singletons/mail_box_map.hpp"
#include "../mail_box.hpp"
#include "../mail_data.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../singletons/item_box_map.hpp"
#include "../reason_ids.hpp"
#include "../data/global.hpp"
#include "../mail_type_ids.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_MailGetAllMails, account_uuid, session, /* req */){
	const auto mail_box = MailBoxMap::require(account_uuid);
	mail_box->pump_status();
	mail_box->synchronize_with_player(session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailGetMailData, account_uuid, session, req){
	const auto mail_box = MailBoxMap::require(account_uuid);
	mail_box->pump_status();

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::ERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	const auto language_id = LanguageId(req.language_id);
	const auto mail_data = MailBoxMap::get_mail_data(mail_uuid, language_id);
	if(!mail_data){
		LOG_EMPERY_CENTER_DEBUG("Mail data not found: mail_uuid = ", mail_uuid, ", language_id = ", language_id);
		return Response(Msg::ERR_NO_SUCH_LANGUAGE_ID) <<mail_uuid;
	}

	synchronize_mail_data_with_player(mail_data, session);

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailWriteToAccount, account_uuid, session, req){
	const auto to_account_uuid = AccountUuid(req.to_account_uuid);
	if(!AccountMap::has(to_account_uuid)){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<to_account_uuid;
	}

	const auto to_mail_box = MailBoxMap::require(to_account_uuid);
	to_mail_box->pump_status();

	const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
	const auto language_id = LanguageId(req.language_id);

	const auto expiry_duration = checked_mul(Data::Global::as_unsigned(Data::Global::SLOT_DEFAULT_MAIL_EXPIRY_DURATION), (boost::uint64_t)60000);
	const auto utc_now = Poseidon::get_utc_time();

	const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
		MailTypeIds::ID_NORMAL, account_uuid, std::move(req.subject), std::move(req.body),
		boost::container::flat_map<ItemId, boost::uint64_t>());
	MailBoxMap::insert_mail_data(mail_data);
	to_mail_box->insert(mail_data, saturated_add(utc_now, expiry_duration));

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
	mail_box->pump_status();

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::ERR_NO_SUCH_MAIL) <<mail_uuid;
	}

	Poseidon::add_flags(info.flags, MailBox::FL_READ);
	mail_box->update(std::move(info));

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailFetchAttachments, account_uuid, session, req){
	const auto mail_box = MailBoxMap::require(account_uuid);
	mail_box->pump_status();

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::ERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	if(Poseidon::has_any_flags_of(info.flags, MailBox::FL_ATTACHMENTS_FETCHED)){
		return Response(Msg::ERR_ATTACHMENTS_FETCHED) <<mail_uuid;
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
	item_box->commit_transaction(transaction,
		[&]{ mail_box->update(std::move(info)); });

	return Response();
}

PLAYER_SERVLET(Msg::CS_MailDelete, account_uuid, session, req){
	const auto mail_box = MailBoxMap::require(account_uuid);
	mail_box->pump_status();

	const auto mail_uuid = MailUuid(req.mail_uuid);
	auto info = mail_box->get(mail_uuid);
	if(info.expiry_time == 0){
		return Response(Msg::ERR_NO_SUCH_MAIL) <<mail_uuid;
	}
	if(Poseidon::has_none_flags_of(info.flags, MailBox::FL_READ)){
		return Response(Msg::ERR_MAIL_IS_UNREAD) <<mail_uuid;
	}
	if(Poseidon::has_none_flags_of(info.flags, MailBox::FL_ATTACHMENTS_FETCHED)){
		return Response(Msg::ERR_MAIL_HAS_ATTACHMENTS) <<mail_uuid;
	}

	mail_box->remove(mail_uuid);

	return Response();
}

}
