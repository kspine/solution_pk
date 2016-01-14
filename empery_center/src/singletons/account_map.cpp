#include "../precompiled.hpp"
#include "account_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <tuple>
#include "player_session_map.hpp"
#include "../mysql/account.hpp"
#include "../msg/sc_account.hpp"
#include "../player_session.hpp"
#include "../events/account.hpp"
#include "../account.hpp"
#include "../account_attribute_ids.hpp"
#include "item_box_map.hpp"
#include "../item_box.hpp"
#include "../data/item.hpp"
#include "../string_utilities.hpp"

namespace EmperyCenter {

namespace {
	struct AccountElement {
		boost::shared_ptr<Account> account;

		AccountUuid account_uuid;
		std::pair<PlatformId, std::size_t> platform_id_login_name_hash;
		std::size_t nick_hash;
		AccountUuid referrer_uuid;

		explicit AccountElement(boost::shared_ptr<Account> account_)
			: account(std::move(account_))
			, account_uuid(account->get_account_uuid())
			, platform_id_login_name_hash(account->get_platform_id(), hash_string_nocase(account->get_login_name()))
			, nick_hash(hash_string_nocase(account->get_nick())), referrer_uuid(account->get_referrer_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(AccountMapContainer, AccountElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(platform_id_login_name_hash)
		MULTI_MEMBER_INDEX(nick_hash)
		MULTI_MEMBER_INDEX(referrer_uuid)
	)

	boost::shared_ptr<AccountMapContainer> g_account_map;

	enum CacheType : std::uint64_t {
		CT_NONE       = 0x0000, // Silent the warning.
		CT_NICK       = 0x0001,
		CT_ATTRS      = 0x0002,
		CT_PRIV_ATTRS = 0x0004,
		CT_ITEMS      = 0x0008,
	};

	struct InfoCacheElement {
		std::uint64_t expiry_time;
		std::tuple<AccountUuid, boost::weak_ptr<PlayerSession>, CacheType> key;

		InfoCacheElement(std::uint64_t expiry_time_,
			AccountUuid account_uuid_, const boost::shared_ptr<PlayerSession> &session_, CacheType type_)
			: expiry_time(expiry_time_), key(account_uuid_, session_, type_)
		{
		}
	};

	MULTI_INDEX_MAP(InfoCacheContainer, InfoCacheElement,
		MULTI_MEMBER_INDEX(expiry_time)
		UNIQUE_MEMBER_INDEX(key)
	)

	boost::shared_ptr<InfoCacheContainer> g_info_cache_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// Account
		struct TempAccountElement {
			boost::shared_ptr<MySql::Center_Account> obj;
			std::vector<boost::shared_ptr<MySql::Center_AccountAttribute>> attributes;
		};
		std::map<AccountUuid, TempAccountElement> temp_account_map;

		LOG_EMPERY_CENTER_INFO("Loading accounts...");
		conn->execute_sql("SELECT * FROM `Center_Account`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Account>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto account_uuid = AccountUuid(obj->get_account_uuid());
			temp_account_map[account_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_account_map.size(), " account(s).");

		LOG_EMPERY_CENTER_INFO("Loading account attributes...");
		conn->execute_sql("SELECT * FROM `Center_AccountAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_AccountAttribute>();
			obj->fetch(conn);
			const auto account_uuid = AccountUuid(obj->unlocked_get_account_uuid());
			const auto it = temp_account_map.find(account_uuid);
			if(it == temp_account_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.attributes.emplace_back(std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading account attributes.");

		const auto account_map = boost::make_shared<AccountMapContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			auto account = boost::make_shared<Account>(std::move(it->second.obj), it->second.attributes);

			account_map->insert(AccountElement(std::move(account)));
		}
		g_account_map = account_map;
		handles.push(account_map);

		const auto info_cache_map = boost::make_shared<InfoCacheContainer>();
		g_info_cache_map = info_cache_map;
		handles.push(info_cache_map);
	}

	void synchronize_account_and_update_cache(std::uint64_t now, std::uint64_t cache_timeout,
		const boost::shared_ptr<Account> &account, const boost::shared_ptr<PlayerSession> &session, std::uint64_t flags) noexcept
	try {
		PROFILE_ME;

		if(flags == 0){
			return;
		}

		const auto account_uuid = account->get_account_uuid();

		Msg::SC_AccountAttributes msg;
		msg.account_uuid = account_uuid.str();

		// nick
		if(flags & CT_NICK){
			msg.nick = account->get_nick();
		}

		// attributes
		boost::container::flat_map<AccountAttributeId, std::string> attributes;
		auto public_end = attributes.end();

		const auto fetch_attributes = [&]{
			if(attributes.empty()){
				account->get_attributes(attributes);
				msg.attributes.reserve(attributes.size());
				public_end = attributes.upper_bound(AccountAttributeIds::ID_PUBLIC_END);
			}
		};
		const auto copy_attributes  = [&](decltype(attributes)::iterator begin, decltype(attributes)::iterator end){
			for(auto it = begin; it != end; ++it){
				auto &attribute = *msg.attributes.emplace(msg.attributes.end());
				attribute.account_attribute_id = it->first.get();
				attribute.value                = std::move(it->second);
			}
		};

		if(flags & CT_ATTRS){
			fetch_attributes();
			copy_attributes(attributes.begin(), public_end);
		}
		if(flags & CT_PRIV_ATTRS){
			fetch_attributes();
			copy_attributes(public_end, attributes.end());
		}

		// items
		if(flags & CT_ITEMS){
			const auto item_box = ItemBoxMap::require(account_uuid);

			std::vector<boost::shared_ptr<const Data::Item>> items_to_check;
			Data::Item::get_public(items_to_check);
			msg.public_items.reserve(items_to_check.size());
			for(auto it = items_to_check.begin(); it != items_to_check.end(); ++it){
				const auto &item_data = *it;
				auto info = item_box->get(item_data->item_id);

				auto &item = *msg.public_items.emplace(msg.public_items.end());
				item.item_id    = info.item_id.get();
				item.item_count = info.count;
			}
		}

		// 其他。
		msg.promotion_level = account->get_promotion_level();
		msg.activated       = account->has_been_activated();

		session->send(msg);

		const auto update_cache = [&](CacheType type){
			if(cache_timeout != 0){
				const auto expiry_time = saturated_add(now, cache_timeout);
				const auto result = g_info_cache_map->insert(InfoCacheElement(expiry_time, account_uuid, session, type));
				if(!result.second){
					g_info_cache_map->replace(result.first, InfoCacheElement(expiry_time, account_uuid, session, type));
				}
			}
		};

		if(flags & CT_NICK){
			update_cache(CT_NICK);
		}
		if(flags & CT_ATTRS){
			update_cache(CT_ATTRS);
		}
		if(flags & CT_PRIV_ATTRS){
			update_cache(CT_PRIV_ATTRS);
		}
		if(flags & CT_ITEMS){
			update_cache(CT_ITEMS);
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		session->shutdown(e.what());
	}
}

boost::shared_ptr<Account> AccountMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	const auto it = g_account_map->find<0>(account_uuid);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		return { };
	}
	return it->account;
}
boost::shared_ptr<Account> AccountMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Account not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	return account;
}

boost::shared_ptr<Account> AccountMap::get_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	const auto key = std::make_pair(platform_id, hash_string_nocase(login_name));
	const auto range = g_account_map->equal_range<1>(key);
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->account->get_login_name(), login_name)){
			return it->account;
		}
	}
	LOG_EMPERY_CENTER_DEBUG("Login name not found: platform_id = ", platform_id, ", login_name = ", login_name);
	return { };
}
boost::shared_ptr<Account> AccountMap::require_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	auto account = get_by_login_name(platform_id, login_name);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Login name not found: platform_id = ", platform_id, ", login_name = ", login_name);
		DEBUG_THROW(Exception, sslit("Login name not found"));
	}
	return account;
}

