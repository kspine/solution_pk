#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include <poseidon/async_job.hpp>
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/err_account.hpp"
#include "../msg/err_item.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "../data/signing_in.hpp"
#include "../item_ids.hpp"
#include "../data/item.hpp"
#include "../data/global.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../checked_arithmetic.hpp"
#include "../events/account.hpp"
#include "../singletons/war_status_map.hpp"
#include "../singletons/controller_client.hpp"
#include "../singletons/world_map.hpp"

namespace EmperyCenter {

namespace {
	std::pair<long, std::string> AccountLoginServlet(const boost::shared_ptr<PlayerSession> &session, Poseidon::StreamBuffer payload){
		PROFILE_ME;
		Msg::CS_AccountLogin req(payload);
		LOG_EMPERY_CENTER_TRACE("Received request from ", session->get_remote_info(), ": ", req);
// ============================================================================
{
	const auto platform_id  = PlatformId(req.platform_id);
	const auto &login_name  = req.login_name;
	const auto &login_token = req.login_token;

	LOG_EMPERY_CENTER_DEBUG("Account login: platform_id = ", platform_id, ", login_name = ", login_name, ", login_token = ", login_token);

	const auto old_account_uuid = PlayerSessionMap::get_account_uuid(session);
	if(old_account_uuid){
		return Response(Msg::ERR_MULTIPLE_LOGIN) <<old_account_uuid;
	}

	const auto account = AccountMap::forced_reload_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_LOGIN_NAME) <<login_name;
	}
	if(!account->has_been_activated()){
		return Response(Msg::ERR_ACTIVATE_YOUR_ACCOUNT) <<login_name;
	}
	const auto account_uuid = account->get_account_uuid();

	const auto third_error_code = boost::make_shared<long>(Msg::ST_OK);
	Poseidon::sync_raise_event(
		boost::make_shared<Events::AccountSynchronizeWithThirdServer>(third_error_code,
			account->get_platform_id(), account->get_login_name(), account->get_attribute(AccountAttributeIds::ID_SAVED_THIRD_TOKEN)));
	if(*third_error_code != Msg::ST_OK){
		LOG_EMPERY_CENTER_DEBUG("Third server returned an error: third_error_code = ", *third_error_code);
		return Response(Msg::ERR_TOKEN_INVALIDATED) <<login_name;
	}

	AccountMap::require_controller_token(account_uuid);

	const auto utc_now = Poseidon::get_utc_time();
	const auto expected_token_expiry_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_LOGIN_TOKEN_EXPIRY_TIME);
	if(utc_now >= expected_token_expiry_time){
		return Response(Msg::ERR_TOKEN_EXPIRED) <<login_name;
	}
	if(login_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	const auto &expected_token = account->get_attribute(AccountAttributeIds::ID_LOGIN_TOKEN);
	if(login_token != expected_token){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", expected_token, ", got ", login_token);
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}

	WorldMap::forced_reload_map_objects_by_owner(account_uuid);

	PlayerSessionMap::add(account_uuid, session);

	session->send(Msg::SC_AccountSynchronizeSystemClock({ }, utc_now));

	session->send(Msg::SC_AccountLoginSuccess(account_uuid.str()));
	AccountMap::synchronize_account_with_player_all(account_uuid, session, true, true, true, { });

	return Response();
}
// ============================================================================
	}
	MODULE_RAII(handles){
		handles.push(PlayerSession::create_servlet(Msg::CS_AccountLogin::ID, &AccountLoginServlet));
	}
}

namespace {
	struct SequentialSignInInfo {
		std::uint64_t last_signed_in_time;
		std::uint64_t last_signed_in_day;
		std::uint64_t now;
		std::uint64_t today;
		std::uint64_t sequential_days;
	};

	SequentialSignInInfo get_signed_in(const boost::shared_ptr<Account> &account){
		PROFILE_ME;

		const auto item_data = Data::Item::require(ItemIds::ID_SIGNING_IN);
		if(item_data->auto_inc_type != Data::Item::AIT_DAILY){
			LOG_EMPERY_CENTER_ERROR("Sign-in item is not daily-reset?");
			DEBUG_THROW(Exception, sslit("Sign-in item is not daily-reset"));
		}
		const auto auto_inc_offset = checked_mul<std::uint64_t>(item_data->auto_inc_offset, 60000);
		LOG_EMPERY_CENTER_DEBUG("Retrieved daily sign-in offset: auto_inc_offset = ", auto_inc_offset);

		const auto utc_now = Poseidon::get_utc_time();

		const auto last_signed_in_time = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_LAST_SIGNED_IN_TIME);
		const auto last_signed_day = saturated_sub(last_signed_in_time, auto_inc_offset) / 86400000;
		const auto today = saturated_sub(utc_now, auto_inc_offset) / 86400000;
		LOG_EMPERY_CENTER_DEBUG("Checking sign-in date: last_signed_day = ", last_signed_day, ", today = ", today);

