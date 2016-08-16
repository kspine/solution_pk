#include "../precompiled.hpp"
#include "legion_map.hpp"
#include "../mmain.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/singletons/timer_daemon.hpp>
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <tuple>
#include "player_session_map.hpp"
#include "../mysql/legion.hpp"
#include "../msg/sc_legion.hpp"
#include "../player_session.hpp"
#include "../account.hpp"
#include "../legion.hpp"
#include "../legion_attribute_ids.hpp"
#include "../string_utilities.hpp"
#include "controller_client.hpp"
#include "../msg/err_legion.hpp"
#include "legion_member_map.hpp"
#include "../legion_member.hpp"
#include "../account_attribute_ids.hpp"
#include "../legion_member_attribute_ids.hpp"
#include "account_map.hpp"
#include "../data/legion_corps_power.hpp"
#include "../data/global.hpp"
#include "../chat_message_type_ids.hpp"

namespace EmperyCenter {

namespace {
	struct LegionMemberElement {
		boost::shared_ptr<LegionMember> member;

		LegionUuid legion_uuid;
		AccountUuid account_uuid;

		explicit LegionMemberElement(boost::shared_ptr<LegionMember> member_)
			: member(std::move(member_))
			, legion_uuid(member->get_legion_uuid())
			, account_uuid(member->get_account_uuid())
		{
		}
	};

	MULTI_INDEX_MAP(LegionMemberContainer, LegionMemberElement,
		MULTI_MEMBER_INDEX(legion_uuid)
		MULTI_MEMBER_INDEX(account_uuid)
	)

	boost::shared_ptr<LegionMemberContainer> g_legionmember_map;

	enum CacheType : std::uint64_t {
		CT_NONE       = 0, // Silent the warning.
		CT_QUIT       = 1,
		CT_DISBAND    = 2,
	};

	struct InfoCacheElement {
		std::tuple<LegionUuid, boost::weak_ptr<LegionMember>, CacheType> key;
		std::uint64_t expiry_time;

		InfoCacheElement(LegionUuid legion_uuid_, const boost::shared_ptr<LegionMember> &member_, CacheType type_,
			std::uint64_t expiry_time_)
			: key(legion_uuid_, member_, type_), expiry_time(expiry_time_)
		{
		}
	};

	MULTI_INDEX_MAP(InfoCacheContainer, InfoCacheElement,
		UNIQUE_MEMBER_INDEX(key)
		MULTI_MEMBER_INDEX(expiry_time)
	)

	boost::weak_ptr<InfoCacheContainer> g_info_cache_map;

	void gc_member_proc(std::uint64_t now){
		PROFILE_ME;
		LOG_EMPERY_CENTER_ERROR("legion member map gc timer: now =================================================== ", now);

		const auto &account_map = g_legionmember_map;
		if(!account_map){
			return;
		}

		for(auto it = account_map->begin(); it != account_map->end(); ++it)
		{
			const auto &member = it->member;

			if(LegionMemberMap::check_in_waittime(member))
				break;


			// 查看member权限
		//

		}
	}


