#include "precompiled.hpp"
#include "dungeon_play.hpp"
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

DungeonPassPoint::DungeonPassPoint(DungeonUuid dungeon_uuid,Coord coord,boost::container::flat_map<Coord,BLOCK_STATE> blocks)
	: m_dungeon_uuid(dungeon_uuid), m_coord(coord),m_blocks(std::move(blocks))
{
}
DungeonPassPoint::~DungeonPassPoint(){
}

void DungeonPassPoint::pump_status(){
	PROFILE_ME;

	//
}

void DungeonPassPoint::set_coord_no_synchronize(Coord coord) noexcept {
	PROFILE_ME;

	if(get_coord() == coord){
		return;
	}
	m_coord = coord;
}

void DungeonPassPoint::delete_from_game() noexcept {
	PROFILE_ME;

	m_deleted = true;

	const auto dungeon_uuid = get_dungeon_uuid();
	const auto dungeon = DungeonMap::get(dungeon_uuid);
	if(dungeon){
		//dungeon->update_pass_point(virtual_shared_from_this<DungeonPassPoint>(), false);
	}
}

bool DungeonPassPoint::is_block_coord(Coord coord){
	PROFILE_ME;

	auto it = m_blocks.find(coord);
	if(it != m_blocks.end()){
		return true;
	}
	return false;
}
bool DungeonPassPoint::update_block_monster(std::string tag){
	bool dirty = false;
	std::uint64_t unblock_num = 0;
	for(auto it = m_blocks.begin(); it != m_blocks.end(); ++it){
		auto &block_state_pair = it->second;
		auto &block_state      = block_state_pair.first;
		auto &monster_state    = block_state_pair.second;
		std::uint64_t kill_monster = 0;
		for(auto itt = monster_state.begin(); itt != monster_state.end(); ++itt){
			if(itt->first == tag){
				itt->second = 1;
				dirty = true;
			}
			if(itt->second == 1){
				kill_monster +=1;
			}
		}
		if(kill_monster == monster_state.size()){
			block_state = 1;
			unblock_num +=1;
		}
	}
	if(unblock_num == m_blocks.size()){
		m_state = true;
	}
	if(dirty){
		return true;
	}
	return false;
}

bool DungeonPassPoint::is_virtually_removed() const {
	return has_been_deleted();
}
void DungeonPassPoint::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		LOG_EMPERY_CENTER_FATAL("pass point removed");
	} else {
		Msg::SC_DungeonPassPointInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.state               = m_state;
		for(auto it = m_blocks.begin(); it != m_blocks.end(); ++it){
			auto &block_coord = it->first;
			auto &block_state_pair = it->second;
			auto &block_state      = block_state_pair.first;
			auto &monster_state_vec    = block_state_pair.second;
			auto &block = *msg.blocks.emplace(msg.blocks.end());
			block.x = block_coord.x();
			block.y = block_coord.y();
			block.state = block_state;
			for(auto itt = monster_state_vec.begin();itt != monster_state_vec.end();++itt){
				auto &monster_state = *block.relate_monster.emplace(block.relate_monster.end());
				monster_state.tag = itt->first;
			}
		}
		session->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	}
}
void DungeonPassPoint::synchronize_with_dungeon_server(const boost::shared_ptr<DungeonSession> &server) const {
	PROFILE_ME;

	if(has_been_deleted()){
		LOG_EMPERY_CENTER_FATAL("pass point removed");
	} else {
		Msg::SD_DungeonPassPointInfo msg;
		msg.dungeon_uuid        = get_dungeon_uuid().str();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.state               = m_state;
		for(auto it = m_blocks.begin(); it != m_blocks.end(); ++it){
			auto &block_coord = it->first;
			auto &block_state_pair = it->second;
			auto &block_state      = block_state_pair.first;
			auto &monster_state_vec    = block_state_pair.second;
			auto &block = *msg.blocks.emplace(msg.blocks.end());
			block.x = block_coord.x();
			block.y = block_coord.y();
			block.state = block_state;
			for(auto itt = monster_state_vec.begin();itt != monster_state_vec.end();++itt){
				auto &monster_state = *block.relate_monster.emplace(block.relate_monster.end());
				monster_state.tag = itt->first;
			}
		}
		server->send(msg);
		LOG_EMPERY_CENTER_FATAL(msg);
	}
}

}
