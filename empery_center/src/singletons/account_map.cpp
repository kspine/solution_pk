#include "../precompiled.hpp"
#include "account_map.hpp"
#include <string.h>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include "player_session_map.hpp"
#include "../mysql/account.hpp"
#include "../mysql/account_attribute.hpp"
#include "../msg/sc_account.hpp"
#include "../player_session.hpp"
#include "../events/account.hpp"

namespace EmperyCenter {

namespace {
	struct StringCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};

	struct LoginKey {
		PlatformId platformId;
		const std::string *loginName;

		LoginKey(PlatformId platformId_, const std::string &loginName_)
			: platformId(platformId_), loginName(&loginName_)
		{
		}

		bool operator<(const LoginKey &rhs) const {
			if(platformId != rhs.platformId){
				return platformId < rhs.platformId;
			}
			return StringCaseComparator()(*loginName, *rhs.loginName);
		}
	};

	struct AccountElement {
		boost::shared_ptr<MySql::Center_Account> obj;

		AccountUuid accountUuid;
		std::string nick;
		LoginKey loginKey;

		explicit AccountElement(boost::shared_ptr<MySql::Center_Account> obj_)
			: obj(std::move(obj_))
			, accountUuid(obj->unlockedGet_accountUuid()), nick(obj->unlockedGet_nick())
			, loginKey(PlatformId(obj->get_platformId()), obj->unlockedGet_loginName())
		{
		}
	};

	MULTI_INDEX_MAP(AccountMapDelegator, AccountElement,
		UNIQUE_MEMBER_INDEX(accountUuid)
		MULTI_MEMBER_INDEX(nick, StringCaseComparator)
		UNIQUE_MEMBER_INDEX(loginKey)
	)

	AccountMapDelegator g_accountMap;

	struct AccountAttributeElement {
		boost::shared_ptr<MySql::Center_AccountAttribute> obj;

		std::pair<AccountUuid, unsigned> accountSlot;

		explicit AccountAttributeElement(boost::shared_ptr<MySql::Center_AccountAttribute> obj_)
			: obj(std::move(obj_))
			, accountSlot(std::make_pair(AccountUuid(obj->get_accountUuid()), obj->get_slot()))
		{
		}
	};

	MULTI_INDEX_MAP(AccountAttributeMapDelegator, AccountAttributeElement,
		UNIQUE_MEMBER_INDEX(accountSlot)
	)

	AccountAttributeMapDelegator g_attributeMap;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		AccountMapDelegator accountMap;
		LOG_EMPERY_CENTER_INFO("Loading accounts...");
		conn->executeSql("SELECT * FROM `Center_Account`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Center_Account>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			accountMap.insert(AccountElement(std::move(obj)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", accountMap.size(), " account(s).");
		g_accountMap.swap(accountMap);