	MODULE_RAII_PRIORITY(handles, 5000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		// LegionMember
		struct TempLegionMemberElement {
			boost::shared_ptr<MySql::Center_Legion_Member> obj;
			std::vector<boost::shared_ptr<MySql::Center_LegionMemberAttribute>> attributes;
			std::vector<boost::shared_ptr<LegionMember>> members;
		};
		std::map<AccountUuid, TempLegionMemberElement> temp_account_map;


		LOG_EMPERY_CENTER_INFO("Loading LegionMembers...");
		conn->execute_sql("SELECT * FROM `Center_Legion_Member`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_Legion_Member>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto accout_uuid = AccountUuid(obj->get_account_uuid());
			temp_account_map[accout_uuid].obj = std::move(obj);
		}
		LOG_EMPERY_CENTER_INFO("Loaded ", temp_account_map.size(), " LegionMember(s).");

		LOG_EMPERY_CENTER_INFO("Done loading Legion members.",temp_account_map.size());

	//	sleep(5000);

		LOG_EMPERY_CENTER_INFO("Loading LegionMember attributes...");
		conn->execute_sql("SELECT * FROM `Center_LegionMemberAttribute`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_LegionMemberAttribute>();
			obj->fetch(conn);
			const auto legion_uuid = AccountUuid(obj->unlocked_get_account_uuid());
			const auto it = temp_account_map.find(legion_uuid);
			if(it == temp_account_map.end()){
				continue;
			}
			obj->enable_auto_saving();
			it->second.attributes.emplace_back(std::move(obj));
		}

		LOG_EMPERY_CENTER_INFO("Done loading LegionMember attributes.");

		const auto legionmember_map = boost::make_shared<LegionMemberContainer>();
		for(auto it = temp_account_map.begin(); it != temp_account_map.end(); ++it){
			auto member = boost::make_shared<LegionMember>(std::move(it->second.obj), it->second.attributes);

			legionmember_map->insert(LegionMemberElement(std::move(member)));
		}
		g_legionmember_map = legionmember_map;
		handles.push(legionmember_map);

		const auto info_cache_map = boost::make_shared<InfoCacheContainer>();
		g_info_cache_map = info_cache_map;
		handles.push(info_cache_map);

	//	const auto gc_interval = get_config<std::uint64_t>("object_gc_interval", 10*1000);
		auto timer = Poseidon::TimerDaemon::register_timer(0, 100*1000,
			std::bind(&gc_member_proc, std::placeholders::_2));
		handles.push(timer);
	}

	boost::shared_ptr<LegionMember> reload_account_aux(boost::shared_ptr<MySql::Center_Legion_Member> obj){
		PROFILE_ME;

		std::deque<boost::shared_ptr<const Poseidon::JobPromise>> promises;

		const auto attributes = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_LegionMemberAttribute>>>();

#define RELOAD_PART_(sink_, table_)	\
		{	\
			std::ostringstream oss;	\
			const auto account_uuid = obj->unlocked_get_account_uuid();	\
			oss <<"SELECT * FROM `" #table_ "` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid);	\
			auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(	\
				[sink_](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){	\
					auto obj = boost::make_shared<MySql:: table_ >();	\
					obj->fetch(conn);	\
					obj->enable_auto_saving();	\
					(sink_)->emplace_back(std::move(obj));	\
				}, #table_, oss.str());	\
			promises.emplace_back(std::move(promise));	\
		}
//=============================================================================
	//	RELOAD_PART_(attributes,         Center_LegionMemberAttribute)
//=============================================================================
		for(const auto &promise : promises){
			Poseidon::JobDispatcher::yield(promise, true);
		}
#undef RELOAD_PART_

		return boost::make_shared<LegionMember>(std::move(obj), *attributes);
	}

	template<typename IteratorT>
	void really_append_account(std::vector<boost::shared_ptr<LegionMember>> &ret,
		const std::pair<IteratorT, IteratorT> &range, std::uint64_t begin, std::uint64_t count)
	{
		PROFILE_ME;

		auto lower = range.first;
		for(std::uint64_t i = 0; i < begin; ++i){
			if(lower == range.second){
				break;
			}
			++lower;
		}

		auto upper = lower;
		for(std::uint64_t i = 0; i < count; ++i){
			if(upper == range.second){
				break;
			}
			++upper;
		}

		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(lower, upper)));
		for(auto it = lower; it != upper; ++it){
			ret.emplace_back(it->member);
		}
	}

