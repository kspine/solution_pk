#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/err_account.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "../data/signing_in.hpp"
#include "../item_ids.hpp"
#include "../data/item.hpp"
#include "../singletons/item_box_map.hpp"
#include "../item_box.hpp"
#include "../transaction_element.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyCenter {

namespace {
	struct SequentialSignInInfo {
		boost::uint64_t last_signed_in_time;
		boost::uint64_t last_signed_in_day;
		boost::uint64_t now;
		boost::uint64_t today;
		boost::uint64_t sequential_days;
	};

	SequentialSignInInfo get_signed_in(const boost::shared_ptr<Account> &account){
		PROFILE_ME;

		const auto item_data = Data::Item::require(ItemIds::ID_SIGNING_IN);
		if(item_data->auto_inc_type != Data::Item::AIT_DAILY){
			LOG_EMPERY_CENTER_ERROR("Sign-in item is not daily-reset?");
			DEBUG_THROW(Exception, sslit("Sign-in item is not daily-reset"));
		}
		const auto auto_inc_offset = item_data->auto_inc_offset;
		LOG_EMPERY_CENTER_DEBUG("Retrieved daily sign-in offset: auto_inc_offset = ", auto_inc_offset);

		const auto utc_now = Poseidon::get_utc_time();

		const auto last_signed_in_time = account->cast_attribute<boost::uint64_t>(AccountAttributeIds::ID_LAST_SIGNED_IN_TIME);
		const auto last_signed_day = saturated_sub(last_signed_in_time, auto_inc_offset) / 86400000;
		const auto today = saturated_sub(utc_now, auto_inc_offset) / 86400000;
		LOG_EMPERY_CENTER_DEBUG("Checking sign-in date: last_signed_day = ", last_signed_day, ", today = ", today);

		boost::uint64_t sequential_days = 0;
		if(saturated_sub(today, last_signed_day) > 1){
			LOG_EMPERY_CENTER_DEBUG("Broken sign-in sequence: account_uuid = ", account->get_account_uuid());

			boost::container::flat_map<AccountAttributeId, std::string> modifiers;
			modifiers[AccountAttributeIds::ID_SEQUENTIAL_SIGNED_IN_DAYS] = { };
			account->set_attributes(modifiers);
		} else {
			sequential_days = account->cast_attribute<boost::uint64_t>(AccountAttributeIds::ID_SEQUENTIAL_SIGNED_IN_DAYS);
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

PLAYER_SERVLET_RAW(Msg::CS_AccountLogin, session, req){
	const auto platform_id  = PlatformId(req.platform_id);
	const auto &login_name  = req.login_name;
	const auto &login_token = req.login_token;
	LOG_EMPERY_CENTER_INFO("Account login: platform_id = ", platform_id, ", login_name = ", login_name, ", login_token = ", login_token);

	const auto old_account_uuid = PlayerSessionMap::get_account_uuid(session);
	if(old_account_uuid){
		return Response(Msg::ERR_MULTIPLE_LOGIN) <<old_account_uuid;
	}

	const auto account = AccountMap::get_by_login_name(platform_id, login_name);
	if(!account){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now >= account->get_login_token_expiry_time()){
		return Response(Msg::ERR_TOKEN_EXPIRED) <<login_name;
	}
	if(login_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	if(login_token != account->get_login_token()){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", account->get_login_token(), ", got ", login_token);
		return Response(Msg::ERR_INVALID_TOKEN) <<login_name;
	}
	if(utc_now < account->get_banned_until()){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<login_name;
	}
	const auto account_uuid = account->get_account_uuid();

	get_signed_in(account);

	PlayerSessionMap::add(account, session);
	session->send(Msg::SC_AccountLoginSuccess(account_uuid.str()));
	AccountMap::synchronize_account_with_player(account_uuid, session, true, true, true, false);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetAttribute, account_uuid, session, req){
	const auto account_attribute_id = AccountAttributeId(req.account_attribute_id);
	auto &value = req.value;

	if(account_attribute_id >= AccountAttributeIds::ID_CUSTOM_END){
		return Response(Msg::ERR_ATTR_NOT_SETTABLE) <<account_attribute_id;
	}
	if(value.size() > Account::MAX_ATTRIBUTE_LEN){
		return Response(Msg::ERR_ATTR_TOO_LONG) <<Account::MAX_ATTRIBUTE_LEN;
	}

	const auto account = AccountMap::require(account_uuid);

	boost::container::flat_map<AccountAttributeId, std::string> modifiers;
	modifiers[account_attribute_id] = std::move(value);
	account->set_attributes(modifiers);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetNick, account_uuid, session, req){
	auto &nick = req.nick;

	if(nick.size() > Account::MAX_NICK_LEN){
		return Response(Msg::ERR_NICK_TOO_LONG) <<Account::MAX_NICK_LEN;
	}

	const auto account = AccountMap::require(account_uuid);

	account->set_nick(std::move(nick));

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountFindByNick, account_uuid, session, req){
	auto &nick = req.nick;

	std::vector<boost::shared_ptr<Account>> accounts;
	AccountMap::get_by_nick(accounts, nick);
	for(auto it = accounts.begin(); it != accounts.end(); ++it){
		const auto &other_account = *it;
		const auto other_account_uuid = other_account->get_account_uuid();

		AccountMap::synchronize_account_with_player(other_account_uuid, session, true, true, false, true);
	}

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountQueryAttributes, account_uuid, session, req){
	enum {
		FL_NICK           = 0x0001,
		FL_ATTRIBUTES     = 0x0002,
		FL_ITEMS          = 0x0004,
	};

	Msg::SC_AccountQueryAttributesRet msg;
	msg.accounts.reserve(req.accounts.size());
	for(auto it = req.accounts.begin(); it != req.accounts.end(); ++it){
		const auto other_account_uuid = AccountUuid(it->account_uuid);

		msg.accounts.emplace_back();
		auto &account = msg.accounts.back();
		account.account_uuid = std::move(it->account_uuid);
		account.error_code = Msg::ERR_NO_SUCH_ACCOUNT;

		const auto &other_account = AccountMap::get(other_account_uuid);
		if(other_account){
			continue;
		}
		const bool wants_nick               = Poseidon::has_any_flags_of(it->detail_flags, FL_NICK);
		const bool wants_attributes         = Poseidon::has_any_flags_of(it->detail_flags, FL_ATTRIBUTES);
		const bool wants_private_attributes = wants_attributes && (other_account_uuid == account_uuid);
		const bool wants_items              = Poseidon::has_any_flags_of(it->detail_flags, FL_ITEMS);
		AccountMap::synchronize_account_with_player(other_account_uuid, session,
			wants_nick, wants_attributes, wants_private_attributes, wants_items);
		account.error_code = Msg::ST_OK;
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSignIn, account_uuid, session, req){
	const auto item_box = ItemBoxMap::require(account_uuid);
	item_box->pump_status();

	const auto account = AccountMap::require(account_uuid);

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
	modifiers.reserve(2);
	modifiers[AccountAttributeIds::ID_LAST_SIGNED_IN_TIME]       = boost::lexical_cast<std::string>(signed_in.now);
	modifiers[AccountAttributeIds::ID_SEQUENTIAL_SIGNED_IN_DAYS] = boost::lexical_cast<std::string>(signed_in.sequential_days + 1);

	std::vector<ItemTransactionElement> transaction;
	Data::unpack_item_trade(transaction, trade_data, 1, decltype(req)::ID);
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction,
		[&]{ account->set_attributes(std::move(modifiers)); });
	if(insuff_item_id){
		LOG_EMPERY_CENTER_DEBUG("Insufficient item: account_uuid = ", account_uuid, ", insuff_item_id = ", insuff_item_id);
		return Response(Msg::ERR_ALREADY_SIGNED_IN);
	}

	return Response();
}

PLAYER_SERVLET_RAW(Msg::CS_AccountSynchronizeSystemClock, session, req){
	session->send(Msg::SC_AccountSynchronizeSystemClock(std::move(req.context), Poseidon::get_utc_time()));

	return Response();
}

}
