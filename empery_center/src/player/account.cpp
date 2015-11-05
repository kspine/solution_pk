#include "../precompiled.hpp"
#include "common.hpp"
#include <poseidon/singletons/event_dispatcher.hpp>
#include "../msg/cs_account.hpp"
#include "../msg/sc_account.hpp"
#include "../msg/cerr_account.hpp"

namespace EmperyCenter {

namespace {
	void sendAccountInfoToClient(const AccountUuid &accountUuid, const boost::shared_ptr<PlayerSession> &session,
		bool wantsNick, bool wantsAttributes, bool wantsPrivateAttributes, bool wantsItems)
	{
		PROFILE_ME;

		Msg::SC_AccountAttributes msg;
		msg.accountUuid = accountUuid.str();
		if(wantsNick){
			auto info = AccountMap::require(accountUuid);
			msg.nick = std::move(info.nick);
		}
		if(wantsAttributes){
			unsigned slotEnd = AccountMap::ATTR_PUBLIC_END;
			if(wantsPrivateAttributes){
				slotEnd = AccountMap::ATTR_END;
			}
			boost::container::flat_map<unsigned, std::string> attributes;
			AccountMap::getAttributes(attributes, accountUuid, 0, slotEnd);
			msg.attributes.reserve(attributes.size());
			for(auto it = attributes.begin(); it != attributes.end(); ++it){
				msg.attributes.emplace_back();
				auto &attribute = msg.attributes.back();
				attribute.slot = it->first;
				attribute.value = std::move(it->second);
			}
		}
		if(wantsItems){
			// TODO 添加公开资源。
		}
		session->send(msg);
	}
}

PLAYER_SERVLET_RAW(Msg::CS_AccountLogin, session, req){
	LOG_EMPERY_CENTER_INFO("Account login: platformId = ", req.platformId, ", loginName = ", req.loginName);

	const auto oldAccountUuid = PlayerSessionMap::getAccountUuid(session);
	if(oldAccountUuid){
		return Response(Msg::CERR_MULTIPLE_LOGIN) <<oldAccountUuid;
	}

	const auto loginInfo = AccountMap::getLoginInfo(PlatformId(req.platformId), req.loginName);
	if(Poseidon::hasNoneFlagsOf(loginInfo.flags, AccountMap::FL_VALID)){
		return Response(Msg::CERR_NO_SUCH_ACCOUNT) <<req.loginName;
	}
	const auto localNow = Poseidon::getUtcTime();
	if(localNow >= loginInfo.loginTokenExpiryTime){
		return Response(Msg::CERR_TOKEN_EXPIRED) <<req.loginName;
	}
	if(req.loginToken.empty()){
		LOG_EMPERY_CENTER_DEBUG("Empty token");
		return Response(Msg::CERR_INVALID_TOKEN) <<req.loginName;
	}
	if(req.loginToken != loginInfo.loginToken){
		LOG_EMPERY_CENTER_DEBUG("Invalid token: expecting ", loginInfo.loginToken, ", got ", req.loginToken);
		return Response(Msg::CERR_INVALID_TOKEN) <<req.loginName;
	}
	if(localNow < loginInfo.bannedUntil){
		return Response(Msg::CERR_ACCOUNT_BANNED) <<req.loginName;
	}

	const auto accountUuid = loginInfo.accountUuid;

	PlayerSessionMap::add(accountUuid, session);
	session->send(Msg::SC_AccountLoginSuccess(accountUuid.str()));
	sendAccountInfoToClient(accountUuid, session, true, true, true, true);
	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetAttribute, accountUuid, session, req){
	if(req.slot >= AccountMap::ATTR_CUSTOM_END){
		return Response(Msg::CERR_ATTR_NOT_SETTABLE) <<req.slot;
	}
	if(req.value.size() > AccountMap::MAX_ATTRIBUTE_LEN){
		return Response(Msg::CERR_ATTR_TOO_LONG) <<AccountMap::MAX_ATTRIBUTE_LEN;
	}

	AccountMap::setAttribute(accountUuid, req.slot, std::move(req.value));
	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountSetNick, accountUuid, session, req){
	if(req.nick.size() > AccountMap::MAX_NICK_LEN){
		return Response(Msg::CERR_NICK_TOO_LONG) <<AccountMap::MAX_NICK_LEN;
	}

	AccountMap::setNick(accountUuid, std::move(req.nick));
	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountFindByNick, accountUuid, session, req){
	std::vector<AccountMap::AccountInfo> infos;
	AccountMap::getByNick(infos, req.nick);
	for(auto it = infos.begin(); it != infos.end(); ++it){
		const auto otherUuid = AccountUuid(it->accountUuid);

		sendAccountInfoToClient(otherUuid, session, true, true, otherUuid == accountUuid, true);
	}
	return Response();
}

PLAYER_SERVLET(Msg::CS_AccountQueryAttributes, accountUuid, session, req){
	enum {
		FL_NICK           = 0x0001,
		FL_ATTRIBUTES     = 0x0002,
		FL_ITEMS          = 0x0004,
	};

	Msg::SC_AccountQueryAttributesRet msg;
	msg.accounts.reserve(req.accounts.size());
	for(auto it = req.accounts.begin(); it != req.accounts.end(); ++it){
		const auto otherUuid = AccountUuid(it->accountUuid);

		msg.accounts.emplace_back();
		auto &account = msg.accounts.back();
		account.accountUuid = std::move(it->accountUuid);
		account.errorCode = Msg::CERR_NO_SUCH_ACCOUNT;
		if(!AccountMap::has(otherUuid)){
			continue;
		}
		sendAccountInfoToClient(otherUuid, session,
			it->detailFlags & FL_NICK, it->detailFlags & FL_ATTRIBUTES, otherUuid == accountUuid, it->detailFlags & FL_ITEMS);
		account.errorCode = Msg::ST_OK;
	}
	session->send(msg);
	return Response();
}

}
