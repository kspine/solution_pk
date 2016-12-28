#include "precompiled.hpp"
#include "defense_matrix.hpp"
#include "dungeon_object.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "dungeon_client.hpp"
#include "cbpp_response.hpp"
#include "../../empery_center/src/msg/ds_dungeon.hpp"
#include "../../empery_center/src/msg/err_dungeon.hpp"
#include "dungeon_utilities.hpp"
#include "data/dungeon_buff.hpp"
#include "dungeon_buff.hpp"

namespace EmperyDungeon {

namespace Msg = ::EmperyCenter::Msg;


DefenseMatrix::DefenseMatrix(boost::weak_ptr<Dungeon> dungeon,std::vector<std::string> tags,std::uint64_t next_change_time,std::uint64_t interval)
	: m_owner_dungeon(dungeon),m_defense_matrix(std::move(tags)),m_next_change_time(next_change_time),m_interval(interval)
{
}
DefenseMatrix::~DefenseMatrix(){
}
void DefenseMatrix::change_defense_matrix(std::uint64_t utc_now){
	PROFILE_ME;

	auto dungeon = m_owner_dungeon.lock();
	if(!dungeon){
		LOG_EMPERY_DUNGEON_WARNING("emptry dungeon");
		return;
	}
	m_ignore_object_uuid = DungeonObjectUuid();
	std::vector<boost::shared_ptr<DungeonObject>> ret;
	dungeon->get_objects_all(ret);
	std::vector<boost::shared_ptr<DungeonObject>> valid_objects;
	for(auto it = ret.begin(); it != ret.end(); ++it){
		auto object = *it;
		auto tag = object->get_tag();
		auto itv = std::find(m_defense_matrix.begin(), m_defense_matrix.end(),tag);
		if(itv != m_defense_matrix.end()){
			valid_objects.push_back(object);
		}
	}
	if(valid_objects.empty()){
		LOG_EMPERY_DUNGEON_WARNING("matrix valid object is empty");
		return;
	}
	std::random_shuffle(valid_objects.begin(), valid_objects.end());
	const auto buff_data = Data::DungeonBuff::require(ID_BUFF_DEFENSE_MATRIX);
	auto expired_time      = utc_now + buff_data->continue_time*1000;
	for(unsigned i = 0; i < valid_objects.size() - 1; ++i){
		auto valid_object = valid_objects.at(i);
		auto dungeon_object_uuid = valid_object->get_dungeon_object_uuid();
		auto dungeon_object_owner_account = valid_object->get_owner_uuid();
		auto dungeon_buff = boost::make_shared<DungeonBuff>(dungeon->get_dungeon_uuid(),ID_BUFF_DEFENSE_MATRIX,dungeon_object_uuid,dungeon_object_owner_account,Coord(0,0),expired_time);
		dungeon->insert_skill_buff(dungeon_object_uuid,ID_BUFF_DEFENSE_MATRIX,std::move(dungeon_buff));
	}
	const auto ingore_object = valid_objects.at(valid_objects.size() - 1);
	if(!ingore_object){
		LOG_EMPERY_DUNGEON_WARNING("valid_objects empty");
		return;
	}
	m_ignore_object_uuid = ingore_object->get_dungeon_object_uuid();
	dungeon->remove_skill_buff(m_ignore_object_uuid,ID_BUFF_DEFENSE_MATRIX);
	m_next_change_time = utc_now + m_interval*1000;
}

bool DefenseMatrix::is_ignore_die(){
	if(!m_ignore_object_uuid){
		return false;
	}
	auto dungeon = m_owner_dungeon.lock();
	if(!dungeon){
		LOG_EMPERY_DUNGEON_WARNING("emptry dungeon");
		return false;
	}
   auto ingore_object = dungeon->get_object(m_ignore_object_uuid);
   if(!ingore_object){
	   return true;
   }
   return false;
}

}
