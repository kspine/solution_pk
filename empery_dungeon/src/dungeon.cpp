#include "precompiled.hpp"
#include "dungeon.hpp"
#include "singletons/dungeon_map.hpp"
#include "src/dungeon_object.hpp"

namespace EmperyDungeon {

Dungeon::Dungeon(DungeonUuid dungeon_uuid, DungeonTypeId dungeon_type_id,const boost::shared_ptr<DungeonClient> &dungeon,AccountUuid founder_uuid)
	: m_dungeon_uuid(dungeon_uuid), m_dungeon_type_id(dungeon_type_id)
	, m_dungeon_client(dungeon)
	, m_founder_uuid(founder_uuid)
{
}
Dungeon::~Dungeon(){
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

boost::shared_ptr<DungeonObject> Dungeon::get_object(DungeonObjectUuid dungeon_object_uuid) const {
	PROFILE_ME;

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_DUNGEON_DEBUG("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid);
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
		LOG_EMPERY_DUNGEON_WARNING("This dungeon object does not belong to this dungeon!");
		DEBUG_THROW(Exception, sslit("This dungeon object does not belong to this dungeon"));
	}

	const auto dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid();

	if(dungeon_object->is_virtually_removed()){
		LOG_EMPERY_DUNGEON_WARNING("Dungeon object has been marked as deleted: dungeon_object_uuid = ", dungeon_object_uuid);
		DEBUG_THROW(Exception, sslit("Dongeon object has been marked as deleted"));
	}

	LOG_EMPERY_DUNGEON_DEBUG("Inserting dungeon object: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
	const auto result = m_objects.emplace(dungeon_object_uuid, dungeon_object);
	if(!result.second){
		LOG_EMPERY_DUNGEON_WARNING("Dungeon object already exists: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
		DEBUG_THROW(Exception, sslit("Dungeon object already exists"));
	}
}
void Dungeon::update_object(const boost::shared_ptr<DungeonObject> &dungeon_object, bool throws_if_not_exists){
	PROFILE_ME;

	const auto dungeon_uuid = get_dungeon_uuid();
	if(dungeon_object->get_dungeon_uuid() != dungeon_uuid){
		LOG_EMPERY_DUNGEON_WARNING("This dungeon object does not belong to this dungeon!");
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("This dungeon object does not belong to this dungeon"));
		}
		return;
	}

	const auto dungeon_object_uuid = dungeon_object->get_dungeon_object_uuid();

	const auto it = m_objects.find(dungeon_object_uuid);
	if(it == m_objects.end()){
		LOG_EMPERY_DUNGEON_TRACE("Dungeon object not found: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Dungeon object not found"));
		}
		return;
	}

	LOG_EMPERY_DUNGEON_TRACE("Updating dungeon object: dungeon_object_uuid = ", dungeon_object_uuid, ", dungeon_uuid = ", dungeon_uuid);
	if(dungeon_object->is_virtually_removed()){
		m_objects.erase(it);
	} else {
		//
	}
}


}
