#include "precompiled.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "msg/sd_dungeon.hpp"
#include "dungeon_session.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "msg/sc_dungeon.hpp"
#include "src/dungeon_object.hpp"
#include "dungeon_buff.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include "events/dungeon.hpp"

namespace EmperyCenter {

namespace {
	void fill_scope_msg(Msg::SC_DungeonSetScope &msg, DungeonUuid dungeon_uuid, Rectangle scope){
		PROFILE_ME;

		msg.dungeon_uuid = dungeon_uuid.str();
		msg.x            = scope.left();
		msg.y            = scope.bottom();
		msg.width        = scope.width();
		msg.height       = scope.height();
	}
	void fill_suspension_msg(Msg::SC_DungeonWaitForPlayerConfirmation &msg, DungeonUuid dungeon_uuid, const Dungeon::Suspension &susp){
		PROFILE_ME;

		msg.dungeon_uuid = dungeon_uuid.str();
		msg.context      = susp.context;
		msg.type         = susp.type;
		msg.param1       = susp.param1;
		msg.param2       = susp.param2;
		msg.param3       = susp.param3;
	}
}

Dungeon::Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id, const boost::shared_ptr<DungeonSession> &server,
	AccountUuid founder_uuid, std::uint64_t create_time, std::uint64_t expiry_time,std::uint64_t finish_count)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_type_id(dungeon_type_id), m_server(server)
	, m_founder_uuid(founder_uuid), m_create_time(create_time), m_expiry_time(expiry_time)
	, m_finish_count(finish_count), m_begin(false)
{
	try {
		Msg::SD_DungeonCreate msg;
		msg.dungeon_uuid    = get_dungeon_uuid().str();
		msg.dungeon_type_id = get_dungeon_type_id().get();
		msg.founder_uuid    = get_founder_uuid().str();
		msg.finish_count    = m_finish_count;
		msg.expiry_time     = m_expiry_time;
		server->send(msg);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		server->shutdown(e.what());
	}
}
Dungeon::~Dungeon(){
	const auto server = m_server.lock();
	if(server){
		try {
			Msg::SD_DungeonDestroy msg;
			msg.dungeon_uuid    = get_dungeon_uuid().str();
			server->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			server->shutdown(e.what());
		}
	}
	clear_observers(Q_DUNGEON_EXPIRED, "");
	/*
	auto event = boost::make_shared<Events::DungeonDeleted>(get_founder_uuid(),get_dungeon_type_id());
	Poseidon::async_raise_event(event);
	*/
}

/*
void Dungeon::synchronize_with_all_observers(const boost::shared_ptr<DungeonObject> &dungeon_object) const noexcept {
	PROFILE_ME;

	for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
		const auto session = it->second.session.lock();
		if(session){
			try {
				dungeon_object->synchronize_with_player(session);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}
void Dungeon::synchronize_with_dungeon_server(const boost::shared_ptr<DungeonObject> &dungeon_object) const noexcept {
	PROFILE_ME;

	const auto server = m_server.lock();
	if(server){
		try {
			dungeon_object->synchronize_with_dungeon_server(server);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			server->shutdown(e.what());
		}
	}
}
*/

void Dungeon::pump_status(){
	PROFILE_ME;

	//
}

void Dungeon::set_founder_uuid(AccountUuid founder_uuid) noexcept {
	PROFILE_ME;

	m_founder_uuid = founder_uuid;

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);
}

void Dungeon::set_expiry_time(std::uint64_t expiry_time) noexcept {
	PROFILE_ME;

	m_expiry_time = expiry_time;

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);
}

void Dungeon::set_begin(bool begin) noexcept{
	PROFILE_ME;

	m_begin = begin;
}

void Dungeon::set_scope(Rectangle scope){
	PROFILE_ME;

	m_scope = scope;

	try {
		Msg::SC_DungeonSetScope msg;
		fill_scope_msg(msg, get_dungeon_uuid(), m_scope);
		broadcast_to_observers(msg);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		clear_observers(Q_INTERNAL_ERROR, e.what());
	}
}

void Dungeon::set_suspension(Suspension suspension){
	PROFILE_ME;

	m_suspension = std::move(suspension);

	try {
		Msg::SC_DungeonWaitForPlayerConfirmation msg;
		fill_suspension_msg(msg, get_dungeon_uuid(), m_suspension);
		broadcast_to_observers(msg);
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		clear_observers(Q_INTERNAL_ERROR, e.what());
	}
}

