#include "../precompiled.hpp"
#include "account_map.hpp"
#include <string.h>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include <poseidon/hash.hpp>
#include <poseidon/http/utilities.hpp>
#include <boost/container/flat_map.hpp>
#include "../mysql/account.hpp"
#include "../mysql/account_attribute.hpp"
#include "../events/account.hpp"

namespace EmperyPromotion {

namespace {
	struct StringCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};

	struct AccountElement {
		boost::shared_ptr<MySql::Promotion_Account> obj;

		AccountId accountId;
		std::string loginName;
		std::string phoneNumber;
		std::string nick;
		AccountId referrerId;

		explicit AccountElement(boost::shared_ptr<MySql::Promotion_Account> obj_)
			: obj(std::move(obj_))
			, accountId(obj->get_accountId()), loginName(obj->unlockedGet_loginName())
			, phoneNumber(obj->unlockedGet_phoneNumber()), nick(obj->unlockedGet_nick())
			, referrerId(obj->get_referrerId())
		{
		}
	};

	MULTI_INDEX_MAP(AccountMapDelegator, AccountElement,
		UNIQUE_MEMBER_INDEX(accountId)
		MULTI_MEMBER_INDEX(loginName, StringCaseComparator)
		MULTI_MEMBER_INDEX(phoneNumber)
		MULTI_MEMBER_INDEX(nick, StringCaseComparator)
		MULTI_MEMBER_INDEX(referrerId)
	)

	boost::shared_ptr<AccountMapDelegator> g_accountMap;

	struct AccountAttributeElement {
		boost::shared_ptr<MySql::Promotion_AccountAttribute> obj;

		AccountId accountId;
		std::pair<AccountId, unsigned> accountSlot;

		explicit AccountAttributeElement(boost::shared_ptr<MySql::Promotion_AccountAttribute> obj_)
			: obj(std::move(obj_))
			, accountId(obj->get_accountId())
			, accountSlot(std::make_pair(accountId, obj->get_slot()))
		{
		}
	};

	MULTI_INDEX_MAP(AccountAttributeMapDelegator, AccountAttributeElement,
		MULTI_MEMBER_INDEX(accountId)
		UNIQUE_MEMBER_INDEX(accountSlot)
	);

	boost::shared_ptr<AccountAttributeMapDelegator> g_attributeMap;

	struct SubordinateInfoCacheElement {
		boost::uint64_t subordCount;
		boost::uint64_t maxSubordLevel;
	};

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		const auto accountMap = boost::make_shared<AccountMapDelegator>();
		LOG_EMPERY_PROMOTION_INFO("Loading accounts...");
		conn->executeSql("SELECT * FROM `Promotion_Account`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Promotion_Account>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			accountMap->insert(AccountElement(std::move(obj)));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", accountMap->size(), " account(s).");
		g_accountMap = accountMap;
		handles.push(accountMap);

		const auto attributeMap = boost::make_shared<AccountAttributeMapDelegator>();
		LOG_EMPERY_PROMOTION_INFO("Loading account attributes...");
		conn->executeSql("SELECT * FROM `Promotion_AccountAttribute`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::Promotion_AccountAttribute>();
			obj->syncFetch(conn);
			const auto it = g_accountMap->find<0>(AccountId(obj->get_accountId()));
			if(it == g_accountMap->end<0>()){
				LOG_EMPERY_PROMOTION_ERROR("No such account: accountId = ", AccountId(obj->get_accountId()));
				continue;
			}
			obj->enableAutoSaving();
			attributeMap->insert(AccountAttributeElement(std::move(obj)));
		}
		LOG_EMPERY_PROMOTION_INFO("Loaded ", attributeMap->size(), " account attribute(s).");
		g_attributeMap = attributeMap;
		handles.push(attributeMap);

