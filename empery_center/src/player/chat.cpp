#include "../precompiled.hpp"
#include "common.hpp"
#include "../msg/cs_chat.hpp"
#include "../msg/sc_chat.hpp"
#include "../msg/err_chat.hpp"
#include <poseidon/json.hpp>
#include <poseidon/async_job.hpp>
#include "../singletons/chat_box_map.hpp"
#include "../chat_box.hpp"
#include "../chat_message.hpp"
#include "../chat_channel_ids.hpp"
#include "../chat_message_type_ids.hpp"
#include "../chat_message_slot_ids.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "../data/global.hpp"
#include "../singletons/world_map.hpp"
#include "../horn_message.hpp"
#include "../msg/err_item.hpp"
#include "../data/item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../reason_ids.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_ChatSendMessage, account, session, req){
	const auto channel     = ChatChannelId(req.channel);
	const auto type        = ChatMessageTypeId(req.type);
	const auto language_id = LanguageId(req.language_id);

	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now < account->get_quieted_until()){
		return Response(Msg::ERR_ACCOUNT_QUIETED);
	}

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	segments.reserve(req.segments.size());
	for(auto it = req.segments.begin(); it != req.segments.end(); ++it){
		const auto slot = ChatMessageSlotId(it->slot);
		if((slot != ChatMessageSlotIds::ID_TEXT) && (slot != ChatMessageSlotIds::ID_SMILEY) && (slot != ChatMessageSlotIds::ID_VOICE)){
			return Response(Msg::ERR_INVALID_CHAT_MESSAGE_SLOT) <<slot;
		}
		segments.emplace_back(slot, std::move(it->value));
	}

	std::uint64_t last_chat_time;
	std::uint64_t min_seconds;
	if(channel == ChatChannelIds::ID_ADJACENT){
		last_chat_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_LAST_CHAT_TIME_ADJACENT);
		min_seconds = Data::Global::as_unsigned(Data::Global::SLOT_MIN_MESSAGE_INTERVAL_IN_ADJACENT_CHANNEL);
	} else if(channel == ChatChannelIds::ID_TRADE){
		last_chat_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_LAST_CHAT_TIME_TRADE);
		min_seconds = Data::Global::as_unsigned(Data::Global::SLOT_MIN_MESSAGE_INTERVAL_IN_TRADE_CHANNEL);
	} else if(channel == ChatChannelIds::ID_ALLIANCE){
		last_chat_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_LAST_CHAT_TIME_ALLIANCE);
		min_seconds = Data::Global::as_unsigned(Data::Global::SLOT_MIN_MESSAGE_INTERVAL_IN_ALLIANCE_CHANNEL);
	} else {
		return Response(Msg::ERR_CANNOT_SEND_TO_SYSTEM_CHANNEL) <<channel;
	}
	const auto milliseconds_remaining = saturated_sub(saturated_mul<std::uint64_t>(min_seconds, 1000), saturated_sub(utc_now, last_chat_time));
	if(milliseconds_remaining != 0){
		return Response(Msg::ERR_CHAT_FLOOD) <<milliseconds_remaining;
	}

	if(channel == ChatChannelIds::ID_ALLIANCE){
		// TODO check alliance
	}

	const auto chat_message_uuid = ChatMessageUuid(Poseidon::Uuid::random());
	const auto message = boost::make_shared<ChatMessage>(
		chat_message_uuid, channel, type, language_id, utc_now, account->get_account_uuid(), std::move(segments));

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers[AccountAttributeIds::ID_LAST_CHAT_TIME_ALLIANCE] = boost::lexical_cast<std::string>(utc_now);
	account->set_attributes(std::move(modifiers));

	if(channel == ChatChannelIds::ID_ADJACENT){
		const auto view = session->get_view();
		if((view.width() == 0) || (view.height() == 0)){
			LOG_EMPERY_CENTER_DEBUG("View is null: account_uuid = ", account->get_account_uuid());
		} else {
			const auto range = Data::Global::as_array(Data::Global::SLOT_ADJACENT_CHAT_RANGE);
			const auto width  = static_cast<std::uint64_t>(range.at(0).get<double>());
			const auto height = static_cast<std::uint64_t>(range.at(1).get<double>());

			const auto center_x = view.left()   + static_cast<std::int64_t>(view.width() / 2);
			const auto center_y = view.bottom() + static_cast<std::int64_t>(view.height() / 2);
			std::vector<boost::shared_ptr<PlayerSession>> other_sessions;
			WorldMap::get_players_viewing_rectangle(other_sessions,
				Rectangle(center_x - static_cast<std::int64_t>(width / 2), center_y - static_cast<std::int64_t>(height / 2), width, height));
			for(auto it = other_sessions.begin(); it != other_sessions.end(); ++it){
				const auto &other_session = *it;
				try {
					Poseidon::enqueue_async_job(
						std::bind(
							[](const boost::shared_ptr<PlayerSession> &other_session, const boost::shared_ptr<ChatMessage> &message){
								const auto other_account = PlayerSessionMap::require_account(other_session);
								const auto other_chat_box = ChatBoxMap::require(other_account->get_account_uuid());
								other_chat_box->insert(message);
							},
							other_session, message));
				} catch(std::exception &e){
					LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					other_session->shutdown(e.what());
				}
			}
		}
	} else if(channel == ChatChannelIds::ID_TRADE){
	} else if(channel == ChatChannelIds::ID_ALLIANCE){
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ChatGetMessages, account, session, req){
	const auto chat_box = ChatBoxMap::require(account->get_account_uuid());

	Msg::SC_ChatGetMessagesRet msg;
	msg.chat_messages.reserve(req.chat_messages.size());

	// const auto language_id = LanguageId(req.language_id);
	for(auto it = req.chat_messages.begin(); it != req.chat_messages.end(); ++it){
		const auto chat_message_uuid = ChatMessageUuid(it->chat_message_uuid);

		auto &chat_message = *msg.chat_messages.emplace(msg.chat_messages.end());
		chat_message.chat_message_uuid = chat_message_uuid.str();
		chat_message.error_code = Msg::ERR_NO_SUCH_CHAT_MESSAGE;

		const auto message = chat_box->get(chat_message_uuid);
		if(!message){
			continue;
		}
		synchronize_chat_message_with_player(message, session);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ChatGetHornMessages, account, session, req){
	const auto language_id = LanguageId(req.language_id);

	std::vector<boost::shared_ptr<HornMessage>> horn_messages;
	ChatBoxMap::get_horn_messages_by_language_id(horn_messages, language_id);
	for(auto it = horn_messages.begin(); it != horn_messages.end(); ++it){
		const auto &horn_message = *it;
		horn_message->synchronize_with_player(session);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_ChatHornMessage, account, session, req){
	const auto item_id = ItemId(req.item_id);
	const auto item_data = Data::Item::get(item_id);
	if(item_data->type.first != Data::Item::CAT_HORN){
		return Response(Msg::ERR_ITEM_TYPE_MISMATCH) <<(unsigned)Data::Item::CAT_HORN;
	}

	const auto account_uuid = account->get_account_uuid();
	const auto item_box = ItemBoxMap::require(account_uuid);

	const auto horn_message_uuid = HornMessageUuid(Poseidon::Uuid::random());
	const auto language_id = LanguageId(req.language_id);
	const auto utc_now = Poseidon::get_utc_time();
	const auto expiry_time = saturated_add(utc_now, item_data->value);

	std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
	segments.reserve(req.segments.size());
	for(auto it = req.segments.begin(); it != req.segments.end(); ++it){
		segments.emplace_back(ChatMessageSlotId(it->slot), std::move(it->value));
	}

	std::vector<ItemTransactionElement> transaction;
	transaction.emplace_back(ItemTransactionElement::OP_REMOVE, item_id, 1,
		ReasonIds::ID_HORN_MESSAGE, 0, 0, 0);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{
			const auto horn_message = boost::make_shared<HornMessage>(horn_message_uuid,
				language_id, utc_now, expiry_time, account_uuid, std::move(segments));
			ChatBoxMap::insert_horn_message(horn_message);
		});
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

}