	void synchronize_account_and_update_cache(std::uint64_t now, std::uint64_t cache_timeout,
		const boost::shared_ptr<LegionMember> &account, const boost::shared_ptr<ItemBox> &item_box,
		const boost::shared_ptr<PlayerSession> &session, std::uint64_t flags)
	try {
		PROFILE_ME;

		if(flags == 0){
			return;
		}

/*
		const auto account_uuid = account->get_account_uuid();


		Msg::SC_LegionMemberAttributes msg;
		msg.account_uuid = account_uuid.str();

		// nick
		if(flags & CT_NICK){
			msg.nick = account->get_nick();
		}

		// attributes
		boost::container::flat_map<LegionMemberAttributeId, std::string> attributes;
		auto public_end = attributes.end();

		const auto fetch_attributes = [&]{
			if(attributes.empty()){
				account->get_attributes(attributes);
				msg.attributes.reserve(attributes.size());
				public_end = attributes.upper_bound(LegionMemberAttributeIds::ID_PUBLIC_END);
			}
		};
		const auto copy_attributes  = [&](decltype(attributes.cbegin()) begin, decltype(attributes.cbegin()) end){
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
			copy_attributes(public_end, attributes.upper_bound(LegionMemberAttributeIds::ID_END));
		}

		// items
		if(flags & CT_ITEMS){
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

		const auto info_cache_map = g_info_cache_map.lock();
		if(info_cache_map){
			const auto update_cache = [&](CacheType type){
				if(cache_timeout != 0){
					auto elem = InfoCacheElement(account_uuid, session, type, saturated_add(now, cache_timeout));
					auto it = info_cache_map->find<0>(elem.key);
					if(it == info_cache_map->end<0>()){
						info_cache_map->insert<0>(std::move(elem));
					} else {
						info_cache_map->replace<0>(it, std::move(elem));
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
		}

		*/
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		session->shutdown(e.what());
	}

}

bool LegionMemberMap::is_holding_controller_token(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return false;
	}

	/*
	const auto controller = ControllerClient::require();

	Msg::ST_LegionMemberQueryToken treq;
	treq.account_uuid = account_uuid.str();
	auto tresult = controller->send_and_wait(treq);
	LOG_EMPERY_CENTER_DEBUG("Controller response: code = ", tresult.first, ", msg = ", tresult.second);
	if(tresult.first != 0){
		return false;
	}
	*/
	return true;
}
void LegionMemberMap::require_controller_token(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		DEBUG_THROW(Exception, sslit("legion map not loaded"));
	}

	const auto controller = ControllerClient::require();

	/*
	Msg::ST_LegionMemberAcquireToken treq;
	treq.account_uuid = account_uuid.str();
	auto tresult = controller->send_and_wait(treq);
	LOG_EMPERY_CENTER_DEBUG("Controller response: code = ", tresult.first, ", msg = ", tresult.second);
	if(tresult.first == Msg::ERR_INVALIDATION_IN_PROGRESS){
		const auto wait_delay = get_config<std::uint64_t>("account_invalidation_wait_delay", 15000);

		const auto promise = boost::make_shared<Poseidon::JobPromise>();
		const auto timer = Poseidon::TimerDaemon::register_timer(wait_delay, 0, std::bind([=]() mutable { promise->set_success(); }));
		LOG_EMPERY_CENTER_DEBUG("Waiting for account invalidation...");
		Poseidon::JobDispatcher::yield(promise, true);

		tresult = controller->send_and_wait(treq);
	}
	if(tresult.first != 0){
		LOG_EMPERY_CENTER_INFO("Failed to acquire controller token: code = ", tresult.first, ", msg = ", tresult.second);
		DEBUG_THROW(Exception, sslit("Failed to acquire controller token"));
	}

	*/
}

boost::shared_ptr<LegionMember> LegionMemberMap::get(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto it = account_map->find<0>(account_uuid);
	if(it == account_map->end<0>()){
		LOG_EMPERY_CENTER_TRACE("legion not found: account_uuid = ", account_uuid);
		return { };
	}
	return it->member;
}
boost::shared_ptr<LegionMember> LegionMemberMap::require(LegionUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("LegionMember not found: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("LegionMember not found"));
	}
	return account;
}

boost::shared_ptr<LegionMember> LegionMemberMap::get_by_account_uuid(AccountUuid account_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto it = account_map->find<1>(account_uuid);
	if(it == account_map->end<1>()){
		LOG_EMPERY_CENTER_TRACE("legion not found: account_uuid = ", account_uuid);
		return { };
	}
	return it->member;
}

