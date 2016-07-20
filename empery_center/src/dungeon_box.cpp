#include "precompiled.hpp"
#include "dungeon_box.hpp"
#include "msg/sc_dungeon.hpp"
#include "mysql/dungeon.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

namespace {
	void fill_dungeon_info(DungeonBox::DungeonInfo &info, const boost::shared_ptr<MySql::Center_Dungeon> &obj){
		PROFILE_ME;

		info.dungeon_type_id   = DungeonTypeId(obj->get_dungeon_type_id());
		info.score        = DungeonBox::Score(obj->get_score());
		info.entry_count  = obj->get_entry_count();
		info.finish_count = obj->get_finish_count();
	}

	void fill_dungeon_message(Msg::SC_DungeonScoreChanged &msg, const boost::shared_ptr<MySql::Center_Dungeon> &obj){
		PROFILE_ME;

		msg.dungeon_type_id = obj->get_dungeon_type_id();
		msg.score      = obj->get_score();
	}
}

DungeonBox::DungeonBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Dungeon>> &dungeons)
	: m_account_uuid(account_uuid)
{
	for(auto it = dungeons.begin(); it != dungeons.end(); ++it){
		const auto &obj = *it;
		m_dungeons.emplace(DungeonTypeId(obj->get_dungeon_type_id()), obj);
	}
}
DungeonBox::~DungeonBox(){
}

void DungeonBox::pump_status(){
	PROFILE_ME;

	//
}

DungeonBox::DungeonInfo DungeonBox::get(DungeonTypeId dungeon_type_id) const {
	PROFILE_ME;

	DungeonInfo info = { };
	info.dungeon_type_id = dungeon_type_id;

	const auto it = m_dungeons.find(dungeon_type_id);
	if(it == m_dungeons.end()){
		return info;
	}
	fill_dungeon_info(info, it->second);
	return info;
}
void DungeonBox::get_all(std::vector<DungeonBox::DungeonInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_dungeons.size());
	for(auto it = m_dungeons.begin(); it != m_dungeons.end(); ++it){
		DungeonInfo info;
		fill_dungeon_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

void DungeonBox::set(DungeonBox::DungeonInfo info){
	PROFILE_ME;

	const auto dungeon_type_id = info.dungeon_type_id;
	auto it = m_dungeons.find(dungeon_type_id);
	if(it == m_dungeons.end()){
		const auto obj = boost::make_shared<MySql::Center_Dungeon>(get_account_uuid().get(), dungeon_type_id.get(),
			S_NONE, 0, 0);
		obj->async_save(true);
		it = m_dungeons.emplace(dungeon_type_id, obj).first;
	}
	const auto &obj = it->second;

	obj->set_score(static_cast<unsigned>(info.score));
	obj->set_entry_count(info.entry_count);
	obj->set_finish_count(info.finish_count);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_DungeonScoreChanged msg;
			fill_dungeon_message(msg, obj);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
bool DungeonBox::remove(DungeonTypeId dungeon_type_id) noexcept {
	PROFILE_ME;

	const auto it = m_dungeons.find(dungeon_type_id);
	if(it == m_dungeons.end()){
		return false;
	}
	const auto obj = std::move(it->second);
	// m_dungeons.erase(it);

	obj->set_score(S_NONE);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_DungeonScoreChanged msg;
			fill_dungeon_message(msg, obj);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return true;
}

void DungeonBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	for(auto it = m_dungeons.begin(); it != m_dungeons.end(); ++it){
		Msg::SC_DungeonScoreChanged msg;
		fill_dungeon_message(msg, it->second);
		session->send(msg);
	}
}

}
