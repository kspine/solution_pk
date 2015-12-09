#include "../precompiled.hpp"
#include "account_map.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
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

#include "../castle.hpp" // FIXME remove this
#include "../transaction_element.hpp" // FIXME remove this
#include "world_map.hpp" // FIXME remove this
#include "../map_object_type_ids.hpp" // FIXME remove this

namespace EmperyCenter {

namespace {
	inline std::size_t hash_string_nocase(const std::string &str) noexcept {
		// http://www.isthe.com/chongo/tech/comp/fnv/
		std::size_t hash;
		if(sizeof(std::size_t) < 8){
			hash = 2166136261u;
			for(auto it = str.begin(); it != str.end(); ++it){
				const auto ch = std::toupper(*it);
				hash ^= static_cast<unsigned char>(ch);
				hash *= 16777619u;
			}
		} else {
			hash = 14695981039346656037u;
			for(auto it = str.begin(); it != str.end(); ++it){
				const auto ch = std::toupper(*it);
				hash ^= static_cast<unsigned char>(ch);
				hash *= 1099511628211u;
			}
		}
		return hash;
	}
	inline bool are_strings_equal_nocase(const std::string &lhs, const std::string &rhs) noexcept {
		if(lhs.size() != rhs.size()){
			return false;
		}
		for(std::size_t i = 0; i < lhs.size(); ++i){
			const auto ch1 = std::toupper(lhs[i]);
			const auto ch2 = std::toupper(rhs[i]);
			if(ch1 != ch2){
				return false;
			}
		}
		return true;
	}

	struct AccountElement {
		boost::shared_ptr<Account> account;

		AccountUuid account_uuid;
		std::pair<PlatformId, std::size_t> platform_id_login_name_hash;
		std::size_t nick_hash;

		explicit AccountElement(boost::shared_ptr<Account> account_)
			: account(std::move(account_))
			, account_uuid(account->get_account_uuid())
			, platform_id_login_name_hash(account->get_platform_id(), hash_string_nocase(account->get_login_name()))
			, nick_hash(hash_string_nocase(account->get_nick()))
		{
		}
	};

	MULTI_INDEX_MAP(AccountMapContainer, AccountElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(platform_id_login_name_hash)
		MULTI_MEMBER_INDEX(nick_hash)
	)

	boost::shared_ptr<AccountMapContainer> g_account_map;

	struct InfoCacheElement {
		boost::uint64_t expiry_time;
		std::pair<AccountUuid, boost::weak_ptr<PlayerSession>> account_uuid_session;

		InfoCacheElement(boost::uint64_t expiry_time_, AccountUuid account_uuid_, const boost::shared_ptr<PlayerSession> &session_)
			: expiry_time(expiry_time_), account_uuid_session(account_uuid_, session_)
		{
		}
	};

	MULTI_INDEX_MAP(InfoCacheContainer, InfoCacheElement,
		MULTI_MEMBER_INDEX(expiry_time)
		UNIQUE_MEMBER_INDEX(account_uuid_session)
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

		auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountSetToken>(
			[](const boost::shared_ptr<Events::AccountSetToken> &event){
				PROFILE_ME;

				const auto &platform_id = event->platform_id;
				const auto &login_name  = event->login_name;
				const auto &login_token = event->login_token;
				const auto &expiry_time = event->expiry_time;
				const auto &remote_ip   = event->remote_ip;
				LOG_EMPERY_CENTER_INFO("Set token: platform_id = ", platform_id, ", login_name = ", login_name,
					", login_token = ", login_token, ", expiry_time = ", expiry_time, ", remote_ip = ", remote_ip);

				auto account = AccountMap::get_by_login_name(platform_id, login_name);
				if(!account){
					const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
					const auto utc_now = Poseidon::get_utc_time();
					LOG_EMPERY_CENTER_INFO("Creating new account: account_uuid = ", account_uuid);
					account = boost::make_shared<Account>(account_uuid, platform_id, login_name, utc_now,
						login_name, 0);
					AccountMap::insert(account, remote_ip);
				}
				account->set_login_token(login_token, expiry_time);
			});
		LOG_EMPERY_CENTER_DEBUG("Created AccountSetToken listener");
		handles.push(listener);
	}

