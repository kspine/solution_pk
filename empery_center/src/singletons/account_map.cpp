#include "../precompiled.hpp"
#include "account_map.hpp"
#include <string.h>
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/event_dispatcher.hpp>
#include "player_session_map.hpp"
#include "../mysql/account.hpp"
#include "../msg/sc_account.hpp"
#include "../player_session.hpp"
#include "../events/account.hpp"

#include "../castle.hpp" // FIXME remove this
#include "../transaction_element.hpp" // FIXME remove this
#include "map_object_map.hpp" // FIXME remove this
#include "../map_object_type_ids.hpp" // FIXME remove this

namespace EmperyCenter {

namespace {
	struct StringCaseComparator {
		bool operator()(const std::string &lhs, const std::string &rhs) const {
			return ::strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};

	struct LoginKey {
		PlatformId platform_id;
		const std::string *login_name;

		LoginKey(PlatformId platform_id_, const std::string &login_name_)
			: platform_id(platform_id_), login_name(&login_name_)
		{
		}

		bool operator<(const LoginKey &rhs) const {
			if(platform_id != rhs.platform_id){
				return platform_id < rhs.platform_id;
			}
			return StringCaseComparator()(*login_name, *rhs.login_name);
		}
	};

	struct AccountElement {
		boost::shared_ptr<MySql::Center_Account> obj;

		AccountUuid account_uuid;
		std::string nick;
		LoginKey login_key;

		explicit AccountElement(boost::shared_ptr<MySql::Center_Account> obj_)
			: obj(std::move(obj_))
			, account_uuid(obj->unlocked_get_account_uuid()), nick(obj->unlocked_get_nick())
			, login_key(PlatformId(obj->get_platform_id()), obj->unlocked_get_login_name())
		{
		}
	};

	MULTI_INDEX_MAP(AccountMapDelegator, AccountElement,
		UNIQUE_MEMBER_INDEX(account_uuid)
		MULTI_MEMBER_INDEX(nick, StringCaseComparator)
		UNIQUE_MEMBER_INDEX(login_key)
	)

	AccountMapDelegator g_account_map;

	struct AccountAttributeElement {
		boost::shared_ptr<MySql::Center_AccountAttribute> obj;

		std::pair<AccountUuid, unsigned> account_slot;

		explicit AccountAttributeElement(boost::shared_ptr<MySql::Center_AccountAttribute> obj_)
			: obj(std::move(obj_))
			, account_slot(std::make_pair(AccountUuid(obj->get_account_uuid()), obj->get_slot()))
		{
		}
	};

	MULTI_INDEX_MAP(AccountAttributeMapDelegator, AccountAttributeElement,
		UNIQUE_MEMBER_INDEX(account_slot)
	)

	AccountAttributeMapDelegator g_attribute_map;

	struct InfoTimestampElement {
		boost::uint64_t expiry_time;
		std::pair<AccountUuid, boost::weak_ptr<PlayerSession>> account_uuid_session;

		InfoTimestampElement(boost::uint64_t expiry_time_,
			AccountUuid account_uuid_, const boost::shared_ptr<PlayerSession> &session_)
			: expiry_time(expiry_time_), account_uuid_session(account_uuid_, session_)
		{
		}
	};

	MULTI_INDEX_MAP(InfoTimestampMap, InfoTimestampElement,
		MULTI_MEMBER_INDEX(expiry_time)
		UNIQUE_MEMBER_INDEX(account_uuid_session)
	)

