#include "../precompiled.hpp"
#include "map_object_type.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	using BattalionMap = boost::container::flat_map<MapObjectTypeId, Data::MapObjectTypeBattalion>;
	boost::weak_ptr<const BattalionMap> g_battalion_map;
	const char BATTALION_FILE[] = "Arm";

	using MonsterMap = boost::container::flat_map<MapObjectTypeId, Data::MapObjectTypeMonster>;
	boost::weak_ptr<const MonsterMap> g_monster_map;
	const char MONSTER_FILE[] = "monster";

	MULTI_INDEX_MAP(MonsterRewardMap, Data::MapObjectTypeMonsterReward,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(collection_name)
	)
	boost::weak_ptr<const MonsterRewardMap> g_monster_reward_map;
	const char MONSTER_REWARD_FILE[] = "monster_reward";

	MULTI_INDEX_MAP(MonsterRewardExtraMap, Data::MapObjectTypeMonsterRewardExtra,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(available_since)
	)
	boost::weak_ptr<const MonsterRewardExtraMap> g_monster_reward_extra_map;
	const char MONSTER_REWARD_EXTRA_FILE[] = "monster_activity";

	MULTI_INDEX_MAP(AttributeBonusMap, Data::MapObjectTypeAttributeBonus,
		UNIQUE_MEMBER_INDEX(unique_id)
		MULTI_MEMBER_INDEX(applicability_key)
	)
	boost::weak_ptr<const AttributeBonusMap> g_attribute_bonus_map;
	const char ATTRIBUTE_BONUS_FILE[] = "attribute";

	template<typename ElementT>
	void read_map_object_type_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.map_object_type_id,     "arm_id");
		csv.get(elem.map_object_weapon_id,   "arm_type");
		csv.get(elem.map_object_chassis_id,  "arm_class");

		csv.get(elem.max_soldier_count,      "force_mnax");
		csv.get(elem.speed,                  "speed");
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(BATTALION_FILE);
		const auto battalion_map = boost::make_shared<BattalionMap>();
		while(csv.fetch_row()){
			Data::MapObjectTypeBattalion elem = { };

			read_map_object_type_abstract(elem, csv);

			csv.get(elem.harvest_speed,      "collect_speed");
			csv.get(elem.resource_carriable, "arm_load");

			Poseidon::JsonObject object;
			csv.get(object, "arm_need");
			elem.prerequisite.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto building_id = boost::lexical_cast<BuildingId>(it->first);
				const auto building_level = static_cast<unsigned>(it->second.get<double>());
				if(!elem.prerequisite.emplace(building_id, building_level).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate prerequisite: building_id = ", building_id);
					DEBUG_THROW(Exception, sslit("Duplicate prerequisite"));
				}
			}
			object.clear();
			csv.get(object, "levelup_resource");
			elem.enability_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.enability_cost.emplace(resource_id, resource_amount).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate enability resource cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate enability resource cost"));
				}
			}

			object.clear();
			csv.get(object, "recruit_resource");
			elem.production_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.production_cost.emplace(resource_id, resource_amount).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate production resource cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate production resource cost"));
				}
			}
			object.clear();
			csv.get(object, "maintenance_resource");
			elem.maintenance_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
				const auto resource_amount = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.maintenance_cost.emplace(resource_id, resource_amount).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate maintenance resource cost: resource_id = ", resource_id);
					DEBUG_THROW(Exception, sslit("Duplicate maintenance resource cost"));
				}
			}

			csv.get(elem.previous_id,        "soldiers_need");
			csv.get(elem.production_time,    "levy_time");
			csv.get(elem.factory_id,         "city_camp");

			if(!battalion_map->emplace(elem.map_object_type_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectTypeBattalion: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeBattalion"));
			}
		}
		g_battalion_map = battalion_map;
		handles.push(battalion_map);
		auto servlet = DataSession::create_servlet(BATTALION_FILE, Data::encode_csv_as_json(csv, "arm_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MONSTER_FILE);
		const auto monster_map = boost::make_shared<MonsterMap>();
		while(csv.fetch_row()){
			Data::MapObjectTypeMonster elem = { };

			read_map_object_type_abstract(elem, csv);

			csv.get(elem.return_range,       "return_range");

			Poseidon::JsonObject object;
			csv.get(object, "drop");
			elem.monster_rewards.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				auto collection_name = std::string(it->first.get());
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.monster_rewards.emplace(std::move(collection_name), count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate reward set: collection_name = ", collection_name);
					DEBUG_THROW(Exception, sslit("Duplicate reward set"));
				}
			}

			if(!monster_map->emplace(elem.map_object_type_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectTypeMonster: map_object_type_id = ", elem.map_object_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeMonster"));
			}
		}
		g_monster_map = monster_map;
		handles.push(monster_map);
		servlet = DataSession::create_servlet(MONSTER_FILE, Data::encode_csv_as_json(csv, "arm_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MONSTER_REWARD_FILE);
		const auto monster_reward_map = boost::make_shared<MonsterRewardMap>();
		while(csv.fetch_row()){
			Data::MapObjectTypeMonsterReward elem = { };

			csv.get(elem.unique_id,       "id");
			csv.get(elem.collection_name, "name");
			csv.get(elem.weight,          "probability");

			Poseidon::JsonObject object;
			csv.get(object, "item");
			elem.reward_items.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.reward_items.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate reward item: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate reward item"));
				}
			}

			if(!monster_reward_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectTypeMonsterReward: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeMonsterReward"));
			}
		}
		g_monster_reward_map = monster_reward_map;
		handles.push(monster_reward_map);
		servlet = DataSession::create_servlet(MONSTER_REWARD_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(MONSTER_REWARD_EXTRA_FILE);
		const auto monster_reward_extra_map = boost::make_shared<MonsterRewardExtraMap>();
		while(csv.fetch_row()){
			Data::MapObjectTypeMonsterRewardExtra elem = { };

			csv.get(elem.unique_id,    "id");

			Poseidon::JsonObject object;
			csv.get(object, "drop");
			elem.monster_rewards.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				auto collection_name = std::string(it->first.get());
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.monster_rewards.emplace(std::move(collection_name), count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate reward set: collection_name = ", collection_name);
					DEBUG_THROW(Exception, sslit("Duplicate reward set"));
				}
			}

			Poseidon::JsonArray array;
			csv.get(array, "monste_id");
			elem.monster_type_ids.reserve(array.size());
			for(auto it = array.begin(); it != array.end(); ++it){
				const auto monster_type_id = MapObjectTypeId(it->get<double>());
				if(!elem.monster_type_ids.insert(monster_type_id).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate monster: monster_type_id = ", monster_type_id);
					DEBUG_THROW(Exception, sslit("Duplicate monster"));
				}
			}

			std::string str;
			csv.get(str, "start_time");
			elem.available_since = Poseidon::scan_time(str.c_str());
			csv.get(str, "end_time");
			elem.available_until = Poseidon::scan_time(str.c_str());

			csv.get(elem.available_period,   "refresh_time");
			csv.get(elem.available_duration, "continued_time");

			if(!monster_reward_extra_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectTypeMonsterRewardExtra: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeMonsterRewardExtra"));
			}
		}
		g_monster_reward_extra_map = monster_reward_extra_map;
		handles.push(monster_reward_extra_map);
		servlet = DataSession::create_servlet(MONSTER_REWARD_EXTRA_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(ATTRIBUTE_BONUS_FILE);
		const auto attribute_bonus_map = boost::make_shared<AttributeBonusMap>();
		while(csv.fetch_row()){
			Data::MapObjectTypeAttributeBonus elem = { };

			csv.get(elem.unique_id,                "id");
			csv.get(elem.applicability_key.first,  "type");
			csv.get(elem.applicability_key.second, "arm_id");
			csv.get(elem.tech_attribute_id,        "science_id");
			csv.get(elem.bonus_attribute_id,       "attribute_id");

			if(!attribute_bonus_map->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate MapObjectTypeAttributeBonus: unique_id = ", elem.unique_id);
				DEBUG_THROW(Exception, sslit("Duplicate MapObjectTypeAttributeBonus"));
			}
		}
		g_attribute_bonus_map = attribute_bonus_map;
		handles.push(attribute_bonus_map);
		servlet = DataSession::create_servlet(ATTRIBUTE_BONUS_FILE, Data::encode_csv_as_json(csv, "id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const MapObjectTypeAbstract> MapObjectTypeAbstract::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		boost::shared_ptr<const MapObjectTypeAbstract> ret;
		if(!!(ret = MapObjectTypeBattalion::get(map_object_type_id))){
			return ret;
		}
		if(!!(ret = MapObjectTypeMonster::get(map_object_type_id))){
			return ret;
		}
		return ret;
	}
	boost::shared_ptr<const MapObjectTypeAbstract> MapObjectTypeAbstract::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeAbstract not found: map_object_type_id = ", map_object_type_id);;
			DEBUG_THROW(Exception, sslit("MapObjectTypeAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapObjectTypeBattalion> MapObjectTypeBattalion::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto battalion_map = g_battalion_map.lock();
		if(!battalion_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeBattalionMap has not been loaded.");
			return { };
		}

		const auto it = battalion_map->find(map_object_type_id);
		if(it == battalion_map->end()){
			LOG_EMPERY_CENTER_TRACE("MapObjectTypeBattalion not found: map_object_type_id = ", map_object_type_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeBattalion>(battalion_map, &(it->second));
	}
	boost::shared_ptr<const MapObjectTypeBattalion> MapObjectTypeBattalion::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeBattalion not found: map_object_type_id = ", map_object_type_id);
			DEBUG_THROW(Exception, sslit("MapObjectTypeBattalion not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapObjectTypeMonster> MapObjectTypeMonster::get(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		const auto monster_map = g_monster_map.lock();
		if(!monster_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterMap has not been loaded.");
			return { };
		}

		const auto it = monster_map->find(map_object_type_id);
		if(it == monster_map->end()){
			LOG_EMPERY_CENTER_TRACE("MapObjectTypeMonster not found: map_object_type_id = ", map_object_type_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeMonster>(monster_map, &(it->second));
	}
	boost::shared_ptr<const MapObjectTypeMonster> MapObjectTypeMonster::require(MapObjectTypeId map_object_type_id){
		PROFILE_ME;

		auto ret = get(map_object_type_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonster not found: map_object_type_id = ", map_object_type_id);
			DEBUG_THROW(Exception, sslit("MapObjectTypeMonster not found"));
		}
		return ret;
	}

	boost::shared_ptr<const MapObjectTypeMonsterReward> MapObjectTypeMonsterReward::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto monster_reward_map = g_monster_reward_map.lock();
		if(!monster_reward_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterRewardMap has not been loaded.");
			return { };
		}

		const auto it = monster_reward_map->find<0>(unique_id);
		if(it == monster_reward_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapObjectTypeMonsterReward not found: unique_id = ", unique_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeMonsterReward>(monster_reward_map, &*it);
	}
	boost::shared_ptr<const MapObjectTypeMonsterReward> MapObjectTypeMonsterReward::require(std::uint64_t unique_id){
		PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterReward not found: unique_id = ", unique_id);
			DEBUG_THROW(Exception, sslit("MapObjectTypeMonsterReward not found"));
		}
		return ret;
	}

	void MapObjectTypeMonsterReward::get_by_collection_name(std::vector<boost::shared_ptr<const MapObjectTypeMonsterReward>> &ret,
		const std::string &collection_name)
	{
		PROFILE_ME;

		const auto monster_reward_map = g_monster_reward_map.lock();
		if(!monster_reward_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterRewardMap has not been loaded.");
			return;
		}

		const auto range = monster_reward_map->equal_range<1>(collection_name);
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(monster_reward_map, &*it);
		}
	}
	boost::shared_ptr<const MapObjectTypeMonsterReward> MapObjectTypeMonsterReward::random_by_collection_name(const std::string &collection_name){
		PROFILE_ME;

		const auto monster_reward_map = g_monster_reward_map.lock();
		if(!monster_reward_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterRewardMap has not been loaded.");
			return { };
		}

		const auto range = monster_reward_map->equal_range<1>(collection_name);
		const auto count = static_cast<std::size_t>(std::distance(range.first, range.second));
		LOG_EMPERY_CENTER_DEBUG("Get a random monster reward: collection_name = ", collection_name, ", count = ", count);
		if(count == 1){
			const auto it = range.first;
			LOG_EMPERY_CENTER_DEBUG("Fast generation: unique_id = ", it->unique_id);
			return boost::shared_ptr<const MapObjectTypeMonsterReward>(monster_reward_map, &*it);
		} else {
			double weight_total = 0;
			for(auto it = range.first; it != range.second; ++it){
				weight_total += it->weight;
			}
			if(weight_total <= 0){
				LOG_EMPERY_CENTER_DEBUG("> Sum of weight is zero: collection_name = ", collection_name);
				return { };
			}
			auto random_weight = Poseidon::rand_double(0, weight_total);
			LOG_EMPERY_CENTER_DEBUG("> Generated a random weight: random_weight = ", random_weight, ", weight_total = ", weight_total);
			for(auto it = range.first; it != range.second; ++it){
				random_weight -= it->weight;
				if(random_weight <= 0){
					LOG_EMPERY_CENTER_DEBUG("Picking monster reward: unique_id = ", it->unique_id);
					return boost::shared_ptr<const MapObjectTypeMonsterReward>(monster_reward_map, &*it);
				}
			}
			return { };
		}
	}

	boost::shared_ptr<const MapObjectTypeMonsterRewardExtra> MapObjectTypeMonsterRewardExtra::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto monster_reward_extra_map = g_monster_reward_extra_map.lock();
		if(!monster_reward_extra_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterRewardExtraMap has not been loaded.");
			return { };
		}

		const auto it = monster_reward_extra_map->find<0>(unique_id);
		if(it == monster_reward_extra_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapObjectTypeMonsterRewardExtra not found: unique_id = ", unique_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeMonsterRewardExtra>(monster_reward_extra_map, &*it);
	}
	boost::shared_ptr<const MapObjectTypeMonsterRewardExtra> MapObjectTypeMonsterRewardExtra::require(std::uint64_t unique_id){
		PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterRewardExtra not found: unique_id = ", unique_id);
			DEBUG_THROW(Exception, sslit("MapObjectTypeMonsterRewardExtra not found"));
		}
		return ret;
	}

	void MapObjectTypeMonsterRewardExtra::get_available(std::vector<boost::shared_ptr<const MapObjectTypeMonsterRewardExtra>> &ret,
		std::uint64_t utc_now, MapObjectTypeId monster_type_id)
	{
		PROFILE_ME;

		const auto monster_reward_extra_map = g_monster_reward_extra_map.lock();
		if(!monster_reward_extra_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeMonsterRewardExtraMap has not been loaded.");
			return;
		}

		const auto range = std::make_pair(monster_reward_extra_map->begin<1>(), monster_reward_extra_map->upper_bound<1>(utc_now));
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			if(utc_now >= it->available_until){
				continue;
			}
			if(it->available_period != 0){
				const auto period   = checked_mul<std::uint64_t>(it->available_period,   60000);
				const auto duration = checked_mul<std::uint64_t>(it->available_duration, 60000);

				const auto begin_offset = it->available_since % period;
				const auto delta_from_offset = saturated_sub(utc_now, begin_offset) % period;
				if(delta_from_offset >= duration){
					continue;
				}
			}
			if(it->monster_type_ids.find(monster_type_id) == it->monster_type_ids.end()){
				continue;
			}
			ret.emplace_back(monster_reward_extra_map, &*it);
		}
	}

	boost::shared_ptr<const MapObjectTypeAttributeBonus> MapObjectTypeAttributeBonus::get(std::uint64_t unique_id){
		PROFILE_ME;

		const auto attribute_bonus_map = g_attribute_bonus_map.lock();
		if(!attribute_bonus_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeAttributeBonusMap has not been loaded.");
			return { };
		}

		const auto it = attribute_bonus_map->find<0>(unique_id);
		if(it == attribute_bonus_map->end<0>()){
			LOG_EMPERY_CENTER_TRACE("MapObjectTypeAttributeBonus not found: unique_id = ", unique_id);
			return { };
		}
		return boost::shared_ptr<const MapObjectTypeAttributeBonus>(attribute_bonus_map, &*it);
	}
	boost::shared_ptr<const MapObjectTypeAttributeBonus> MapObjectTypeAttributeBonus::require(std::uint64_t unique_id){
		PROFILE_ME;

		auto ret = get(unique_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeAttributeBonus not found: unique_id = ", unique_id);
			DEBUG_THROW(Exception, sslit("MapObjectTypeAttributeBonus not found"));
		}
		return ret;
	}

	void MapObjectTypeAttributeBonus::get_applicable(std::vector<boost::shared_ptr<const MapObjectTypeAttributeBonus>> &ret,
		MapObjectTypeAttributeBonus::ApplicabilityKeyType applicability_key_type, std::uint64_t applicability_key_value)
	{
		PROFILE_ME;

		const auto attribute_bonus_map = g_attribute_bonus_map.lock();
		if(!attribute_bonus_map){
			LOG_EMPERY_CENTER_WARNING("MapObjectTypeAttributeBonusMap has not been loaded.");
			return;
		}

		const auto range = attribute_bonus_map->equal_range<1>(
			std::make_pair(static_cast<unsigned>(applicability_key_type), applicability_key_value));
		ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(range.first, range.second)));
		for(auto it = range.first; it != range.second; ++it){
			ret.emplace_back(attribute_bonus_map, &*it);
		}
	}
}

}