void LegionMemberMap::get_member_by_titleid(std::vector<boost::shared_ptr<LegionMember>> &ret, LegionUuid legion_uuid,std::uint64_t level)
{
	PROFILE_ME;
	std::vector<boost::shared_ptr<LegionMember>> members;
	get_by_legion_uuid(members,legion_uuid);
	ret.reserve(ret.size() + members.size());
	for(auto it = members.begin(); it != members.end(); ++it)
	{
		if(boost::lexical_cast<uint>((*it)->get_attribute(LegionMemberAttributeIds::ID_TITLEID)) == level)
		{
			ret.reserve(ret.size() + 1);
			ret.emplace_back(*it);
		}
	}
}

void LegionMemberMap::get_by_legion_uuid(std::vector<boost::shared_ptr<LegionMember>> &ret, LegionUuid legion_uuid)
{
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Legion map not loaded.");
		return;
	}

	const auto range = account_map->equal_range<0>(legion_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->member);
	}
}
/*
boost::shared_ptr<LegionMember> LegionMemberMap::get_or_reload(LegionUuid account_uuid){
	PROFILE_ME;

	auto account = get(account_uuid);
	if(!account){
		account = forced_reload(account_uuid);
	}
	return account;
}
*/
/*
boost::shared_ptr<LegionMember> LegionMemberMap::forced_reload(LegionUuid account_uuid){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("legion map not loaded.");
		return { };
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_LegionMember>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_LegionMember` WHERE `account_uuid` = " <<Poseidon::MySql::UuidFormatter(account_uuid.get());
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_LegionMember>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_LegionMember", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_DEBUG("LegionMember not found in database: account_uuid = ", account_uuid);
		return { };
	}

	auto account = reload_account_aux(std::move(sink->front()));

	const auto elem = LegionMemberElement(account);
	const auto result = account_map->insert(elem);
	if(!result.second){
		account_map->replace(result.first, elem);
	}

	LOG_EMPERY_CENTER_DEBUG("Successfully reloaded account: account_uuid = ", account_uuid);
	return std::move(account);

}
*/
/*
boost::shared_ptr<LegionMember> LegionMemberMap::get_or_reload_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Center_LegionMember map not loaded.");
		return { };
	}


	const auto key = std::make_pair(platform_id, hash_string_nocase(login_name));
	const auto range = account_map->equal_range<1>(key);
	for(auto it = range.first; it != range.second; ++it){
		if(are_strings_equal_nocase(it->account->get_login_name(), login_name)){
			return it->account;
		}
	}

	LOG_EMPERY_CENTER_DEBUG("Login name not found. Reloading: platform_id = ", platform_id, ", login_name = ", login_name);

	return forced_reload_by_login_name(platform_id, login_name);

}

*/
/*
boost::shared_ptr<LegionMember> LegionMemberMap::forced_reload_by_login_name(PlatformId platform_id, const std::string &login_name){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Center_LegionMember map not loaded.");
		return { };
	}

	const auto sink = boost::make_shared<std::vector<boost::shared_ptr<MySql::Center_Legion_Member>>>();
	{
		std::ostringstream oss;
		oss <<"SELECT * FROM `Center_LegionMember` WHERE `platform_id` = " <<platform_id
		    <<" AND `login_name` = " <<Poseidon::MySql::StringEscaper(login_name);
		const auto promise = Poseidon::MySqlDaemon::enqueue_for_batch_loading(
			[sink](const boost::shared_ptr<Poseidon::MySql::Connection> &conn){
				auto obj = boost::make_shared<MySql::Center_Legion_Member>();
				obj->fetch(conn);
				obj->enable_auto_saving();
				sink->emplace_back(std::move(obj));
			}, "Center_LegionMember", oss.str());
		Poseidon::JobDispatcher::yield(promise, true);
	}
	if(sink->empty()){
		LOG_EMPERY_CENTER_DEBUG("LegionMember not found in database: platform_id = ", platform_id, ", login_name = ", login_name);
		return { };
	}

	auto account = reload_account_aux(std::move(sink->front()));
	const auto account_uuid = account->get_account_uuid();

	const auto elem = LegionMemberElement(account);
	const auto result = account_map->insert(elem);
	if(!result.second){
		account_map->replace(result.first, elem);
	}

	LOG_EMPERY_CENTER_DEBUG("Successfully reloaded account: account_uuid = ", account_uuid,
		", platform_id = ", platform_id, ", login_name = ", login_name);
	return std::move(account);
}
*/
std::uint64_t LegionMemberMap::get_count(){
	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("Center_LegionMember map not loaded.");
		return 0;
	}

	return account_map->size();
}

