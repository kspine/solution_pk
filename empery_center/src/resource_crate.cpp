#include "precompiled.hpp"
#include "resource_crate.hpp"
#include "mysql/resource_crate.hpp"
#include "singletons/world_map.hpp"
#include "player_session.hpp"
#include "cluster_session.hpp"
#include "msg/sc_map.hpp"
#include "msg/sk_map.hpp"
#include "checked_arithmetic.hpp"
#include "data/map_object_type.hpp"
#include "data/castle.hpp"
#include "attribute_ids.hpp"
#include "map_object.hpp"

namespace EmperyCenter {

ResourceCrate::ResourceCrate(ResourceCrateUuid resource_crate_uuid, ResourceId resource_id, std::uint64_t amount_max,
	Coord coord, std::uint64_t created_time, std::uint64_t expiry_time)
	: m_obj(
		[&]{
			auto obj = boost::make_shared<MySql::Center_ResourceCrate>(resource_crate_uuid.get(),
				resource_id.get(), amount_max, coord.x(), coord.y(), created_time, expiry_time, amount_max);
			obj->async_save(true);
			return obj;
		}())
{
}
ResourceCrate::ResourceCrate(boost::shared_ptr<MySql::Center_ResourceCrate> obj)
	: m_obj(std::move(obj))
{
}
ResourceCrate::~ResourceCrate(){
}

ResourceCrateUuid ResourceCrate::get_resource_crate_uuid() const {
	return ResourceCrateUuid(m_obj->unlocked_get_resource_crate_uuid());
}
ResourceId ResourceCrate::get_resource_id() const {
	return ResourceId(m_obj->get_resource_id());
}
std::uint64_t ResourceCrate::get_amount_max() const {
	return m_obj->get_amount_max();
}
Coord ResourceCrate::get_coord() const {
	return Coord(m_obj->get_x(), m_obj->get_y());
}
std::uint64_t ResourceCrate::get_created_time() const {
	return m_obj->get_created_time();
}
std::uint64_t ResourceCrate::get_expiry_time() const {
	return m_obj->get_expiry_time();
}
std::uint64_t ResourceCrate::get_amount_remaining() const {
	return m_obj->get_amount_remaining();
}

bool ResourceCrate::has_been_deleted() const {
	return m_obj->get_amount_remaining() == 0;
}
void ResourceCrate::delete_from_game() noexcept {
	m_obj->set_amount_remaining(0);

	WorldMap::update_resource_crate(virtual_shared_from_this<ResourceCrate>(), false);
}

std::uint64_t ResourceCrate::harvest(const boost::shared_ptr<MapObject> &harvester, double amount_to_harvest, bool saturated){
	PROFILE_ME;

	const auto resource_id = get_resource_id();
	const auto resource_data = Data::CastleResource::require(resource_id);
	const auto carried_attribute_id = resource_data->carried_attribute_id;
	if(!carried_attribute_id){
		LOG_EMPERY_CENTER_DEBUG("Resource is not harvestable: resource_id = ", resource_id);
		return 0;
	}
	const auto amount_remaining = get_amount_remaining();
	if(amount_remaining == 0){
		LOG_EMPERY_CENTER_DEBUG("No resource available: resource_crate_uuid = ", get_resource_crate_uuid());
		return 0;
	}

	const auto harvester_type_id = harvester->get_map_object_type_id();
	const auto harvester_type_data = Data::MapObjectTypeBattalion::require(harvester_type_id);

	const auto rounded_amount_to_harvest = static_cast<std::uint64_t>(amount_to_harvest);
	const auto rounded_amount_removable = std::min(rounded_amount_to_harvest, amount_remaining);

	const auto soldier_count = static_cast<std::uint64_t>(harvester->get_attribute(AttributeIds::ID_SOLDIER_COUNT));
	const auto resource_capacity = static_cast<std::uint64_t>(std::floor(harvester_type_data->resource_carriable * soldier_count + 0.001));
	const auto resource_amount_carried = harvester->get_resource_amount_carried();
	const auto capacity_remaining = saturated_sub(resource_capacity, resource_amount_carried);
	const auto amount_to_add = std::min(rounded_amount_removable, capacity_remaining);
	const auto amount_to_remove = saturated ? rounded_amount_removable : amount_to_add;
	LOG_EMPERY_CENTER_DEBUG("Harvesting resource crate: resource_crate_uuid = ", get_resource_crate_uuid(),
		", resource_id = ", resource_id, ", carried_attribute_id = ", carried_attribute_id,
		", resource_amount_carried = ", resource_amount_carried, ", capacity_remaining = ", capacity_remaining,
		", amount_to_add = ", amount_to_add, ", amount_to_remove = ", amount_to_remove);

	boost::container::flat_map<AttributeId, std::int64_t> modifiers;
	modifiers[carried_attribute_id] = harvester->get_attribute(carried_attribute_id) + static_cast<std::int64_t>(amount_to_add);

	harvester->set_attributes(std::move(modifiers));
	m_obj->set_amount_remaining(checked_sub(m_obj->get_amount_remaining(), amount_to_remove));

	WorldMap::update_resource_crate(virtual_shared_from_this<ResourceCrate>(), false);

	return amount_to_remove;
}

bool ResourceCrate::is_virtually_removed() const {
	return has_been_deleted();
}
void ResourceCrate::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SC_MapResourceCrateRemoved msg;
		msg.resource_crate_uuid = get_resource_crate_uuid().str();
		session->send(msg);
	} else {
		Msg::SC_MapResourceCrateInfo msg;
		msg.resource_crate_uuid = get_resource_crate_uuid().str();
		msg.resource_id         = get_resource_id().get();
		msg.amount_max          = get_amount_max();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		const auto utc_now = Poseidon::get_utc_time();
		msg.expiry_duration     = saturated_sub(get_expiry_time(), utc_now);
		msg.amount_remaining    = get_amount_remaining();
		session->send(msg);
	}
}
void ResourceCrate::synchronize_with_cluster(const boost::shared_ptr<ClusterSession> &cluster) const {
	PROFILE_ME;

	if(is_virtually_removed()){
		Msg::SK_MapRemoveResourceCrate msg;
		msg.resource_crate_uuid = get_resource_crate_uuid().str();
		cluster->send(msg);
	} else {
		Msg::SK_MapCreateResourceCrate msg;
		msg.resource_crate_uuid = get_resource_crate_uuid().str();
		msg.resource_id         = get_resource_id().get();
		msg.amount_max          = get_amount_max();
		msg.x                   = get_coord().x();
		msg.y                   = get_coord().y();
		msg.created_time        = get_created_time();
		msg.expiry_time         = get_expiry_time();
		msg.amount_remaining    = get_amount_remaining();
		cluster->send(msg);
	}
}

}
