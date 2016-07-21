#include "precompiled.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "msg/sd_dungeon.hpp"
#include "dungeon_session.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "msg/sc_dungeon.hpp"
#include "src/dungeon_object.hpp"

namespace EmperyCenter {

Dungeon::Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id, std::uint64_t expiry_time,
	const boost::shared_ptr<DungeonSession> &server, AccountUuid founder_uuid)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_type_id(dungeon_type_id), m_expiry_time(expiry_time)
	, m_server(server)
	, m_founder_uuid(founder_uuid)
{
	try {
		Msg::SD_DungeonCreate msg;
		msg.dungeon_uuid    = get_dungeon_uuid().str();
		msg.dungeon_type_id = get_dungeon_type_id().get();
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

	clear_observers(Q_DESTRUCTOR, { });
}

void Dungeon::broadcast_to_observers(std::uint16_t message_id, const Poseidon::StreamBuffer &payload){
	PROFILE_ME;

	for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
		const auto session = it->second.lock();
		if(session){
			try {
				session->send(message_id, payload);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
}
template<class MessageT>
void Dungeon::broadcast_to_observers(const MessageT &msg){
	broadcast_to_observers(MessageT::ID, Poseidon::StreamBuffer(msg));
}

void Dungeon::synchronize_with_all_observers(const boost::shared_ptr<DungeonObject> &dungeon_object) const noexcept {
	PROFILE_ME;

	for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
		const auto session = it->second.lock();
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

void Dungeon::pump_status(){
	PROFILE_ME;

	//
}

void Dungeon::set_founder_uuid(AccountUuid founder_uuid){
	PROFILE_ME;

	m_founder_uuid = founder_uuid;

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);
}

boost::shared_ptr<PlayerSession> Dungeon::get_observer(AccountUuid account_uuid) const {
	PROFILE_ME;

	const auto it = m_observers.find(account_uuid);
	if(it == m_observers.end()){
		LOG_EMPERY_CENTER_DEBUG("Observer not found: account_uuid = ", account_uuid);
		return { };
	}
	auto session = it->second.lock();
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
		auto session = it->second.lock();
		if(!session){
			continue;
		}
		ret.emplace_back(it->first, std::move(session));
	}
}
void Dungeon::insert_observer(AccountUuid account_uuid, const boost::shared_ptr<PlayerSession> &session){
	PROFILE_ME;

	const auto result = m_observers.emplace(account_uuid, session);
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Observer already exists: dungeon_uuid = ", get_dungeon_uuid(), ", account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Observer already exists"));
	}

	try {
		Msg::SC_DungeonEntered msg;
		msg.dungeon_uuid    = get_dungeon_uuid().str();
		msg.dungeon_type_id = get_dungeon_type_id().get();
		session->send(msg);
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
	const auto session = it->second.lock();
	m_observers.erase(it);

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

	return true;
}
void Dungeon::clear_observers(Dungeon::QuitReason reason, const char *param) noexcept {
	PROFILE_ME;

	if(!param){
		param = "";
	}

	for(auto it = m_observers.begin(); it != m_observers.end(); ++it){
		const auto session = it->second.lock();
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

void Dungeon::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	
}

}
