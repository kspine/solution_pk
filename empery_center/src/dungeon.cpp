#include "precompiled.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "dungeon_session.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "msg/sc_dungeon.hpp"

namespace EmperyCenter {

Dungeon::Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id, std::uint64_t expiry_time, const boost::shared_ptr<DungeonSession> &server,
	AccountUuid founder_uuid)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_type_id(dungeon_type_id), m_expiry_time(expiry_time), m_server(server)
	, m_founder_uuid(founder_uuid)
{
}
Dungeon::~Dungeon(){
	clear();
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

bool Dungeon::is_account_in(AccountUuid account_uuid) const {
	PROFILE_ME;

	const auto it = m_accounts.find(account_uuid);
	if(it == m_accounts.end()){
		return false;
	}
	return true;
}
void Dungeon::get_all_accounts(std::vector<AccountUuid> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_accounts.size());
	for(auto it = m_accounts.begin(); it != m_accounts.end(); ++it){
		ret.emplace_back(*it);
	}
}
void Dungeon::insert_account(AccountUuid account_uuid){
	PROFILE_ME;

	const auto result = m_accounts.insert(account_uuid);
	if(!result.second){
		LOG_EMPERY_CENTER_WARNING("Account conflict: dungeon_uuid = ", get_dungeon_uuid(), ", account_uuid = ", account_uuid);
		DEBUG_THROW(Exception, sslit("Account conflict"));
	}

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
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
}
bool Dungeon::remove_account(AccountUuid account_uuid) noexcept {
	PROFILE_ME;

	const auto it = m_accounts.find(account_uuid);
	if(it == m_accounts.end()){
		return false;
	}
	m_accounts.erase(it);

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);

	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_DungeonLeft msg;
			msg.dungeon_uuid    = get_dungeon_uuid().str();
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return true;
}
void Dungeon::clear() noexcept {
	PROFILE_ME;

	for(auto it = m_accounts.begin(); it != m_accounts.end(); ++it){
		const auto account_uuid = *it;

		const auto session = PlayerSessionMap::get(account_uuid);
		if(session){
			try {
				Msg::SC_DungeonLeft msg;
				msg.dungeon_uuid    = get_dungeon_uuid().str();
				session->send(msg);
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
				session->shutdown(e.what());
			}
		}
	}
	m_accounts.clear();

	DungeonMap::update(virtual_shared_from_this<Dungeon>(), false);
}

}