std::uint64_t LegionMemberMap::get_legion_member_count(LegionUuid legion_uuid)
{
	std::vector<boost::shared_ptr<LegionMember>> members;
	get_by_legion_uuid(members, legion_uuid);

	return members.size();

}

void LegionMemberMap::get_all(std::vector<boost::shared_ptr<LegionMember>> &ret, std::uint64_t begin, std::uint64_t count){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		return;
	}

	really_append_account(ret,
		std::make_pair(account_map->begin<0>(), account_map->end<0>()),
		begin, count);
}
void LegionMemberMap::get_banned(std::vector<boost::shared_ptr<LegionMember>> &ret, std::uint64_t begin, std::uint64_t count){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		return;
	}
/*
	really_append_account(ret,
		std::make_pair(account_map->upper_bound<4>(0), account_map->end<4>()),
		begin, count);

		*/
}
void LegionMemberMap::get_quieted(std::vector<boost::shared_ptr<LegionMember>> &ret, std::uint64_t begin, std::uint64_t count){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		return;
	}

	/*
	really_append_account(ret,
		std::make_pair(account_map->upper_bound<5>(0), account_map->end<5>()),
		begin, count);

		*/
}
void LegionMemberMap::get_by_nick(std::vector<boost::shared_ptr<LegionMember>> &ret, const std::string &nick){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		return;
	}

	/*
	const auto key = hash_string_nocase(nick);
	const auto range = account_map->equal_range<1>(key);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		if(!are_strings_equal_nocase(it->account->get_nick(), nick)){
			continue;
		}
		ret.emplace_back(it->account);
	}
	*/
}
void LegionMemberMap::get_by_referrer(std::vector<boost::shared_ptr<LegionMember>> &ret, AccountUuid referrer_uuid){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		return;
	}
	/*
	auto range = account_map->equal_range<2>(referrer_uuid);
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
	for(auto it = range.first; it != range.second; ++it){
		ret.emplace_back(it->account);
	}
	*/
}