		LOG_EMPERY_PROMOTION_INFO("Updating subordinate info cache...");
		boost::container::flat_map<AccountId, SubordinateInfoCacheElement> tempMap;
		tempMap.reserve(accountMap->size());
		for(auto it = accountMap->begin(); it != accountMap->end(); ++it){
			boost::uint64_t level = 0;
			auto attrIt = attributeMap->find<1>(std::make_pair(it->accountId, AccountMap::ATTR_ACCOUNT_LEVEL));
			if(attrIt != attributeMap->end<1>()){
				level = boost::lexical_cast<boost::uint64_t>(attrIt->obj->unlockedGet_value());
			}

			auto referrerIt = accountMap->find<0>(it->referrerId);
			while(referrerIt != accountMap->end<0>()){
				const auto currentReferrerId = referrerIt->accountId;
				auto &elem = tempMap[currentReferrerId];
				++elem.subordCount;
				if(elem.maxSubordLevel < level){
					elem.maxSubordLevel = level;
				}
				referrerIt = accountMap->find<0>(referrerIt->referrerId);
			}
		}
		for(auto it = tempMap.begin(); it != tempMap.end(); ++it){
			const auto reallySetAttribute = [&](unsigned slot, std::string str){
				auto attrIt = attributeMap->find<1>(std::make_pair(it->first, slot));
				if(attrIt == attributeMap->end<1>()){
					attrIt = attributeMap->insert<1>(attrIt, AccountAttributeElement(
						boost::make_shared<MySql::Promotion_AccountAttribute>(it->first.get(), slot, std::move(str))));
					attrIt->obj->asyncSave(true);
				} else {
					if(attrIt->obj->unlockedGet_value() != str){
						attrIt->obj->set_value(std::move(str));
					}
				}
			};

			reallySetAttribute(AccountMap::ATTR_SUBORD_COUNT,     boost::lexical_cast<std::string>(it->second.subordCount));
			reallySetAttribute(AccountMap::ATTR_MAX_SUBORD_LEVEL, boost::lexical_cast<std::string>(it->second.maxSubordLevel));
		}
	}

	void fillAccountInfo(AccountMap::AccountInfo &info, const boost::shared_ptr<MySql::Promotion_Account> &obj){
		PROFILE_ME;

		info.accountId        = AccountId(obj->get_accountId());
		info.loginName        = obj->unlockedGet_loginName();
		info.phoneNumber      = obj->unlockedGet_phoneNumber();
		info.nick             = obj->unlockedGet_nick();
		info.passwordHash     = obj->unlockedGet_passwordHash();
		info.dealPasswordHash = obj->unlockedGet_dealPasswordHash();
		info.referrerId       = AccountId(obj->get_referrerId());
		info.flags            = obj->get_flags();
		info.bannedUntil      = obj->get_bannedUntil();
		info.createdTime      = obj->get_createdTime();
		info.createdIp        = obj->get_createdIp();
	}
}

bool AccountMap::has(AccountId accountId){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		return false;
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
		return false;
	}
	return true;
}
AccountMap::AccountInfo AccountMap::get(AccountId accountId){
	PROFILE_ME;

	AccountInfo info = { };
	info.accountId = accountId;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		return info;
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		return info;
	}
	fillAccountInfo(info, it->obj);
	return info;
}
AccountMap::AccountInfo AccountMap::require(AccountId accountId){
	PROFILE_ME;

	auto info = get(accountId);
	if(Poseidon::hasNoneFlagsOf(info.flags, FL_VALID)){
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	return info;
}

boost::uint64_t AccountMap::getCount(){
	return g_accountMap->size();
}
void AccountMap::getAll(std::vector<AccountMap::AccountInfo> &ret, boost::uint64_t begin, boost::uint64_t max){
	PROFILE_ME;

	const auto size = g_accountMap->size();
	if(begin >= size){
		return;
	}
	auto count = size - begin;
	if(count > max){
		count = max;
	}
	ret.reserve(ret.size() + count);
	auto it = g_accountMap->begin();
	std::advance(it, static_cast<std::ptrdiff_t>(begin));
	for(boost::uint64_t i = 0; i < count; ++it, ++i){
		AccountInfo info;
		fillAccountInfo(info, it->obj);
		ret.push_back(std::move(info));
	}
}

AccountMap::AccountInfo AccountMap::getByLoginName(const std::string &loginName){
	PROFILE_ME;

	AccountInfo info = { };
	info.loginName = loginName;

	const auto it = g_accountMap->find<1>(loginName);
	if(it == g_accountMap->end<1>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: loginName = ", loginName);
		return info;
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: loginName = ", loginName);
		return info;
	}
	fillAccountInfo(info, it->obj);
	return info;
}

void AccountMap::getByPhoneNumber(std::vector<AccountMap::AccountInfo> &ret, const std::string &phoneNumber){
	PROFILE_ME;

	const auto range = g_accountMap->equalRange<2>(phoneNumber);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
			LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", it->obj->get_accountId());
			continue;
		}
		AccountInfo info;
		fillAccountInfo(info, it->obj);
		ret.push_back(std::move(info));
	}
}
void AccountMap::getByReferrerId(std::vector<AccountMap::AccountInfo> &ret, AccountId referrerId){
	PROFILE_ME;

	const auto range = g_accountMap->equalRange<4>(referrerId);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
			LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", it->obj->get_accountId());
			continue;
		}
		AccountInfo info;
		fillAccountInfo(info, it->obj);
		ret.push_back(std::move(info));
	}
}

