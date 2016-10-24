#include "precompiled.hpp"
#include "dungeon_buff.hpp"
#include "dungeon.hpp"
#include "singletons/account_map.hpp"
#include "msg/sc_dungeon.hpp"
#include "msg/sd_dungeon.hpp"
#include "player_session.hpp"
#include "dungeon_session.hpp"
#include "singletons/dungeon_map.hpp"

namespace EmperyCenter {

DungeonBuff::DungeonBuff(DungeonUuid dungeon_uuid,DungeonBuffTypeId buff_type_id,DungeonObjectUuid create_uuid,AccountUuid create_owner_uuid,Coord coord,std::uint64_t expired_time)
	: m_dungeon_uuid(dungeon_uuid),m_dungeon_buff_type_id(buff_type_id),m_create_uuid(create_uuid),m_create_owner_uuid(create_owner_uuid),m_coord(coord),m_expired_time(expired_time)
{
}

DungeonBuff::~DungeonBuff(){
}

void DungeonBuff::pump_status(){
	PROFILE_ME;

	//
}

void DungeonBuff::set_coord_no_synchronize(Coord coord) noexcept {
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;
}

void DungeonBuff::delete_from_game() noexcept {
	PROFILE_ME;

	m_deleted = true;

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		//dungeon->update_trap(virtual_shared_from_this<DungeonBuff>(), false);
	}
}

bool DungeonBuff::is_virtually_removed() const {
	return has_been_deleted();
}
void DungeonBuff::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_DungeonBuffRemoved msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		session->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	} else {
		Msg::SC_DungeonBuffInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.buff_type_id        = get_dungeon_buff_type_id().get();
		msg.create_uuid         = get_create_uuid().str();
		msg.create_owner_uuid   = get_create_owner_uuid().str();
		msg.expired_time        = m_expired_time;
		session->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	}
}
void DungeonBuff::synchronize_with_dungeon_server(const boost::shared_ptr<DungeonSession> &server) const {
	PROFILE_ME;

	if(has_been_deleted()){
		Msg::SD_DungeonBuffRemoved msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		server->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	} else {
		Msg::SD_DungeonBuffInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.buff_type_id        = get_dungeon_buff_type_id().get();
		msg.create_uuid         = get_create_uuid().str();
		msg.create_owner_uuid   = get_create_owner_uuid().str();
		msg.expired_time        = m_expired_time;
		server->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	}
}

}