void AccountMap::get_all(std::vector<boost::shared_ptr<Account>> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + g_account_map->size());
	for(auto it = g_account_map->begin<0>(); it != g_account_map->end<0>(); ++it){
		ret.emplace_back(it->account);
	}
}
void AccountMap::get_by_nick(std::vector<boost::shared_ptr<Account>> &ret, const std::string &nick){
	PROFILE_ME;

	const auto key = hash_string_nocase(nick);
	const auto range = g_account_map->equal_range<2>(key);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(!are_strings_equal_nocase(it->account->get_nick(), nick)){
			continue;
		}
		ret.emplace_back(it->account);
	}
}
void AccountMap::get_by_referrer(std::vector<boost::shared_ptr<Account>> &ret, AccountUuid referrer_uuid){
	PROFILE_ME;

	const auto range = g_account_map->equal_range<3>(referrer_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->account);
	}
}

void AccountMap::insert(const boost::shared_ptr<Account> &account, const std::string &remote_ip){
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	const auto withdrawn = boost::make_shared<bool>(true);
	Poseidon::async_raise_event(boost::make_shared<Events::AccountCreated>(account_uuid, remote_ip), withdrawn);

	LOG_EMPERY_CENTER_DEBUG("Inserting account: account_uuid = ", account_uuid);
	if(!g_account_map->insert(AccountElement(account)).second){
		LOG_EMPERY_CENTER_WARNING("Account already exists: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account already exists"));
	}

	*withdrawn = false;
}
void AccountMap::update(const boost::shared_ptr<Account> &account, bool throws_if_not_exists){
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	const auto it = g_account_map->find<0>(account_uuid);
	if(it == g_account_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Account not found: account_uuid = ", account_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Account not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating account: account_uuid = ", account_uuid);
	g_account_map->replace<0>(it, AccountElement(account));
}