		std::uint64_t sequential_days = 0;
		if(saturated_sub(today, last_signed_day) > 1){
			LOG_EMPERY_CENTER_DEBUG("Broken sign-in sequence: account_uuid = ", account->get_account_uuid());

			boost::container::flat_map<AccountAttributeId, std::string> modifiers;
			modifiers[AccountAttributeIds::ID_SEQUENTIAL_SIGNED_IN_DAYS] = std::string();
			account->set_attributes(modifiers);
		} else {
			sequential_days = account->cast_attribute<std::uint64_t>(AccountAttributeIds::ID_SEQUENTIAL_SIGNED_IN_DAYS);
		}

		SequentialSignInInfo info;
		info.last_signed_in_time = last_signed_in_time;
		info.last_signed_in_day  = last_signed_day;
		info.now                 = utc_now;
		info.today               = today;
		info.sequential_days     = sequential_days;
		return info;
	}
}

PLAYER_SERVLET(Msg::CS_AccountSetAttribute, account, session, req){
	const auto account_attribute_id = AccountAttributeId(req.account_attribute_id);
	auto &value = req.value;

	constexpr std::size_t MAX_ATTRIBUTE_LEN = 0xFFFF;
	if(value.size() > MAX_ATTRIBUTE_LEN){
		return Response(Msg::ERR_ATTR_TOO_LONG) <<MAX_ATTRIBUTE_LEN;
	}
	if(account_attribute_id >= AccountAttributeIds::ID_CUSTOM_END){
		return Response(Msg::ERR_ATTR_NOT_SETTABLE) <<account_attribute_id;
	}

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers[account_attribute_id] = std::move(value);
	account->set_attributes(modifiers);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetNick, account, session, req){
	auto &nick = req.nick;
	constexpr std::size_t MAX_NICK_LEN = 31;
	if(nick.size() > MAX_NICK_LEN){
		return Response(Msg::ERR_NICK_TOO_LONG) <<MAX_NICK_LEN;
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	std::vector<boost::shared_ptr<Account>> other_accounts;
	AccountMap::get_by_nick(other_accounts, nick);
	for(auto it = other_accounts.begin(); it != other_accounts.end(); ++it){
		const auto &other_account = *it;
		if(other_account != account){
			LOG_EMPERY_CENTER_DEBUG("Nick conflict: nick = ", nick, ", account_uuid = ", account->get_account_uuid(),
				", other_nick = ", other_account->get_nick(), ", other_account_uuid = ", other_account->get_account_uuid());
			return Response(Msg::ERR_NICK_CONFLICT) <<other_account->get_nick();
		}
	}

	std::vector<ItemTransactionElement> transaction;
	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_NICK_MODIFICATION_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ account->set_nick(std::move(nick)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountFindByNick, account, session, req){
	auto &nick = req.nick;

	std::vector<boost::shared_ptr<Account>> accounts;
	AccountMap::get_by_nick(accounts, nick);
	for(auto it = accounts.begin(); it != accounts.end(); ++it){
		const auto &other_account = *it;
		const auto other_account_uuid = other_account->get_account_uuid();
		const auto other_item_box = ItemBoxMap::require(other_account_uuid);

		AccountMap::synchronize_account_with_player_all(other_account_uuid, session, true, true, false, other_item_box);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountQueryAttributes, account, session, req){
	constexpr std::uint64_t FL_NICK       = 0x0001;
	constexpr std::uint64_t FL_ATTRIBUTES = 0x0002;
	constexpr std::uint64_t FL_ITEMS      = 0x0004;

	Msg::SC_AccountQueryAttributesRet msg;
	msg.accounts.reserve(req.accounts.size());
	for(auto it = req.accounts.begin(); it != req.accounts.end(); ++it){
		const auto other_account_uuid = AccountUuid(it->account_uuid);

		auto &elem = *msg.accounts.emplace(msg.accounts.end());
		elem.account_uuid = std::move(it->account_uuid);
		elem.error_code   = Msg::ERR_NO_SUCH_ACCOUNT;

		const auto &other_account = AccountMap::get(other_account_uuid);
		if(other_account){
			continue;
		}

		const bool wants_nick               = Poseidon::has_any_flags_of(it->detail_flags, FL_NICK);
		const bool wants_attributes         = Poseidon::has_any_flags_of(it->detail_flags, FL_ATTRIBUTES);
		const bool wants_private_attributes = wants_attributes && (other_account_uuid == account->get_account_uuid());

		boost::shared_ptr<ItemBox> other_item_box;
		const bool wants_items              = Poseidon::has_any_flags_of(it->detail_flags, FL_ITEMS);
		if(wants_items){
			other_item_box = ItemBoxMap::require(other_account_uuid);
		}

		AccountMap::synchronize_account_with_player_all(other_account_uuid, session,
			wants_nick, wants_attributes, wants_private_attributes, other_item_box);
		elem.error_code = Msg::ST_OK;
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSignIn, account, session, req){
	const auto account_uuid = account->get_account_uuid();

	const auto item_box = ItemBoxMap::require(account_uuid);
	item_box->pump_status();

	const auto signed_in = get_signed_in(account);
	LOG_EMPERY_CENTER_DEBUG("Retrieved signed-in info: account_uuid = ", account_uuid,
		", last_signed_in_time = ", signed_in.last_signed_in_time, ", last_signed_in_day = ", signed_in.last_signed_in_day,
		", today = ", signed_in.today, ", sequential_days = ", signed_in.sequential_days);

	const auto max_sequential_days = Data::SigningIn::get_max_sequential_days();
	if(max_sequential_days == 0){
		LOG_EMPERY_CENTER_ERROR("The number of max sequential sign-in days is set to zero!");
		DEBUG_THROW(Exception, sslit("Number of max sequential sign-in days is zero"));
	}
	const unsigned reward_key = signed_in.sequential_days % max_sequential_days + 1;
	const auto signing_in_data = Data::SigningIn::get(reward_key);
	if(!signing_in_data){
		return Response(Msg::ERR_NO_SUCH_SIGN_IN_REWARD) <<reward_key;
	}
	const auto trade_data = Data::ItemTrade::require(signing_in_data->trade_id);

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers.reserve(8);
	modifiers[AccountAttributeIds::ID_LAST_SIGNED_IN_TIME]       = boost::lexical_cast<std::string>(signed_in.now);
	modifiers[AccountAttributeIds::ID_SEQUENTIAL_SIGNED_IN_DAYS] = boost::lexical_cast<std::string>(signed_in.sequential_days + 1);

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, 1, decltype(req)::ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, false,
		[&]{ account->set_attributes(std::move(modifiers)); });
	if(insuff_item_id){
		LOG_EMPERY_CENTER_DEBUG("Insufficient item: insuff_item_id = ", insuff_item_id);
		return Response(Msg::ERR_ALREADY_SIGNED_IN);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSynchronizeSystemClock, account, session, req){
	const auto utc_now = Poseidon::get_utc_time();

	session->send(Msg::SC_AccountSynchronizeSystemClock(std::move(req.context), utc_now));

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountGetWarStatus, account, session, /* req */){
	WarStatusMap::synchronize_with_player_by_account(session, account->get_account_uuid());

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetAvatar, account, session, req){
	auto &avatar = req.avatar;
	constexpr std::size_t MAX_AVATAR_LEN = 255;
	if(avatar.size() > MAX_AVATAR_LEN){
		return Response(Msg::ERR_ATTR_TOO_LONG) <<MAX_AVATAR_LEN;
	}

	const auto item_box = ItemBoxMap::require(account->get_account_uuid());

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers[AccountAttributeIds::ID_AVATAR] = std::move(avatar);

	std::vector<ItemTransactionElement> transaction;
	const auto trade_id = TradeId(Data::Global::as_unsigned(Data::Global::SLOT_NICK_MODIFICATION_TRADE_ID));
	const auto trade_data = Data::ItemTrade::require(trade_id);
	Data::unpack_item_trade(transaction, trade_data, 1, req.ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction, true,
		[&]{ account->set_attributes(std::move(modifiers)); });
	if(insuff_item_id){
		return Response(Msg::ERR_NO_ENOUGH_ITEMS) <<insuff_item_id;
	}

	return Response();
}

}
