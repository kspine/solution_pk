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

		info.dungeon_id = DungeonId(obj->get_dungeon_id());
		info.score      = DungeonBox::Score(obj->get_score());
	}

	void fill_dungeon_message(Msg::SC_DungeonChanged &msg, const boost::shared_ptr<MySql::Center_Dungeon> &obj){
		PROFILE_ME;

		msg.dungeon_id = obj->get_dungeon_id();
		msg.score      = obj->get_score();
	}
}

DungeonBox::DungeonBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Dungeon>> &dungeons)
	: m_account_uuid(account_uuid)
{
	for(auto it = dungeons.begin(); it != dungeons.end(); ++it){
		const auto &obj = *it;
		m_dungeons.emplace(DungeonId(obj->get_dungeon_id()), obj);
	}
}
DungeonBox::~DungeonBox(){
}

DungeonBox::DungeonInfo DungeonBox::get(DungeonId dungeon_id) const {
	PROFILE_ME;

	DungeonInfo info = { };
	info.dungeon_id = dungeon_id;

	const auto it = m_dungeons.find(dungeon_id);
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

void DungeonBox::insert(DungeonBox::DungeonInfo info){
	PROFILE_ME;

	const auto dungeon_id = info.dungeon_id;
	auto it = m_dungeons.find(dungeon_id);
	if(it != m_dungeons.end()){
		LOG_EMPERY_CENTER_WARNING("Dungeon exists: account_uuid = ", get_account_uuid(), ", dungeon_id = ", dungeon_id);
		DEBUG_THROW(Exception, sslit("Dungeon exists"));
	}

	const auto obj = boost::make_shared<MySql::Center_Dungeon>(get_account_uuid().get(), dungeon_id.get(), S_NONE);
	obj->async_save(true);
	it = m_dungeons.emplace(dungeon_id, obj).first;

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_DungeonChanged msg;
			fill_dungeon_message(msg, it->second);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void DungeonBox::update(DungeonBox::DungeonInfo info, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_id = info.dungeon_id;
	const auto it = m_dungeons.find(dungeon_id);
	if(it == m_dungeons.end()){
		LOG_EMPERY_CENTER_WARNING("Dungeon not found: account_uuid = ", get_account_uuid(), ", dungeon_id = ", dungeon_id);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Task not found"));
		}
		return;
	}
	const auto &obj = it->second;

	obj->set_score(static_cast<unsigned>(info.score));

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_DungeonChanged msg;
			fill_dungeon_message(msg, obj);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
bool DungeonBox::remove(DungeonId dungeon_id) noexcept {
	PROFILE_ME;

	const auto it = m_dungeons.find(dungeon_id);
	if(it == m_dungeons.end()){
		return false;
	}
	const auto obj = std::move(it->second);
	// m_dungeons.erase(it);

	obj->set_score(S_NONE);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_DungeonChanged msg;
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
		Msg::SC_DungeonChanged msg;
		fill_dungeon_message(msg, it->second);
		session->send(msg);
	}
}

}