	void synchronize_account_and_update_cache(boost::uint64_t now, boost::uint64_t cache_timeout,
		const boost::shared_ptr<Account> &account, const boost::shared_ptr<PlayerSession> &session,
		bool wants_nick, bool wants_attributes, bool wants_private_attributes, bool wants_items)
	{
		PROFILE_ME;

		const auto account_uuid = account->get_account_uuid();

		Msg::SC_AccountAttributes msg;
		msg.account_uuid = account_uuid.str();
		if(wants_nick){
			msg.nick = account->get_nick();
		}
		if(wants_attributes){
			auto attribute_id_end = AccountAttributeIds::ID_PUBLIC_END;
			if(wants_private_attributes){
				attribute_id_end = AccountAttributeIds::ID_END;
			}

			boost::container::flat_map<AccountAttributeId, std::string> attributes;
			account->get_attributes(attributes);
			const auto lower = attributes.begin();
			const auto upper = attributes.lower_bound(attribute_id_end);

			msg.attributes.reserve(static_cast<std::size_t>(std::distance(lower, upper)));
			for(auto it = lower; it != upper; ++it){
				msg.attributes.emplace_back();
				auto &attribute = msg.attributes.back();
				attribute.account_attribute_id = it->first.get();
				attribute.value                = std::move(it->second);
			}
		}
		if(wants_items){
			const auto item_box = ItemBoxMap::require(account_uuid);

			std::vector<boost::shared_ptr<const Data::Item>> items_to_check;
			Data::Item::get_public(items_to_check);
			msg.public_items.reserve(items_to_check.size());
			for(auto it = items_to_check.begin(); it != items_to_check.end(); ++it){
				const auto &item_data = *it;
				auto info = item_box->get(item_data->item_id);

				msg.public_items.emplace_back();
				auto &item = msg.public_items.back();
				item.item_id    = info.item_id.get();
				item.item_count = info.count;
			}
		}
		session->send(msg);

		if(cache_timeout != 0){
			const auto expiry_time = saturated_add(now, cache_timeout);
			auto it = g_info_cache_map->find<1>(std::make_pair(account_uuid, session));
			if(it == g_info_cache_map->end<1>()){
				it = g_info_cache_map->insert<1>(it, InfoCacheElement(expiry_time, account_uuid, session));
			} else {
				g_info_cache_map->replace<1>(it, InfoCacheElement(expiry_time, account_uuid, session));
			}
		}
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
		if(are_strings_equal_nocase(login_name, it->account->get_login_name())){
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
		if(!are_strings_equal_nocase(nick, it->account->get_nick())){
			continue;
		}
		ret.emplace_back(it->account);
	}
}

void AccountMap::insert(const boost::shared_ptr<Account> &account, const std::string &remote_ip){
	PROFILE_ME;

	const auto account_uuid = account->get_account_uuid();

	const auto it = g_account_map->find<0>(account_uuid);
	if(it != g_account_map->end<0>()){
		LOG_EMPERY_CENTER_WARNING("Account already exists: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account already exists"));
	}

	const auto withdrawn = boost::make_shared<bool>(true);
	Poseidon::async_raise_event(boost::make_shared<Events::AccountCreated>(account_uuid, remote_ip), withdrawn);

	LOG_EMPERY_CENTER_DEBUG("Inserting account: account_uuid = ", account_uuid);
	g_account_map->insert(AccountElement(account));

	*withdrawn = false;

// FIXME remove this
for(int i = 0; i < 1; ++i){
	auto map_object_uuid = MapObjectUuid(Poseidon::Uuid::random());
	auto coord = Coord((boost::int32_t)Poseidon::rand32(0, 400) - 200,
                   	(boost::int32_t)Poseidon::rand32(0, 400) - 200);
	auto castle = boost::make_shared<Castle>(map_object_uuid, account_uuid, MapObjectUuid(), "aaa", coord);

	std::vector<ResourceTransactionElement> rsrc;
	rsrc.emplace_back(ResourceTransactionElement::OP_ADD, ResourceId(1101001), 500000000, ReasonId(0), 0, 0, 0);
	rsrc.emplace_back(ResourceTransactionElement::OP_ADD, ResourceId(1101002), 500000000, ReasonId(0), 0, 0, 0);
	rsrc.emplace_back(ResourceTransactionElement::OP_ADD, ResourceId(1101003), 500000000, ReasonId(0), 0, 0, 0);
	castle->commit_resource_transaction(rsrc);

	castle->create_building_mission(BuildingBaseId(2),  Castle::MIS_CONSTRUCT, BuildingId(1902001));
	castle->create_building_mission(BuildingBaseId(3),  Castle::MIS_CONSTRUCT, BuildingId(1902002));
	castle->create_building_mission(BuildingBaseId(4),  Castle::MIS_CONSTRUCT, BuildingId(1902003));
	castle->create_building_mission(BuildingBaseId(5),  Castle::MIS_CONSTRUCT, BuildingId(1902004));
	castle->create_building_mission(BuildingBaseId(11), Castle::MIS_CONSTRUCT, BuildingId(1904001));
	castle->create_building_mission(BuildingBaseId(12), Castle::MIS_CONSTRUCT, BuildingId(1904001));
	castle->create_building_mission(BuildingBaseId(13), Castle::MIS_CONSTRUCT, BuildingId(1904001));
	castle->create_building_mission(BuildingBaseId(14), Castle::MIS_CONSTRUCT, BuildingId(1904001));

	WorldMap::insert_map_object(castle);
}
// end FIXME
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
	const auto platform_id_login_name_hash = std::make_pair(account->get_platform_id(), hash_string_nocase(account->get_login_name()));
	if(it->platform_id_login_name_hash != platform_id_login_name_hash){
		g_account_map->set_key<0, 1>(it, platform_id_login_name_hash);
	}
	const auto nick_hash = hash_string_nocase(account->get_nick());
	if(it->nick_hash != nick_hash){
		g_account_map->set_key<0, 2>(it, nick_hash);
	}
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
	const auto cache_timeout = get_config<boost::uint64_t>("account_info_cache_timeout", 0);

	g_info_cache_map->erase<0>(g_info_cache_map->begin<0>(), g_info_cache_map->upper_bound<0>(now));

	synchronize_account_and_update_cache(now, cache_timeout, account, session,
		wants_nick, wants_attributes, wants_private_attributes, wants_items);
}
void AccountMap::cached_synchronize_account_with_player(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session) noexcept {
	PROFILE_ME;

	const auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("Account not found: account_uuid = ", account_uuid);
		return;
	}

	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<boost::uint64_t>("account_info_cache_timeout", 0);

	g_info_cache_map->erase<0>(g_info_cache_map->begin<0>(), g_info_cache_map->upper_bound<0>(now));

	if(g_info_cache_map->find<1>(std::make_pair(account_uuid, session)) != g_info_cache_map->end<1>()){
		LOG_EMPERY_CENTER_DEBUG("Cache hit: account_uuid = ", account_uuid);
		return;
	}

	synchronize_account_and_update_cache(now, cache_timeout, account, session,
		true, true, false, true);
}

}