void LegionMemberMap::insert(const boost::shared_ptr<LegionMember> &legion, const std::string &remote_ip){
	PROFILE_ME;

	const auto &legion_map = g_legionmember_map;
	if(!legion_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		DEBUG_THROW(Exception, sslit("LegionMember map not loaded"));
	}


	const auto account_uuid = legion->get_account_uuid();

	// 判断是否已存在
	const auto member = get_by_account_uuid(account_uuid);
	if(member)
	{
		LOG_EMPERY_CENTER_WARNING("LegionMember already exists: account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("LegionMember already exists"));
	}
	else
	{
		legion_map->insert(LegionMemberElement(legion));

		LOG_EMPERY_CENTER_DEBUG("Inserting legion: account_uuid = ", account_uuid);
	}

	LOG_EMPERY_CENTER_DEBUG("Inserting LegionMemberMap size = ", get_legion_member_count(legion->get_legion_uuid()));

}

void LegionMemberMap::deletemember(AccountUuid account_uuid,bool bdeletemap)
{
	// 先从内存中删，然后删数据库的
	PROFILE_ME;

	const auto &legion_map = g_legionmember_map;
	if(!legion_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		DEBUG_THROW(Exception, sslit("LegionMember map not loaded"));
	}

	const auto it = legion_map->find<1>(account_uuid);
	if(it != legion_map->end<1>()){

		const auto legion_uuid = it->member->get_legion_uuid();

		const auto donate = it->member->get_attribute(LegionMemberAttributeIds::ID_DONATE);

		const auto weekdonate = it->member->get_attribute(LegionMemberAttributeIds::ID_WEEKDONATE);

		it->member->leave();

		if(bdeletemap)
			legion_map->erase<1>(it);

		LOG_EMPERY_CENTER_INFO("deletemember members size==============================================",get_legion_member_count(legion_uuid));

		// 从数据库中删除该成员
		std::string strsql = "DELETE FROM Center_Legion_Member WHERE account_uuid='";
		strsql += account_uuid.str();
		strsql += "';";

		Poseidon::MySqlDaemon::enqueue_for_batch_saving("Center_Legion_Member",strsql);


		// 个人贡献移动到账号身上
		const auto account = AccountMap::require(account_uuid);

		boost::container::flat_map<AccountAttributeId, std::string> Attributes;

		Attributes[AccountAttributeIds::ID_DONATE] = donate;
		Attributes[AccountAttributeIds::ID_WEEKDONATE] = weekdonate;

		account->set_attributes(std::move(Attributes));
	}


}


void LegionMemberMap::disband_legion(LegionUuid legion_uuid)
{
	std::vector<boost::shared_ptr<LegionMember>> members;
	get_by_legion_uuid(members, legion_uuid);

	for(auto it = members.begin(); it != members.end();)
	{
		const auto member = *it;
		deletemember(member->get_account_uuid(),false);
		it = members.erase(it);
	}

}

void LegionMemberMap::update(const boost::shared_ptr<LegionMember> &account, bool throws_if_not_exists){
	PROFILE_ME;

	const auto &account_map = g_legionmember_map;
	if(!account_map){
		LOG_EMPERY_CENTER_WARNING("LegionMember map not loaded.");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("LegionMember map not loaded"));
		}
		return;
	}


	const auto account_uuid = account->get_account_uuid();

	const auto it = account_map->find<1>(account_uuid);
	if(it == account_map->end<1>()){
		LOG_EMPERY_CENTER_WARNING("LegionMember not found: account_uuid = ", account_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("LegionMember not found"));
		}
		return;
	}
	if(it->member != account){
		LOG_EMPERY_CENTER_DEBUG("LegionMember expired: account_uuid = ", account_uuid);
		return;
	}

	LOG_EMPERY_CENTER_DEBUG("Updating LegionMember: account_uuid = ", account_uuid);
	account_map->replace<1>(it, LegionMemberElement(account));

	/*
	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			account->synchronize_with_player(session);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
	*/
}


bool LegionMemberMap::is_in_same_legion(AccountUuid account_uuid,AccountUuid other_uuid)
{
	const auto member = get_by_account_uuid(account_uuid);
	const auto member2 = get_by_account_uuid(other_uuid);
	if(member && member2 && member->get_legion_uuid() == member2->get_legion_uuid())
	{
		return true;
	}
	return false;
}