boost::shared_ptr<PlayerSession> Dungeon::get_observer(AccountUuid account_uuid) const {
	PROFILE_ME;

	const auto it = m_observers.find(account_uuid);
	if(it == m_observers.end()){
		LOG_EMPERY_CENTER_DEBUG("Observer not found: account_uuid = ", account_uuid);
		return { };
	}
	auto session = it->second.session.lock();
	if(!session){
		LOG_EMPERY_CENTER_DEBUG("Observer session expired: account_uuid = ", account_uuid);
		return { };
	}
	return std::move(session);
}
void Dungeon::get_observers_all(std::vector<std::pair<AccountUuid, boost::shared_ptr<PlayerSession>>> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_observers.size());
	for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
		auto session = it->second.session.lock();
		if(!session){
			continue;
		}
		ret.emplace_back(it->first, std::move(session));
	}
}
void Dungeon::insert_observer(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	Observer elem = { session };
	const auto result = m_observers.emplace(account_uuid, std::move(elem));
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Observer already exists: dungeon_uuid = ", get_dungeon_uuid(), ", account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Observer already exists"));
	}

	try {
		Msg::SC_DungeonEntered msg_entered;
		msg_entered.dungeon_uuid    = get_dungeon_uuid().str();
		msg_entered.dungeon_type_id = get_dungeon_type_id().get();
		session->send(msg_entered);

		const auto &scope = get_scope();
		Msg::SC_DungeonSetScope msg_scope;
		fill_scope_msg(msg_scope, get_dungeon_uuid(), scope);
		session->send(msg_scope);

		const auto &suspension = get_suspension();
		if(suspension.type != 0){
			Msg::SC_DungeonWaitForPlayerConfirmation msg_susp;
			fill_suspension_msg(msg_susp, get_dungeon_uuid(), suspension);
			session->send(msg_susp);
		}
	} catch(std::exception &e){
		LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		session->shutdown(e.what());
	}
}
bool Dungeon::remove_observer(AccountUuid account_uuid, Dungeon::QuitReason reason, const char *param) noexcept {
	PROFILE_ME;

	if(!param){
		param = "";
	}

	const auto it = m_observers.find(account_uuid);
	if(it == m_observers.end()){
		LOG_EMPERY_CENTER_DEBUG("Observer not found: dungeon_uuid = ", get_dungeon_uuid(), ", account_uuid = ", account_uuid);
		return false;
	}
	const auto session = it->second.session.lock();
	m_observers.erase(it);

	if(session){
		try {
			Msg::SC_DungeonLeft msg_left;
			msg_left.dungeon_uuid = get_dungeon_uuid().str();
			msg_left.reason       = static_cast<int>(reason);
			msg_left.param        = param;
			session->send(msg_left);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return true;
}
void Dungeon::clear_observers(Dungeon::QuitReason reason, const char *param) noexcept {
	PROFILE_ME;

	if(!param){
		param = "";
	}

	for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
		const auto session = it->second.session.lock();
		if(session){
			try {
				Msg::SC_DungeonLeft msg;
				msg.dungeon_uuid    = get_dungeon_uuid().str();
				msg.reason          = static_cast<int>(reason);
				msg.param           = param;
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
	m_observers.clear();
}

void Dungeon::get_soldier_stats(std::vector<std::pair<MapObjectTypeId, Dungeon::SoldierStat>> &ret, AccountUuid account_uuid) const {
	PROFILE_ME;

	const auto it = m_observers.find(account_uuid);
	if(it == m_observers.end()){
		return;
	}
	ret.reserve(ret.size() + it->second.soldier_stats.size());
	for(auto sit = it->second.soldier_stats.begin(); sit != it->second.soldier_stats.end(); ++sit){
		ret.emplace_back(sit->first, sit->second);
	}
}
void Dungeon::update_soldier_stat(AccountUuid account_uuid, MapObjectTypeId map_object_type_id,
	std::uint64_t damaged, std::uint64_t resuscitated, std::uint64_t wounded)
{
	PROFILE_ME;

	auto &stat = m_observers[account_uuid].soldier_stats[map_object_type_id];
	stat.damaged = saturated_add(stat.damaged, damaged);
	stat.resuscitated = saturated_add(stat.resuscitated, resuscitated);
	stat.wounded = saturated_add(stat.wounded, wounded);
}

void Dungeon::broadcast_to_observers(std::uint16_t message_id, const Poseidon::StreamBuffer &payload){
	PROFILE_ME;

	for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
		const auto session = it->second.session.lock();
		if(session){
			try {
				session->send(message_id, payload);
				LOG_EMPERY_CENTER_DEBUG("broadcast_to_observers message_id:",message_id," msg_data = ", payload.dump());
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}

boost::shared_ptr<DungeonObject> Dungeon::get_object(DungeonObjectUuid dungeon_object_uuid) const {
	PROFILE_ME;

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_CENTER_DEBUG("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid);
		return { };
	}
	return it->second;
}
void Dungeon::get_objects_all(std::vector<boost::shared_ptr<DungeonObject>> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_objects.size());
	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		ret.emplace_back(it->second);
	}
}
void Dungeon::insert_object(const boost::shared_ptr<DungeonObject> &dungeon_object){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_object->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_CENTER_WARNING("This dungeon object does not belong to this dungeon!");
		DEBUG_THROW(Exception, sslit("This dungeon object does not belong to this dungeon"));
	}

	const auto dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid();

	if(dungeon_object->is_virtually_removed()){
		LOG_EMPERY_CENTER_WARNING("Dungeon object has been marked as deleted: dungeon_object_uuid = ", dungeon_object_uuid);
		DEBUG_THROW(Exception, sslit("Dongeon object has been marked as deleted"));
	}

	LOG_EMPERY_CENTER_DEBUG("Inserting dungeon object: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
	const auto result = m_objects.emplace(dungeon_object_uuid, dungeon_object);
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Dungeon object already exists: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon object already exists"));
	}

	synchronize_with_all_observers(dungeon_object);
	synchronize_with_dungeon_server(dungeon_object);
}
void Dungeon::update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_object->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_CENTER_WARNING("This dungeon object does not belong to this dungeon!");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("This dungeon object does not belong to this dungeon"));
		}
		return;
	}

	const auto dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid();

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_CENTER_TRACE("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Dungeon object not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_TRACE("Updating dungeon object: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
	if(dungeon_object->is_virtually_removed()){
		m_objects.erase(it);
	} else {
		//
	}

	synchronize_with_all_observers(dungeon_object);
	synchronize_with_dungeon_server(dungeon_object);
}

