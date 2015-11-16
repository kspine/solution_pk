#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/cerr_account.hpp"

namespace EmperyCenter {

PLAYER_SERVLET_RAW(Msg::CS_AccountLogin, session, req){
	LOG_EMPERY_CENTER_INFO("Account login: platform_id = ", req.platform_id, ", login_name = ", req.login_name);

	const auto old_account_uuid = PlayerSessionMap::get_account_uuid(session);
	if(old_account_uuid){
		return Response(Msg::CERR_MULTIPLE_LOGIN) <<old_account_uuid;
	}

	const auto login_info = AccountMap::get_login_info(PlatformId(req.platform_id), req.login_name);
	if(Poseidon::has_none_flags_of(login_info.flags, AccountMap::FL_VALID)){
		return Response(Msg::CERR_NO_SUCH_ACCOUNT) <<req.login_name;
	}
	const auto local_now = Poseidon::get_utc_time();
	if(local_now >= login_info.login_token_expiry_time){
		return Response(Msg::CERR_TOKEN_EXPIRED) <<req.login_name;
	}
	if(req.login_token.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::CERR_INVALID_TOKEN) <<req.login_name;
	}
	if(req.login_token != login_info.login_token){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", login_info.login_token, ", got ", req.login_token);
		return Response(Msg::CERR_INVALID_TOKEN) <<req.login_name;
	}
	if(local_now < login_info.banned_until){
		return Response(Msg::CERR_ACCOUNT_BANNED) <<req.login_name;
	}

	PlayerSessionMap::add(login_info.account_uuid, session);
	session->send(Msg::SC_AccountLoginSuccess(login_info.account_uuid.str()));
	AccountMap::send_attributes_to_client(login_info.account_uuid, session, true, true, true, false);

	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetAttribute, account_uuid, session, req){
	if(req.slot >= AccountMap::ATTR_CUSTOM_END){
		return Response(Msg::CERR_ATTR_NOT_SETTABLE) <<req.slot;
	}
	if(req.value.size() > AccountMap::MAX_ATTRIBUTE_LEN){
		return Response(Msg::CERR_ATTR_TOO_LONG) <<AccountMap::MAX_ATTRIBUTE_LEN;
	}

	AccountMap::set_attribute(account_uuid, req.slot, std::move(req.value));
	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetNick, account_uuid, session, req){
	if(req.nick.size() > AccountMap::MAX_NICK_LEN){
		return Response(Msg::CERR_NICK_TOO_LONG) <<AccountMap::MAX_NICK_LEN;
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
		account.error_code = Msg::CERR_NO_SUCH_ACCOUNT;
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

}