bool LegionMemberMap::check_in_waittime(const boost::shared_ptr<LegionMember> &member)
{

	// 查看member权限
	bool bdelete = false;
	// 根据account_uuid查找是否有军团
	const auto legion = LegionMap::get(LegionUuid(member->get_legion_uuid()));
	if(legion)
	{
		const auto account = AccountMap::get(member->get_account_uuid());
		const auto titileid = member->get_attribute(LegionMemberAttributeIds::ID_TITLEID);
		LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage titileid ================================ ",titileid,"  成员uuid：",member->get_account_uuid());
		if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_QUIT))
		{
	//		LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage 可以退出================================ ");
			// 是否已经在退会等待中、被踢出等待中
			auto quittime = member->get_attribute(LegionMemberAttributeIds::ID_QUITWAITTIME);
	//		LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage quittime= 111111111=============================== ",quittime);
			bool bkick = false;
			if(quittime.empty()  || quittime == Poseidon::EMPTY_STRING)
			{
				// 被踢出等待中
				quittime = member->get_attribute(LegionMemberAttributeIds::ID_KICKWAITTIME);
				bkick = true;
			}
	//		LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage quittime= 2222222=============================== ",quittime);
			if(quittime != Poseidon::EMPTY_STRING)
			{
				const auto utc_now = Poseidon::get_utc_time();

				const auto leavetime = boost::lexical_cast<boost::uint64_t>(quittime) * 1000; 

				LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage utc_now ================ ",utc_now," quittime==============",quittime, " leavetime==============",leavetime);

				if(utc_now > leavetime )
				{
					// 已经过了完全离开的时间，让玩家离开军团
					const auto legion_uuid = member->get_legion_uuid();
					const auto account_uuid = member->get_account_uuid();

					LegionMemberMap::deletemember(member->get_account_uuid());

					const auto target_account = AccountMap::get(account_uuid);
					if(target_account)
					{
						// 广播给军团其他成员
						Msg::SC_LegionNoticeMsg msg;
						if(bkick)
						{
							// 被踢出时发邮件
							legion->sendmail(target_account,ChatMessageTypeIds::ID_LEVEL_LEGION_KICK,legion->get_nick() + ","+ account->get_nick());
							msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_KICK;
						}
						else
							msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_QUIT;
						msg.nick = target_account->get_nick();
						msg.ext1 = "";
						legion->sendNoticeMsg(msg);
					}


					LOG_EMPERY_CENTER_INFO("CS_QuitLegionReqMessage members size==============================================",LegionMemberMap::get_legion_member_count(legion_uuid));
					bdelete = true;
			//		break;
				}
			}
		}

		// 看下是否转让军团长等待时间已过的逻辑
		bool bAttorn = false;

		if(Data::LegionCorpsPower::is_have_power(LegionCorpsPowerId(boost::lexical_cast<uint32_t>(titileid)),Legion::LEGION_POWER::LEGION_POWER_ATTORN))
		{
			const auto utc_now = Poseidon::get_utc_time();
		//	LOG_EMPERY_CENTER_DEBUG("有转让权限 titileid ================================ ",titileid,"  成员uuid：", member->get_account_uuid());
			// 转让军团等待中
			const auto 	quittime = legion->get_attribute(LegionAttributeIds::ID_ATTORNTIME);
			if(!quittime.empty() ||  quittime != Poseidon::EMPTY_STRING)
			{
				// 先看下要转让的目标对象是否还在军团中
				const auto target_uuid = legion->get_attribute(LegionAttributeIds::ID_ATTORNLEADER);
		//		LOG_EMPERY_CENTER_DEBUG("存在转让等待时间=============================== ",target_uuid);
				// 查看两个人是否属于同一军团
				if(LegionMemberMap::is_in_same_legion(member->get_account_uuid(),AccountUuid(target_uuid)))
				{
					const auto target_member =  LegionMemberMap::get_by_account_uuid(AccountUuid(target_uuid));
					// 设置两者新的等级关系
					if(target_member)
					{
						// 查看目标对象是否有离开倒计时或者被踢出的倒计时
						auto target_quittime = target_member->get_attribute(LegionMemberAttributeIds::ID_QUITWAITTIME);
	//					LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage quittime= 111111111=============================== ",target_quittime);
						if(target_quittime.empty()  || target_quittime == Poseidon::EMPTY_STRING)
						{
							// 被踢出等待中
							target_quittime = target_member->get_attribute(LegionMemberAttributeIds::ID_KICKWAITTIME);
						}
	//					LOG_EMPERY_CENTER_DEBUG("CS_GetLegionBaseInfoMessage quittime= 2222222=============================== ",target_quittime);
						if(!target_quittime.empty() || target_quittime != Poseidon::EMPTY_STRING)
						{
							// 因为目标对象要退出军团，所以无法转让
							LOG_EMPERY_CENTER_DEBUG("因为目标对象要退出军团，所以无法转让=============================== ");
							bAttorn = true;
						}
						else
						{
							// 查看时间是否已经超过转让等待时间
							const auto leavetime = boost::lexical_cast<boost::uint64_t>(quittime) * 1000;
							if(utc_now > leavetime )
							{
								// 设置各自的等级
								boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes;

								Attributes[LegionMemberAttributeIds::ID_TITLEID] = "1";

								target_member->set_attributes(Attributes);

								// 设置原来团长为团员
								boost::container::flat_map<LegionMemberAttributeId, std::string> Attributes1;

								Attributes1[LegionMemberAttributeIds::ID_TITLEID] = boost::lexical_cast<std::string>(Data::Global::as_unsigned(Data::Global::SLOT_LEGION_MEMBER_DEFAULT_POWERID));

								member->set_attributes(Attributes1);

								// 广播给军团其他成员
								const auto target_account = AccountMap::get(target_member->get_account_uuid());
								if(target_account && account)
								{
									Msg::SC_LegionNoticeMsg msg;
									msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_ATTORN;
									msg.nick = target_account->get_nick();
									msg.ext1 = account->get_nick();
									legion->sendNoticeMsg(msg);
								}

								LOG_EMPERY_CENTER_DEBUG("成功转让=============================== ",member->get_account_uuid(), "  目标对象：",target_member->get_account_uuid());
								// 转让成功，重置转让等待时间
								bAttorn = true;

								bdelete = true;
							}
						}
					}
					else
					{
						// 目标不存在无法转让
						LOG_EMPERY_CENTER_DEBUG("没找到目标成员，所以无法转让=============================== ");
						bAttorn = true;
					}

				}
				else
				{
					// 要转让的目标已经和自己不属于同一军团了
					LOG_EMPERY_CENTER_DEBUG("要转让的目标已经和自己不属于同一军团了，所以无法转让=============================== ");
					bAttorn = true;
				}
			}
		}

		if(bAttorn)
		{
			// 重置转让信息
			boost::container::flat_map<LegionAttributeId, std::string> Attributes;

			Attributes[LegionAttributeIds::ID_ATTORNTIME] = "";
			Attributes[LegionAttributeIds::ID_ATTORNLEADER] = "";

			legion->set_attributes(Attributes);

	//		break;
		}
	}

	return bdelete;
}