		AccountAttributeMapDelegator attributeMap;
		LOG_EMPERY_CENTER_INFO("Loading account attributes...");
		conn->executeSql("SELECT * FROM `Center_AccountAttribute`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Center_AccountAttribute>();
			obj->syncFetch(conn);
			const auto it = g_accountMap.find<0>(AccountUuid(obj->unlockedGet_accountUuid()));
			if(it == g_accountMap.end<0>()){
				LOG_EMPERY_CENTER_ERROR("No such account: accountUuid = ", obj->unlockedGet_accountUuid());
				continue;
			}
			obj->enableAutoSaving();
			attributeMap.insert(AccountAttributeElement(std::move(obj)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", attributeMap.size(), " account attribute(s).");
		g_attributeMap.swap(attributeMap);

		auto listener = Poseidon::EventDispatcher::registerListener<Events::AccountSetToken>(
			[](const boost::shared_ptr<Events::AccountSetToken> &event){
				PROFILE_ME;
				LOG_EMPERY_CENTER_INFO("Set token: platformId = ", event->platformId, ", loginName = ", event->loginName,
					", loginToken = ", event->loginToken, ", expiryTime = ", event->expiryTime);
				const auto accountUuid = AccountMap::create(event->platformId, event->loginName, event->loginName, 0).first;
				AccountMap::setLoginToken(accountUuid, event->loginToken, event->expiryTime);
			});
		LOG_EMPERY_CENTER_DEBUG("Created AccountSetToken listener");
		handles.push(listener);
	}

	void fillAccountInfo(AccountMap::AccountInfo &info, const boost::shared_ptr<MySql::Center_Account> &obj){
		PROFILE_ME;

		info.accountUuid = AccountUuid(obj->unlockedGet_accountUuid());
		info.loginName   = obj->unlockedGet_loginName();
		info.nick        = obj->unlockedGet_nick();
		info.flags       = obj->get_flags();
		info.createdTime = obj->get_createdTime();
	}
	void fillLoginInfo(AccountMap::LoginInfo &info, const boost::shared_ptr<MySql::Center_Account> &obj){
		PROFILE_ME;

		info.platformId           = PlatformId(obj->get_platformId());
		info.loginName            = obj->unlockedGet_loginName();
		info.accountUuid          = AccountUuid(obj->unlockedGet_accountUuid());
		info.flags                = obj->get_flags();
		info.loginToken           = obj->unlockedGet_loginToken();
		info.loginTokenExpiryTime = obj->get_loginTokenExpiryTime();
		info.bannedUntil          = obj->get_bannedUntil();
	}
}

bool AccountMap::has(const AccountUuid &accountUuid){
	PROFILE_ME;

	const auto it = g_accountMap.find<0>(accountUuid);
	if(it == g_accountMap.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: accountUuid = ", accountUuid);
		return false;
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: accountUuid = ", accountUuid);
		return false;
	}
	return true;
}
AccountMap::AccountInfo AccountMap::get(const AccountUuid &accountUuid){
	PROFILE_ME;

	AccountInfo info = { };
	info.accountUuid = accountUuid;

	const auto it = g_accountMap.find<0>(accountUuid);
	if(it == g_accountMap.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: accountUuid = ", accountUuid);
		return info;
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: accountUuid = ", accountUuid);
		return info;
	}
	fillAccountInfo(info, it->obj);
	return info;
}
AccountMap::AccountInfo AccountMap::require(const AccountUuid &accountUuid){
	PROFILE_ME;

	auto info = get(accountUuid);
	if(Poseidon::hasNoneFlagsOf(info.flags, FL_VALID)){
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	return info;
}
void AccountMap::getByNick(std::vector<AccountMap::AccountInfo> &ret, const std::string &nick){
	PROFILE_ME;

	const auto range = g_accountMap.equalRange<1>(nick);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
			continue;
		}
		AccountInfo info;
		fillAccountInfo(info, it->obj);
		ret.push_back(std::move(info));
	}
}

AccountMap::LoginInfo AccountMap::getLoginInfo(const AccountUuid &accountUuid){
	PROFILE_ME;

	LoginInfo info = { };
	info.accountUuid = accountUuid;

	const auto it = g_accountMap.find<0>(accountUuid);
	if(it == g_accountMap.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: accountUuid = ", accountUuid);
		return info;
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: accountUuid = ", accountUuid);
		return info;
	}
	fillLoginInfo(info, it->obj);
	return info;
}
AccountMap::LoginInfo AccountMap::getLoginInfo(PlatformId platformId, const std::string &loginName){
	PROFILE_ME;

	LoginInfo info = { };
	info.platformId = platformId;
	info.loginName = loginName;

	const auto it = g_accountMap.find<2>(LoginKey(platformId, loginName));
	if(it == g_accountMap.end<2>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: platformId = ", platformId, ", loginName = ", loginName);
		return info;
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: platformId = ", platformId, ", loginName = ", loginName);
		return info;
	}
	fillLoginInfo(info, it->obj);
	return info;
}

std::pair<AccountUuid, bool> AccountMap::create(PlatformId platformId, std::string loginName, std::string nick, boost::uint64_t flags){
	PROFILE_ME;

	auto it = g_accountMap.find<2>(LoginKey(platformId, loginName));
	if(it != g_accountMap.end<2>()){
		return std::make_pair(it->accountUuid, false);
	}

	const auto accountUuid = AccountUuid(Poseidon::Uuid::random());
	Poseidon::addFlags(flags, AccountMap::FL_VALID);
	const auto localNow = Poseidon::getLocalTime();
	auto obj = boost::make_shared<MySql::Center_Account>(
		accountUuid.get(), platformId.get(), std::move(loginName), std::move(nick), flags, std::string(), 0, 0, localNow);
	obj->asyncSave(true);
	it = g_accountMap.insert<2>(it, AccountElement(std::move(obj)));
	return std::make_pair(accountUuid, true);
}

void AccountMap::setNick(const AccountUuid &accountUuid, std::string nick){
	PROFILE_ME;

	const auto it = g_accountMap.find<0>(accountUuid);
	if(it == g_accountMap.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_accountMap.setKey<0, 1>(it, nick); // throws std::bad_alloc
	it->obj->set_nick(std::move(nick)); // noexcept

	const auto session = PlayerSessionMap::get(accountUuid);
	if(session){
		try {
			Msg::SC_AccountAttributes msg;
			msg.accountUuid = accountUuid.str();
			msg.nick = it->obj->unlockedGet_nick(); // 不要用 nick，因为已经 move。
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->forceShutdown();
		}
	}
}
void AccountMap::setLoginToken(const AccountUuid &accountUuid, std::string loginToken, boost::uint64_t expiryTime){
	PROFILE_ME;

	const auto it = g_accountMap.find<0>(accountUuid);
	if(it == g_accountMap.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_loginToken(std::move(loginToken)); // noexcept
	it->obj->set_loginTokenExpiryTime(expiryTime); // noexcept
}
void AccountMap::setBannedUntil(const AccountUuid &accountUuid, boost::uint64_t until){
	PROFILE_ME;

	const auto it = g_accountMap.find<0>(accountUuid);
	if(it == g_accountMap.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_bannedUntil(until); // noexcept
}
void AccountMap::setFlags(const AccountUuid &accountUuid, boost::uint64_t flags){
	PROFILE_ME;

	const auto it = g_accountMap.find<0>(accountUuid);
	if(it == g_accountMap.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: accountUuid = ", accountUuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
//	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
//		LOG_EMPERY_CENTER_DEBUG("Account deleted: accountUuid = ", accountUuid);
//		DEBUG_THROW(Exception, sslit("Account deleted"));
//	}

	it->obj->set_flags(flags); // noexcept
}

const std::string &AccountMap::getAttribute(const AccountUuid &accountUuid, unsigned slot){
	PROFILE_ME;

	const auto it = g_attributeMap.find<0>(std::make_pair(accountUuid, slot));
	if(it == g_attributeMap.end<0>()){
		LOG_EMPERY_CENTER_TRACE("Account attribute not found: accountUuid = ", accountUuid, ", slot = ", slot);
		return Poseidon::EMPTY_STRING;
	}
	return it->obj->unlockedGet_value();
}
void AccountMap::getAttributes(boost::container::flat_map<unsigned, std::string> &ret,
	const AccountUuid &accountUuid, unsigned beginSlot, unsigned endSlot)
{
	PROFILE_ME;

	const auto begin = g_attributeMap.lowerBound<0>(std::make_pair(accountUuid, beginSlot));
	const auto end   = g_attributeMap.upperBound<0>(std::make_pair(accountUuid, endSlot));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
	for(auto it = begin; it != end; ++it){
		ret.emplace(it->obj->get_slot(), it->obj->unlockedGet_value());
	}
}
void AccountMap::touchAttribute(const AccountUuid &accountUuid, unsigned slot){
	PROFILE_ME;

	auto it = g_attributeMap.find<0>(std::make_pair(accountUuid, slot));
	if(it == g_attributeMap.end<0>()){
		it = g_attributeMap.insert<0>(it, AccountAttributeElement(
			boost::make_shared<MySql::Center_AccountAttribute>(accountUuid.get(), slot, std::string())));
		it->obj->asyncSave(true);
	}
}
void AccountMap::setAttribute(const AccountUuid &accountUuid, unsigned slot, std::string value){
	PROFILE_ME;

	auto it = g_attributeMap.find<0>(std::make_pair(accountUuid, slot));
	if(it == g_attributeMap.end<0>()){
		it = g_attributeMap.insert<0>(it, AccountAttributeElement(
			boost::make_shared<MySql::Center_AccountAttribute>(accountUuid.get(), slot, std::move(value))));
		it->obj->asyncSave(true);
	} else {
		it->obj->set_value(std::move(value));
	}

	const auto session = PlayerSessionMap::get(accountUuid);
	if(session){
		try {
			Msg::SC_AccountAttributes msg;
			msg.accountUuid = accountUuid.str();
			msg.attributes.emplace_back();
			auto &attribute = msg.attributes.back();
			attribute.slot = slot;
			attribute.value = it->obj->unlockedGet_value(); // 不要用 value，因为已经 move。
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->forceShutdown();
		}
	}
}

}
