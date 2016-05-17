#include "precompiled.hpp"
#include "strategic_resource.hpp"
#include "mysql/strategic_resource.hpp"
#include "singletons/world_map.hpp"
#include "player_session.hpp"
#include "msg/sc_map.hpp"
#include "msg/sk_map.hpp"
#include "checked_arithmetic.hpp"
#include "data/map.hpp"
#include "singletons/account_map.hpp"
#include "data/map_object_type.hpp"
#include "data/castle.hpp"
#include "attribute_ids.hpp"
#include "map_object.hpp"

namespace EmperyCenter {

StrategicResource::StrategicResource(Coord coord, ResourceId resource_id, std::uint64_t resource_amount,
	std::uint64_t created_time, std::uint64_t expiry_time, MapEventId map_event_id)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_StrategicResource>(coord.x(), coord.y(),
				resource_id.get(), resource_amount, created_time, expiry_time, map_event_id.get());
			obj->async_save(true);
			return obj;
		}())
{
}
StrategicResource::StrategicResource(boost::shared_ptr<MySql::Center_StrategicResource> obj)
	: m_obj(std::move(obj))
{
}
StrategicResource::~StrategicResource(){
}

Coord StrategicResource::get_coord() const {
	return Coord(m_obj->get_x(), m_obj->get_y());
}
ResourceId StrategicResource::get_resource_id() const {
	return ResourceId(m_obj->get_resource_id());
}
std::uint64_t StrategicResource::get_resource_amount() const {
	return m_obj->get_resource_amount();
}
std::uint64_t StrategicResource::get_created_time() const {
	return m_obj->get_created_time();
}
std::uint64_t StrategicResource::get_expiry_time() const {
	return m_obj->get_expiry_time();
}
MapEventId StrategicResource::get_map_event_id() const {
	return MapEventId(m_obj->get_map_event_id());
}

bool StrategicResource::has_been_deleted() const {
	return m_obj->get_resource_amount() == 0;
}
void StrategicResource::delete_from_game() noexcept {
	m_obj->set_resource_amount(0);

	WorldMap::update_strategic_resource(virtual_shared_from_this<StrategicResource>(), false);
}

std::uint64_t StrategicResource::harvest(const boost::shared_ptr<MapObject> &harvester, double amount_to_harvest, bool saturated){
	PROFILE_ME;

	const auto resource_id = get_resource_id();
	if(!resource_id){
		LOG_EMPERY_CENTER_DEBUG("No resource id: coord = ", get_coord());
		return 0;
	}
	const auto amount_remaining = get_resource_amount();
	if(amount_remaining == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: coord = ", get_coord());
		return 0;
	}

	amount_to_harvest += m_harvest_remainder;

	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto rounded_amount_removable = std::min(rounded_amount_to_harvest, amount_remaining);
	const auto amount_added = harvester->load_resource(resource_id, rounded_amount_removable, false);
	const auto amount_removed = saturated ? rounded_amount_removable : amount_added;
	m_obj->set_resource_amount(saturated_sub(amount_remaining, amount_removed));

	m_harvest_remainder = amount_to_harvest - rounded_amount_to_harvest;
	m_last_harvester = harvester;

	WorldMap::update_strategic_resource(virtual_shared_from_this<StrategicResource>(), false);

	return amount_removed;
}

bool StrategicResource::is_virtually_removed() const {
	return has_been_deleted();
}
void StrategicResource::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_MapStrategicResourceRemoved msg;
		msg.x               = get_coord().x();
		msg.y               = get_coord().y();
		session->send(msg);
	} else {
		Msg::SC_MapStrategicResourceInfo msg;
		msg.x               = get_coord().x();
		msg.y               = get_coord().y();
		msg.resource_id     = get_resource_id().get();
		msg.resource_amount = get_resource_amount();
		const auto last_harvester = get_last_harvester();
		if(last_harvester){
			msg.last_harvested_account_uuid = last_harvester->get_owner_uuid().str();
			msg.last_harvested_object_uuid  = last_harvester->get_map_object_uuid().str();
		}
		msg.map_event_id    = get_map_event_id().get();
		session->send(msg);
	}
}

}