	boost::shared_ptr<InfoTimestampMap> g_info_timestamp_map;

	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		AccountMapDelegator account_map;
		LOG_EMPERY_CENTER_INFO("Loading accounts...");
		conn->execute_sql("SELECT * FROM `Center_Account`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Account>();
			obj->sync_fetch(conn);
			obj->enable_auto_saving();
			account_map.insert(AccountElement(std::move(obj)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", account_map.size(), " account(s).");
		g_account_map.swap(account_map);

		AccountAttributeMapDelegator attribute_map;
		LOG_EMPERY_CENTER_INFO("Loading account attributes...");
		conn->execute_sql("SELECT * FROM `Center_AccountAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_AccountAttribute>();
			obj->sync_fetch(conn);
			const auto it = g_account_map.find<0>(AccountUuid(obj->unlocked_get_account_uuid()));
			if(it == g_account_map.end<0>()){
				LOG_EMPERY_CENTER_ERROR("No such account: account_uuid = ", obj->unlocked_get_account_uuid());
				continue;
			}
			obj->enable_auto_saving();
			attribute_map.insert(AccountAttributeElement(std::move(obj)));
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", attribute_map.size(), " account attribute(s).");
		g_attribute_map.swap(attribute_map);

		const auto info_timestamp_map = boost::make_shared<InfoTimestampMap>();
		g_info_timestamp_map = info_timestamp_map;
		handles.push(info_timestamp_map);

		auto listener = Poseidon::EventDispatcher::register_listener<Events::AccountSetToken>(
			[](const boost::shared_ptr<Events::AccountSetToken> &event){
				PROFILE_ME;
				LOG_EMPERY_CENTER_INFO("Set token: platform_id = ", event->platform_id, ", login_name = ", event->login_name,
					", login_token = ", event->login_token, ", expiry_time = ", event->expiry_time, ", remote_ip = ", event->remote_ip);
				const auto account_uuid = AccountMap::create(event->platform_id, event->login_name,
					event->login_name, 0, event->remote_ip).first;
				AccountMap::set_login_token(account_uuid, event->login_token, event->expiry_time);
			});
		LOG_EMPERY_CENTER_DEBUG("Created AccountSetToken listener");
		handles.push(listener);
	}

	void fill_account_info(AccountMap::AccountInfo &info, const boost::shared_ptr<MySql::Center_Account> &obj){
		PROFILE_ME;

		info.account_uuid = AccountUuid(obj->unlocked_get_account_uuid());
		info.login_name   = obj->unlocked_get_login_name();
		info.nick        = obj->unlocked_get_nick();
		info.flags       = obj->get_flags();
		info.created_time = obj->get_created_time();
	}
	void fill_login_info(AccountMap::LoginInfo &info, const boost::shared_ptr<MySql::Center_Account> &obj){
		PROFILE_ME;

		info.platform_id           = PlatformId(obj->get_platform_id());
		info.login_name            = obj->unlocked_get_login_name();
		info.account_uuid          = AccountUuid(obj->unlocked_get_account_uuid());
		info.flags                 = obj->get_flags();
		info.login_token           = obj->unlocked_get_login_token();
		info.login_token_expiry_time = obj->get_login_token_expiry_time();
		info.banned_until          = obj->get_banned_until();
	}
}

bool AccountMap::has(AccountUuid account_uuid){
	PROFILE_ME;

	const auto it = g_account_map.find<0>(account_uuid);
	if(it == g_account_map.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		return false;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: account_uuid = ", account_uuid);
		return false;
	}
	return true;
}
AccountMap::AccountInfo AccountMap::get(AccountUuid account_uuid){
	PROFILE_ME;

	AccountInfo info = { };
	info.account_uuid = account_uuid;

	const auto it = g_account_map.find<0>(account_uuid);
	if(it == g_account_map.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		return info;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: account_uuid = ", account_uuid);
		return info;
	}
	fill_account_info(info, it->obj);
	return info;
}
AccountMap::AccountInfo AccountMap::require(AccountUuid account_uuid){
	PROFILE_ME;

	auto info = get(account_uuid);
	if(Poseidon::has_none_flags_of(info.flags, FL_VALID)){
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	return info;
}
void AccountMap::get_by_nick(std::vector<AccountMap::AccountInfo> &ret, const std::string &nick){
	PROFILE_ME;

	const auto range = g_account_map.equal_range<1>(nick);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
			continue;
		}
		AccountInfo info;
		fill_account_info(info, it->obj);
		ret.push_back(std::move(info));
	}
}

AccountMap::LoginInfo AccountMap::get_login_info(AccountUuid account_uuid){
	PROFILE_ME;

	LoginInfo info = { };
	info.account_uuid = account_uuid;

	const auto it = g_account_map.find<0>(account_uuid);
	if(it == g_account_map.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		return info;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: account_uuid = ", account_uuid);
		return info;
	}
	fill_login_info(info, it->obj);
	return info;
}
AccountMap::LoginInfo AccountMap::get_login_info(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	LoginInfo info = { };
	info.platform_id = platform_id;
	info.login_name = login_name;

	const auto it = g_account_map.find<2>(LoginKey(platform_id, login_name));
	if(it == g_account_map.end<2>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: platform_id = ", platform_id, ", login_name = ", login_name);
		return info;
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: platform_id = ", platform_id, ", login_name = ", login_name);
		return info;
	}
	fill_login_info(info, it->obj);
	return info;
}

std::pair<AccountUuid, bool> AccountMap::create(PlatformId platform_id, std::string login_name,
	std::string nick, boost::uint64_t flags, std::string remote_ip)
{
	PROFILE_ME;

	auto it = g_account_map.find<2>(LoginKey(platform_id, login_name));
	if(it != g_account_map.end<2>()){
		return std::make_pair(it->account_uuid, false);
	}

	const auto withdrawn = boost::make_shared<bool>(true);

	const auto account_uuid = AccountUuid(Poseidon::Uuid::random());
	Poseidon::add_flags(flags, AccountMap::FL_VALID);
	const auto local_now = Poseidon::get_utc_time();

	LOG_EMPERY_CENTER_INFO("Create account: platform_id = ", platform_id, ", login_name = ", login_name, ", nick = ", nick);
	Poseidon::async_raise_event(boost::make_shared<Events::AccountCreated>(account_uuid, std::move(remote_ip)), withdrawn);

	auto obj = boost::make_shared<MySql::Center_Account>(
		account_uuid.get(), platform_id.get(), std::move(login_name), std::move(nick), flags, std::string(), 0, 0, local_now);
	obj->async_save(true);
	it = g_account_map.insert<2>(it, AccountElement(std::move(obj)));

// FIXME remove this
auto castle = boost::make_shared<Castle>(MapObjectTypeIds::ID_CASTLE, account_uuid, "aaa");

std::vector<ResourceTransactionElement> rsrc;
rsrc.emplace_back(ResourceTransactionElement::OP_ADD, ResourceId(1101001), 1000000000, ReasonId(), 0, 0, 0);
rsrc.emplace_back(ResourceTransactionElement::OP_ADD, ResourceId(1101002), 1000000000, ReasonId(), 0, 0, 0);
rsrc.emplace_back(ResourceTransactionElement::OP_ADD, ResourceId(1101003), 1000000000, ReasonId(), 0, 0, 0);
castle->commit_resource_transaction(rsrc.data(), rsrc.size());

castle->create_building_mission(BuildingBaseId(2),  Castle::MIS_CONSTRUCT, BuildingId(1902001));
castle->create_building_mission(BuildingBaseId(3),  Castle::MIS_CONSTRUCT, BuildingId(1902002));
castle->create_building_mission(BuildingBaseId(4),  Castle::MIS_CONSTRUCT, BuildingId(1902003));
castle->create_building_mission(BuildingBaseId(5),  Castle::MIS_CONSTRUCT, BuildingId(1902004));
castle->create_building_mission(BuildingBaseId(11), Castle::MIS_CONSTRUCT, BuildingId(1904001));
castle->create_building_mission(BuildingBaseId(12), Castle::MIS_CONSTRUCT, BuildingId(1904001));
castle->create_building_mission(BuildingBaseId(13), Castle::MIS_CONSTRUCT, BuildingId(1904001));
castle->create_building_mission(BuildingBaseId(14), Castle::MIS_CONSTRUCT, BuildingId(1904001));

const auto coord = Coord((boost::int32_t)Poseidon::rand32(0, 200) - 100, (boost::int32_t)Poseidon::rand32(0, 200) - 100);
MapObjectMap::update(castle, coord);

set_attribute(account_uuid, 399, castle->get_map_object_uuid().str());

	*withdrawn = false;
	return std::make_pair(account_uuid, true);
}

void AccountMap::set_nick(AccountUuid account_uuid, std::string nick){
	PROFILE_ME;

	const auto it = g_account_map.find<0>(account_uuid);
	if(it == g_account_map.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	g_account_map.set_key<0, 1>(it, nick); // throws std::bad_alloc
	it->obj->set_nick(std::move(nick)); // noexcept

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_AccountAttributes msg;
			msg.account_uuid = account_uuid.str();
			msg.nick = it->obj->unlocked_get_nick(); // 不要用 nick，因为已经 move。
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void AccountMap::set_login_token(AccountUuid account_uuid, std::string login_token, boost::uint64_t expiry_time){
	PROFILE_ME;

	const auto it = g_account_map.find<0>(account_uuid);
	if(it == g_account_map.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_login_token(std::move(login_token)); // noexcept
	it->obj->set_login_token_expiry_time(expiry_time); // noexcept
}
void AccountMap::set_banned_until(AccountUuid account_uuid, boost::uint64_t until){
	PROFILE_ME;

	const auto it = g_account_map.find<0>(account_uuid);
	if(it == g_account_map.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
		LOG_EMPERY_CENTER_DEBUG("Account deleted: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account deleted"));
	}

	it->obj->set_banned_until(until); // noexcept
}
void AccountMap::set_flags(AccountUuid account_uuid, boost::uint64_t flags){
	PROFILE_ME;

	const auto it = g_account_map.find<0>(account_uuid);
	if(it == g_account_map.end<0>()){
		LOG_EMPERY_CENTER_DEBUG("Account not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account not found"));
	}
//	if(Poseidon::has_none_flags_of(it->obj->get_flags(), FL_VALID)){
//		LOG_EMPERY_CENTER_DEBUG("Account deleted: account_uuid = ", account_uuid);
//		DEBUG_THROW(Exception, sslit("Account deleted"));
//	}

	it->obj->set_flags(flags); // noexcept
}

const std::string &AccountMap::get_attribute(AccountUuid account_uuid, unsigned slot){
	PROFILE_ME;

	const auto it = g_attribute_map.find<0>(std::make_pair(account_uuid, slot));
	if(it == g_attribute_map.end<0>()){
		LOG_EMPERY_CENTER_TRACE("Account attribute not found: account_uuid = ", account_uuid, ", slot = ", slot);
		return Poseidon::EMPTY_STRING;
	}
	return it->obj->unlocked_get_value();
}
void AccountMap::get_attributes(boost::container::flat_map<unsigned, std::string> &ret,
	AccountUuid account_uuid, unsigned begin_slot, unsigned end_slot)
{
	PROFILE_ME;

	const auto begin = g_attribute_map.lower_bound<0>(std::make_pair(account_uuid, begin_slot));
	const auto end   = g_attribute_map.upper_bound<0>(std::make_pair(account_uuid, end_slot));
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, end)));
	for(auto it = begin; it != end; ++it){
		ret.emplace(it->obj->get_slot(), it->obj->unlocked_get_value());
	}
}
void AccountMap::touch_attribute(AccountUuid account_uuid, unsigned slot){
	PROFILE_ME;

	auto it = g_attribute_map.find<0>(std::make_pair(account_uuid, slot));
	if(it == g_attribute_map.end<0>()){
		it = g_attribute_map.insert<0>(it, AccountAttributeElement(
			boost::make_shared<MySql::Center_AccountAttribute>(account_uuid.get(), slot, std::string())));
		it->obj->async_save(true);
	}
}
void AccountMap::set_attribute(AccountUuid account_uuid, unsigned slot, std::string value){
	PROFILE_ME;

	auto it = g_attribute_map.find<0>(std::make_pair(account_uuid, slot));
	if(it == g_attribute_map.end<0>()){
		it = g_attribute_map.insert<0>(it, AccountAttributeElement(
			boost::make_shared<MySql::Center_AccountAttribute>(account_uuid.get(), slot, std::move(value))));
		it->obj->async_save(true);
	} else {
		it->obj->set_value(std::move(value));
	}

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_AccountAttributes msg;
			msg.account_uuid = account_uuid.str();
			msg.attributes.emplace_back();
			auto &attribute = msg.attributes.back();
			attribute.slot = slot;
			attribute.value = it->obj->unlocked_get_value(); // 不要用 value，因为已经 move。
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

void AccountMap::send_attributes_to_client(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
	bool wants_nick, bool wants_attributes, bool wants_private_attributes, bool wants_items)
{
	PROFILE_ME;

	Msg::SC_AccountAttributes msg;
	msg.account_uuid = account_uuid.str();
	if(wants_nick){
		auto info = AccountMap::require(account_uuid);
		msg.nick = std::move(info.nick);
	}
	if(wants_attributes){
		unsigned slot_end = AccountMap::ATTR_PUBLIC_END;
		if(wants_private_attributes){
			slot_end = AccountMap::ATTR_END;
		}
		boost::container::flat_map<unsigned, std::string> attributes;
		AccountMap::get_attributes(attributes, account_uuid, 0, slot_end);
		msg.attributes.reserve(attributes.size());
		for(auto it = attributes.begin(); it != attributes.end(); ++it){
			msg.attributes.emplace_back();
			auto &attribute = msg.attributes.back();
			attribute.slot = it->first;
			attribute.value = std::move(it->second);
		}
	}
	if(wants_items){
		// TODO 添加公开资源。
	}
	session->send(msg);
}
void AccountMap::combined_send_attributes_to_client(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<boost::uint64_t>("account_info_cache_timeout", 0);

	g_info_timestamp_map->erase<0>(g_info_timestamp_map->begin<0>(), g_info_timestamp_map->upper_bound<0>(now));

	if(g_info_timestamp_map->find<1>(std::make_pair(account_uuid, session)) != g_info_timestamp_map->end<1>()){
		LOG_EMPERY_CENTER_DEBUG("Cache hit: account_uuid = ", account_uuid);
		return;
	}
	send_attributes_to_client(account_uuid, session, true, true, false, true);
	g_info_timestamp_map->insert(InfoTimestampElement(now + cache_timeout, account_uuid, session));
}

}