boost::shared_ptr<DungeonBuff> Dungeon::get_dungeon_buff(Coord coord){
	PROFILE_ME;

	const auto it = m_dungeon_buffs.find(coord);
	if(it == m_dungeon_buffs.end()){
		LOG_EMPERY_CENTER_DEBUG("Dungeon buff not found: coord = ", coord);
		return { };
	}
	return it->second;
}
void Dungeon::insert_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_buff->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_CENTER_WARNING("This dungeon buff does not belong to this dungeon!");
		DEBUG_THROW(Exception, sslit("This dungeon buff does not belong to this dungeon"));
	}

	const auto coord = dungeon_buff->get_coord();

	if(dungeon_buff->is_virtually_removed()){
		LOG_EMPERY_CENTER_WARNING("Dungeon buff has been marked as deleted: coord = ", coord);
		DEBUG_THROW(Exception, sslit("Dongeon buff has been marked as deleted"));
	}

	LOG_EMPERY_CENTER_DEBUG("Inserting dungeon buff: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
	const auto result = m_dungeon_buffs.emplace(coord, dungeon_buff);
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Dungeon buff already exists: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon buff already exists"));
	}

	synchronize_with_all_observers(dungeon_buff);
	synchronize_with_dungeon_server(dungeon_buff);
}
void Dungeon::update_dungeon_buff(const boost::shared_ptr<DungeonBuff> &dungeon_buff, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_buff->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_CENTER_WARNING("This dungeon buff does not belong to this dungeon!");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("This dungeon buff does not belong to this dungeon"));
		}
		return;
	}

	const auto coord = dungeon_buff->get_coord();

	const auto it = m_dungeon_buffs.find(coord);
	if(it == m_dungeon_buffs.end()){
		LOG_EMPERY_CENTER_TRACE("Dungeon buff not found: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Dungeon buff not found"));
		}
		return;
	}

	LOG_EMPERY_CENTER_TRACE("Updating dungeon buff: coord = ", coord, ", dungeon_uuid = ", dungeon_uuid);
	if(dungeon_buff->is_virtually_removed()){
		m_dungeon_buffs.erase(it);
	} else {
		//
		it->second = dungeon_buff;
	}

	synchronize_with_all_observers(dungeon_buff);
	synchronize_with_dungeon_server(dungeon_buff);
}

bool Dungeon::is_virtually_removed() const {
	return get_expiry_time() == 0;
}
void Dungeon::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	for(auto it = m_objects.begin(); it != m_objects.end(); ++it){
		const auto &dungeon_object = it->second;
		dungeon_object->synchronize_with_player(session);
	}
	
}

}
