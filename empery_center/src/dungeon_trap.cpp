#include "precompiled.hpp"
#include "dungeon_trap.hpp"
#include "dungeon.hpp"
#include "singletons/account_map.hpp"
#include "msg/sc_dungeon.hpp"
#include "msg/sd_dungeon.hpp"
#include "player_session.hpp"
#include "dungeon_session.hpp"
#include "singletons/dungeon_map.hpp"

namespace EmperyCenter {

DungeonTrap::DungeonTrap(DungeonUuid dungeon_uuid,DungeonTrapTypeId trap_type_id,Coord coord)
	: m_dungeon_uuid(dungeon_uuid),m_dungeon_trap_type_id(trap_type_id), m_coord(coord)
{
}
DungeonTrap::~DungeonTrap(){
}

void DungeonTrap::pump_status(){
	PROFILE_ME;

	//
}

void DungeonTrap::set_coord_no_synchronize(Coord coord) noexcept {
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;
}

void DungeonTrap::delete_from_game() noexcept {
	PROFILE_ME;

	m_deleted = true;

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		dungeon->update_trap(virtual_shared_from_this<DungeonTrap>(), false);
	}
}

bool DungeonTrap::is_virtually_removed() const {
	return has_been_deleted();
}
void DungeonTrap::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_DungeonTrapRemoved msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		session->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	} else {
		Msg::SC_DungeonTrapInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.trap_type_id        = get_dungeon_trap_type_id().get();
		session->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	}
}
void DungeonTrap::synchronize_with_dungeon_server(const boost::shared_ptr<DungeonSession> &server) const {
	PROFILE_ME;

	if(has_been_deleted()){
		Msg::SD_DungeonTrapRemoved msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		server->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	} else {
		Msg::SD_DungeonTrapInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.trap_type_id        = get_dungeon_trap_type_id().get();
		server->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	}
}

}
