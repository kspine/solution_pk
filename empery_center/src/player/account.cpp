#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/err_account.hpp"
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

	SequentialSignInInfo get_signed_in_info(AccountUuid account_uuid){
		PROFILE_ME;

		const auto item_data = Data::Item::require(ItemIds::ID_SIGNING_IN);
		if(item_data->auto_inc_type != Data::Item::AIT_DAILY){
			LOG_EMPERY_CENTER_ERROR("Sign-in item is not daily-reset?");
			DEBUG_THROW(Exception, sslit("Sign-in item is not daily-reset"));
		}
		const auto auto_inc_offset = item_data->auto_inc_offset;
		LOG_EMPERY_CENTER_DEBUG("Retrieved daily sign-in offset: account_uuid = ", account_uuid, ", auto_inc_offset = ", auto_inc_offset);

		const auto last_signed_in_time = AccountMap::get_attribute_gen<boost::uint64_t>(account_uuid, AccountMap::ATTR_LAST_SIGNED_IN_TIME);
		const auto utc_now = Poseidon::get_utc_time();

		const auto last_signed_day = saturated_sub(last_signed_in_time, auto_inc_offset) / 86400000;
		const auto today = saturated_sub(utc_now, auto_inc_offset) / 86400000;
		LOG_EMPERY_CENTER_DEBUG("Checking sign-in date: last_signed_day = ", last_signed_day, ", today = ", today);

		boost::uint64_t sequential_days = 0;
		if(saturated_sub(today, last_signed_day) > 1){
			LOG_EMPERY_CENTER_DEBUG("Broken sign-in sequence: account_uuid = ", account_uuid);
			AccountMap::set_attribute(account_uuid, AccountMap::ATTR_SEQUENTIAL_SIGNED_IN_DAYS, std::string());
		} else {
			sequential_days = AccountMap::get_attribute_gen<boost::uint64_t>(account_uuid, AccountMap::ATTR_SEQUENTIAL_SIGNED_IN_DAYS);
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
	LOG_EMPERY_CENTER_INFO("Account login: platform_id = ", req.platform_id, ", login_name = ", req.login_name);

	const auto old_account_uuid = PlayerSessionMap::get_account_uuid(session);
	if(old_account_uuid){
		return Response(Msg::ERR_MULTIPLE_LOGIN) <<old_account_uuid;
	}

	const auto login_info = AccountMap::get_login_info(PlatformId(req.platform_id), req.login_name);
	if(Poseidon::has_none_flags_of(login_info.flags, AccountMap::FL_VALID)){
		return Response(Msg::ERR_NO_SUCH_ACCOUNT) <<req.login_name;
	}
	const auto utc_now = Poseidon::get_utc_time();
	if(utc_now >= login_info.login_token_expiry_time){
		return Response(Msg::ERR_TOKEN_EXPIRED) <<req.login_name;
	}
	if(req.login_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::ERR_INVALID_TOKEN) <<req.login_name;
	}
	if(req.login_token != login_info.login_token){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", login_info.login_token, ", got ", req.login_token);
		return Response(Msg::ERR_INVALID_TOKEN) <<req.login_name;
	}
	if(utc_now < login_info.banned_until){
		return Response(Msg::ERR_ACCOUNT_BANNED) <<req.login_name;
	}
	const auto account_uuid = login_info.account_uuid;

	get_signed_in_info(account_uuid);

	PlayerSessionMap::add(account_uuid, session);
	session->send(Msg::SC_AccountLoginSuccess(account_uuid.str()));
	AccountMap::send_attributes_to_client(account_uuid, session, true, true, true, false);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetAttribute, account_uuid, session, req){
	if(req.slot >= AccountMap::ATTR_CUSTOM_END){
		return Response(Msg::ERR_ATTR_NOT_SETTABLE) <<req.slot;
	}
	if(req.value.size() > AccountMap::MAX_ATTRIBUTE_LEN){
		return Response(Msg::ERR_ATTR_TOO_LONG) <<AccountMap::MAX_ATTRIBUTE_LEN;
	}

	AccountMap::set_attribute(account_uuid, req.slot, std::move(req.value));

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetNick, account_uuid, session, req){
	if(req.nick.size() > AccountMap::MAX_NICK_LEN){
		return Response(Msg::ERR_NICK_TOO_LONG) <<AccountMap::MAX_NICK_LEN;
	}

	AccountMap::set_nick(account_uuid, std::move(req.nick));

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountFindByNick, account_uuid, session, req){
	std::vector<AccountMap::AccountInfo> infos;
	AccountMap::get_by_nick(infos, req.nick);
	for(auto it = infos.begin(); it != infos.end(); ++it){
		const auto other_uuid = AccountUuid(it->account_uuid);

		AccountMap::send_attributes_to_client(other_uuid, session, true, true, other_uuid == account_uuid, true);
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
		const auto other_uuid = AccountUuid(it->account_uuid);

		msg.accounts.emplace_back();
		auto &account = msg.accounts.back();
		account.account_uuid = std::move(it->account_uuid);
		account.error_code = Msg::ERR_NO_SUCH_ACCOUNT;
		if(!AccountMap::has(other_uuid)){
			continue;
		}
		AccountMap::send_attributes_to_client(other_uuid, session,
			it->detail_flags & FL_NICK, it->detail_flags & FL_ATTRIBUTES, other_uuid == account_uuid, it->detail_flags & FL_ITEMS);
		account.error_code = Msg::ST_OK;
	}
	session->send(msg);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSignIn, account_uuid, session, req){
	const auto item_box = ItemBoxMap::require(account_uuid);
	item_box->pump_status();

	const auto signed_in_info = get_signed_in_info(account_uuid);
	LOG_EMPERY_CENTER_DEBUG("Retrieved signed-in info: account_uuid = ", account_uuid,
		", last_signed_in_time = ", signed_in_info.last_signed_in_time, ", last_signed_in_day = ", signed_in_info.last_signed_in_day,
		", today = ", signed_in_info.today, ", sequential_days = ", signed_in_info.sequential_days);

	const auto max_sequential_days = Data::SigningIn::get_max_sequential_days();
	if(max_sequential_days == 0){
		LOG_EMPERY_CENTER_ERROR("The number of max sequential sign-in days is set to zero!");
		DEBUG_THROW(Exception, sslit("Number of max sequential sign-in days is zero"));
	}
	const unsigned reward_key = signed_in_info.sequential_days % max_sequential_days + 1;
	const auto signing_in_data = Data::SigningIn::get(reward_key);
	if(!signing_in_data){
		return Response(Msg::ERR_NO_SUCH_SIGN_IN_REWARD) <<reward_key;
	}
	const auto trade_data = Data::ItemTrade::require(signing_in_data->trade_id);

	std::vector<ItemTransactionElement> transaction;
	Data::ItemTrade::unpack(transaction, trade_data, 1, decltype(req)::ID); // XXX Stupid GCC warning.
	const auto insuff_item_id = item_box->commit_transaction_nothrow(transaction.data(), transaction.size(),
		[&]{
			AccountMap::touch_attribute(account_uuid, AccountMap::ATTR_LAST_SIGNED_IN_TIME);
			AccountMap::touch_attribute(account_uuid, AccountMap::ATTR_SEQUENTIAL_SIGNED_IN_DAYS);
			auto time_str = boost::lexical_cast<std::string>(signed_in_info.now);
			auto days_str = boost::lexical_cast<std::string>(signed_in_info.sequential_days + 1);
			AccountMap::set_attribute(account_uuid, AccountMap::ATTR_LAST_SIGNED_IN_TIME, std::move(time_str));
			AccountMap::set_attribute(account_uuid, AccountMap::ATTR_SEQUENTIAL_SIGNED_IN_DAYS, std::move(days_str));
		});
	if(insuff_item_id){
		LOG_EMPERY_CENTER_DEBUG("Insufficient item: account_uuid = ", account_uuid, ", insuff_item_id = ", insuff_item_id);
		return Response(Msg::ERR_ALREADY_SIGNED_IN);
	}

	return Response();
}

}