void LegionMemberMap::synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
	bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box)
{
	PROFILE_ME;

	const auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("LegionMember not found: account_uuid = ", account_uuid);
		return;
	}

	/*
	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<std::uint64_t>("account_info_cache_timeout", 0);


	const auto info_cache_map = g_info_cache_map.lock();
	if(info_cache_map){
		info_cache_map->erase<1>(info_cache_map->begin<1>(), info_cache_map->upper_bound<1>(now));
	}

	synchronize_account_and_update_cache(now, cache_timeout, account, item_box, session,
		(wants_nick               ? CT_NICK       : CT_NONE) |
		(wants_attributes         ? CT_ATTRS      : CT_NONE) |
		(wants_private_attributes ? CT_PRIV_ATTRS : CT_NONE) |
		(item_box                 ? CT_ITEMS      : CT_NONE));
		*/
}

void LegionMemberMap::cached_synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session,
	bool wants_nick, bool wants_attributes, bool wants_private_attributes, const boost::shared_ptr<ItemBox> &item_box)
{
	PROFILE_ME;

	const auto account = get(account_uuid);
	if(!account){
		LOG_EMPERY_CENTER_WARNING("LegionMember not found: account_uuid = ", account_uuid);
		return;
	}

	/*
	const auto now = Poseidon::get_fast_mono_clock();
	const auto cache_timeout = get_config<std::uint64_t>("account_info_cache_timeout", 0);

	const auto info_cache_map = g_info_cache_map.lock();
	if(info_cache_map){
		info_cache_map->erase<1>(info_cache_map->begin<1>(), info_cache_map->upper_bound<1>(now));
	}

	const auto is_miss = [&](CacheType type){
		if(!info_cache_map){
			return true;
		}
		auto elem = InfoCacheElement(account_uuid, session, type, 0);
		const auto it = info_cache_map->find<0>(elem.key);
		if(it == info_cache_map->end<0>()){
			return true;
		}
		return false;
	};

	synchronize_account_and_update_cache(now, cache_timeout, account, item_box, session,
		(wants_nick               && is_miss(CT_NICK      ) ? CT_NICK       : CT_NONE) |
		(wants_attributes         && is_miss(CT_ATTRS     ) ? CT_ATTRS      : CT_NONE) |
		(wants_private_attributes && is_miss(CT_PRIV_ATTRS) ? CT_PRIV_ATTRS : CT_NONE) |
		(item_box                 && is_miss(CT_ITEMS     ) ? CT_ITEMS      : CT_NONE));

		*/
}
void LegionMemberMap::cached_synchronize_account_with_player_all(LegionUuid account_uuid, const boost::shared_ptr<PlayerSession> &session){
	return cached_synchronize_account_with_player_all(account_uuid, session, true, true, false, { });
}

}