void AccountMap::synchronize_account_with_player(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
	bool wants_nick, bool wants_attributes, bool wants_private_attributes, bool wants_items) noexcept
{
	PROFILE_ME;

	const auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Account not found: account_uuid = ", account_uuid);
		return;
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<std::uint64_t>("account_info_cache_timeout", 0);

	g_info_cache_map->erase<0>(g_info_cache_map->begin<0>(), g_info_cache_map->upper_bound<0>(now));

	synchronize_account_and_update_cache(now, cache_timeout, account, session,
		(wants_nick               ? CT_NICK       : CT_NONE) |
		(wants_attributes         ? CT_ATTRS      : CT_NONE) |
		(wants_private_attributes ? CT_PRIV_ATTRS : CT_NONE) |
		(wants_items              ? CT_ITEMS      : CT_NONE));
}
void AccountMap::cached_synchronize_account_with_player(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
	bool wants_nick, bool wants_attributes, bool wants_private_attributes, bool wants_items) noexcept
{
	PROFILE_ME;

	const auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Account not found: account_uuid = ", account_uuid);
		return;
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<std::uint64_t>("account_info_cache_timeout", 0);

	g_info_cache_map->erase<0>(g_info_cache_map->begin<0>(), g_info_cache_map->upper_bound<0>(now));

	const auto is_miss = [&](CacheType type){
		return g_info_cache_map->find<1>(std::make_tuple(account_uuid, session, type)) == g_info_cache_map->end<1>();
	};

	synchronize_account_and_update_cache(now, cache_timeout, account, session,
		(wants_nick               && is_miss(CT_NICK      ) ? CT_NICK       : CT_NONE) |
		(wants_attributes         && is_miss(CT_ATTRS     ) ? CT_ATTRS      : CT_NONE) |
		(wants_private_attributes && is_miss(CT_PRIV_ATTRS) ? CT_PRIV_ATTRS : CT_NONE) |
		(wants_items              && is_miss(CT_ITEMS     ) ? CT_ITEMS      : CT_NONE));
}

}