std::string AccountMap::getPasswordHash(const std::string &password){
	PROFILE_ME;

	auto salt = getConfig<std::string>("password_salt");
	const auto sha256 = Poseidon::sha256Hash(password + std::move(salt));
	return Poseidon::Http::base64Encode(sha256.data(), sha256.size());
}

void AccountMap::setLoginName(AccountId accountId, std::string loginName){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_accountMap->setKey<0, 1>(it, loginName);
	it->obj->set_loginName(std::move(loginName));
}
void AccountMap::setPhoneNumber(AccountId accountId, std::string phoneNumber){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_accountMap->setKey<0, 2>(it, phoneNumber);
	it->obj->set_phoneNumber(std::move(phoneNumber));
}
void AccountMap::setNick(AccountId accountId, std::string nick){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_accountMap->setKey<0, 3>(it, nick);
	it->obj->set_nick(std::move(nick));
}
void AccountMap::setPassword(AccountId accountId, const std::string &password){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_passwordHash(getPasswordHash(password));
}
void AccountMap::setDealPassword(AccountId accountId, const std::string &dealPassword){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_dealPasswordHash(getPasswordHash(dealPassword));
}
void AccountMap::setFlags(AccountId accountId, boost::uint64_t flags){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
//	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
//		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
//		DEBUG_THROW(Exception, sslit("Account deleted"));
//	}

	it->obj->set_flags(flags);
}
void AccountMap::setBannedUntil(AccountId accountId, boost::uint64_t bannedUntil){
	PROFILE_ME;

	const auto it = g_accountMap->find<0>(accountId);
	if(it == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_DEBUG("Account not found: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::hasNoneFlagsOf(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_PROMOTION_DEBUG("Account deleted: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_bannedUntil(bannedUntil);
}

AccountId AccountMap::create(std::string loginName, std::string phoneNumber, std::string nick,
	const std::string &password, const std::string &dealPassword, AccountId referrerId, boost::uint64_t flags, std::string createdIp)
{
	PROFILE_ME;

	if(referrerId){
		auto referrerIt = g_accountMap->find<0>(referrerId);
		for(;;){
			if(referrerIt == g_accountMap->end<0>()){
				LOG_EMPERY_PROMOTION_DEBUG("No such referrer: referrerId = ", referrerId);
				DEBUG_THROW(Exception, sslit("No such referrer"));
			}
			const auto nextReferrerId = referrerIt->referrerId;
			if(!nextReferrerId){
				break;
			}
			if(nextReferrerId == referrerId){
				LOG_EMPERY_PROMOTION_ERROR("Circular referrer loop detected! referrerId = ", referrerId);
				DEBUG_THROW(Exception, sslit("Circular referrer loop detected"));
			}
			referrerIt = g_accountMap->find<0>(nextReferrerId);
		}
	}

	auto it = g_accountMap->find<1>(loginName);
	if(it != g_accountMap->end<1>()){
		LOG_EMPERY_PROMOTION_DEBUG("Duplicate loginName: loginName = ", loginName);
		DEBUG_THROW(Exception, sslit("Duplicate loginName"));
	}

	auto accountId = g_accountMap->empty() ? AccountId() : g_accountMap->rbegin<0>()->accountId;
	do {
		++accountId;
	} while(g_accountMap->find<0>(accountId) != g_accountMap->end<0>());

	Poseidon::addFlags(flags, AccountMap::FL_VALID);
	const auto localNow = Poseidon::getLocalTime();
	auto obj = boost::make_shared<MySql::Promotion_Account>(accountId.get(), std::move(loginName),
		std::move(phoneNumber), std::move(nick), getPasswordHash(password), getPasswordHash(dealPassword),
		referrerId.get(), flags, 0, localNow, std::move(createdIp));
	obj->asyncSave(true);
	it = g_accountMap->insert<1>(it, AccountElement(std::move(obj)));

	auto referrerIt = g_accountMap->find<0>(referrerId);
	while(referrerIt != g_accountMap->end<0>()){
		const auto currentReferrerId = referrerIt->accountId;
		try {
			const auto oldCount = castAttribute<boost::uint64_t>(currentReferrerId, ATTR_SUBORD_COUNT);
			boost::uint64_t newCount = oldCount + 1;
			LOG_EMPERY_PROMOTION_DEBUG("Updating subordinate count: currentReferrerId = ", currentReferrerId,
				", oldCount = ", oldCount, ", newCount = ", newCount);
			setAttribute(currentReferrerId, ATTR_SUBORD_COUNT, boost::lexical_cast<std::string>(newCount));
		} catch(std::exception &e){
			LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
		}
		referrerIt = g_accountMap->find<0>(currentReferrerId);
	}

	return accountId;
}

const std::string &AccountMap::getAttribute(AccountId accountId, unsigned slot){
	PROFILE_ME;

	const auto it = g_attributeMap->find<1>(std::make_pair(accountId, slot));
	if(it == g_attributeMap->end<1>()){
		LOG_EMPERY_PROMOTION_TRACE("Account attribute not found: accountId = ", accountId, ", slot = ", slot);
		return Poseidon::EMPTY_STRING;
	}
	return it->obj->unlockedGet_value();
}
void AccountMap::getAttributes(std::vector<std::pair<unsigned, std::string>> &ret, AccountId accountId){
	PROFILE_ME;

	const auto range = g_attributeMap->equalRange<0>(accountId);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.push_back(std::make_pair(it->obj->get_slot(), it->obj->unlockedGet_value()));
	}
}
void AccountMap::touchAttribute(AccountId accountId, unsigned slot){
	PROFILE_ME;

	auto it = g_attributeMap->find<1>(std::make_pair(accountId, slot));
	if(it == g_attributeMap->end<1>()){
		it = g_attributeMap->insert<1>(it, AccountAttributeElement(
			boost::make_shared<MySql::Promotion_AccountAttribute>(accountId.get(), slot, std::string())));
		it->obj->asyncSave(true);
	}
}
void AccountMap::setAttribute(AccountId accountId, unsigned slot, std::string value){
	PROFILE_ME;

	const auto accountIt = g_accountMap->find<0>(accountId);
	if(accountIt == g_accountMap->end<0>()){
		LOG_EMPERY_PROMOTION_WARNING("No such account: accountId = ", accountId);
		DEBUG_THROW(Exception, sslit("No such account"));
	}

	boost::uint64_t oldLevel = 0, newLevel = 0;

	if(slot == ATTR_ACCOUNT_LEVEL){
		newLevel = boost::lexical_cast<boost::uint64_t>(value);
	}

	auto it = g_attributeMap->find<1>(std::make_pair(accountId, slot));
	if(it == g_attributeMap->end<1>()){
		it = g_attributeMap->insert<1>(it, AccountAttributeElement(
			boost::make_shared<MySql::Promotion_AccountAttribute>(accountId.get(), slot, std::move(value))));
		it->obj->asyncSave(true);
	} else {
		if(slot == ATTR_ACCOUNT_LEVEL){
			oldLevel = boost::lexical_cast<boost::uint64_t>(it->obj->unlockedGet_value());
		}

		it->obj->set_value(std::move(value));
	}

	if(oldLevel != newLevel){
		auto referrerIt = g_accountMap->find<0>(accountIt->referrerId);
		while(referrerIt != g_accountMap->end<0>()){
			const auto currentReferrerId = referrerIt->accountId;
			try {
				const auto oldMaxLevel = castAttribute<boost::uint64_t>(currentReferrerId, ATTR_MAX_SUBORD_LEVEL);
				boost::uint64_t newMaxLevel;
				if(oldLevel < newLevel){
					newMaxLevel = oldMaxLevel;
					if(newMaxLevel < newLevel){
						newMaxLevel = newLevel;
					}
				} else {
					newMaxLevel = 0;
					const auto range = g_accountMap->equalRange<4>(currentReferrerId);
					for(auto subordIt = range.first; subordIt != range.second; ++subordIt){
						const auto level = castAttribute<boost::uint64_t>(subordIt->accountId, ATTR_MAX_SUBORD_LEVEL);
						if(newMaxLevel < level){
							newMaxLevel = level;
						}
					}
				}
				if(oldMaxLevel == newMaxLevel){
					break;
				}
				LOG_EMPERY_PROMOTION_DEBUG("Updating max subordinate level: currentReferrerId = ", currentReferrerId,
					", oldMaxLevel = ", oldMaxLevel, ", newMaxLevel = ", newMaxLevel);
				setAttribute(currentReferrerId, ATTR_MAX_SUBORD_LEVEL, boost::lexical_cast<std::string>(newLevel));
			} catch(std::exception &e){
				LOG_EMPERY_PROMOTION_ERROR("std::exception thrown: what = ", e.what());
			}
			referrerIt = g_accountMap->find<0>(currentReferrerId);
		}
	}
}

}
