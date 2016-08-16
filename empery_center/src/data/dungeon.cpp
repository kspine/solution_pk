#include "../precompiled.hpp"
#include "dungeon.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	MULTI_INDEX_MAP(DungeonContainer, Data::Dungeon,
		UNIQUE_MEMBER_INDEX(dungeon_type_id)
	)
	boost::weak_ptr<const DungeonContainer> g_dungeon_container;
	const char DUNGEON_FILE[] = "dungeon";

	MULTI_INDEX_MAP(TaskContainer, Data::DungeonTask,
		UNIQUE_MEMBER_INDEX(dungeon_task_id)
	)
	boost::weak_ptr<const TaskContainer> g_task_container;
	const char TASK_FILE[] = "dungeon_task";

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(DUNGEON_FILE);
		const auto dungeon_container = boost::make_shared<DungeonContainer>();
		while(csv.fetch_row()){
			Data::Dungeon elem = { };

			csv.get(elem.dungeon_type_id,              "dungeon_id");
			csv.get(elem.reentrant,                    "dungeon_class");
			csv.get(elem.prerequisite_dungeon_type_id, "dungeon_need");

			Poseidon::JsonObject object;
			csv.get(object, "dungeon_resource");
			elem.entry_cost.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.entry_cost.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate entry cost: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate entry cost"));
				}
			}

			Poseidon::JsonArray array;
			csv.get(array, "dungeon_limit");
			elem.start_points.reserve(array.size());
			for(auto it = array.begin(); it != array.end(); ++it){
				const auto &point_array = it->get<Poseidon::JsonArray>();
				const auto x = static_cast<std::int32_t>(point_array.at(0).get<double>());
				const auto y = static_cast<std::int32_t>(point_array.at(1).get<double>());
				elem.start_points.emplace_back(x, y);
			}

			array.clear();
			csv.get(array, "dungeon_limit_min_max");
			elem.soldier_count_limits.first  = static_cast<std::uint64_t>(array.at(0).get<double>());
			elem.soldier_count_limits.second = static_cast<std::uint64_t>(array.at(1).get<double>());

			object.clear();
			csv.get(object, "dungeon_limit_y");
			elem.battalions_required.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto param = boost::lexical_cast<std::uint64_t>(it->first);
				const auto type = static_cast<Data::ApplicabilityKeyType>(it->second.get<double>());
				elem.battalions_required.emplace(type, param);
			}

			object.clear();
			csv.get(object, "dungeon_limit_n");
			elem.battalions_forbidden.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto param = boost::lexical_cast<std::uint64_t>(it->first);
				const auto type = static_cast<Data::ApplicabilityKeyType>(it->second.get<double>());
				elem.battalions_forbidden.emplace(type, param);
			}

			array.clear();
			csv.get(array, "dungeon_task_id");
			elem.tasks.reserve(array.size());
			for(auto it = array.begin(); it != array.end(); ++it){
				const auto task_id = DungeonTaskId(it->get<double>());
				if(!elem.tasks.insert(task_id).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate dungeon task: task_id = ", task_id);
					DEBUG_THROW(Exception, sslit("Duplicate dungeon task"));
				}
			}

			object.clear();
			csv.get(object, "dungeon_reward");
			elem.rewards.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				auto collection_name = std::string(it->first.get());
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.rewards.emplace(std::move(collection_name), count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate reward set: collection_name = ", collection_name);
					DEBUG_THROW(Exception, sslit("Duplicate reward set"));
				}
			}

			csv.get(elem.resuscitation_ratio, "resurrection");

			if(!dungeon_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate Dungeon: dungeon_type_id = ", elem.dungeon_type_id);
				DEBUG_THROW(Exception, sslit("Duplicate Dungeon"));
			}
		}
		g_dungeon_container = dungeon_container;
		handles.push(dungeon_container);
		auto servlet = DataSession::create_servlet(DUNGEON_FILE, Data::encode_csv_as_json(csv, "dungeon_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TASK_FILE);
		const auto task_container = boost::make_shared<TaskContainer>();
		while(csv.fetch_row()){
			Data::DungeonTask elem = { };

			csv.get(elem.dungeon_task_id, "dungeon_task_id");

			Poseidon::JsonObject object;
			csv.get(object, "dungeon_task_reward");
			elem.rewards.reserve(object.size());
			for(auto it = object.begin(); it != object.end(); ++it){
				const auto item_id = boost::lexical_cast<ItemId>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				if(!elem.rewards.emplace(item_id, count).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate reward: item_id = ", item_id);
					DEBUG_THROW(Exception, sslit("Duplicate reward"));
				}
			}

			if(!task_container->insert(std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate DungeonTask: dungeon_task_id = ", elem.dungeon_task_id);
				DEBUG_THROW(Exception, sslit("Duplicate DungeonTask"));
			}
		}
		g_task_container = task_container;
		handles.push(task_container);
		servlet = DataSession::create_servlet(TASK_FILE, Data::encode_csv_as_json(csv, "dungeon_task_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const Dungeon> Dungeon::get(DungeonTypeId dungeon_type_id){
		PROFILE_ME;

		const auto dungeon_container = g_dungeon_container.lock();
		if(!dungeon_container){
			LOG_EMPERY_CENTER_WARNING("DungeonContainer has not been loaded.");
			return { };
		}

		const auto it = dungeon_container->find<0>(dungeon_type_id);
		if(it == dungeon_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("Dungeon not found: dungeon_type_id = ", dungeon_type_id);
			return { };
		}
		return boost::shared_ptr<const Dungeon>(dungeon_container, &*it);
	}
	boost::shared_ptr<const Dungeon> Dungeon::require(DungeonTypeId dungeon_type_id){
		PROFILE_ME;

		auto ret = get(dungeon_type_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("Dungeon not found: dungeon_type_id = ", dungeon_type_id);
			DEBUG_THROW(Exception, sslit("Dungeon not found"));
		}
		return ret;
	}

	boost::shared_ptr<const DungeonTask> DungeonTask::get(DungeonTaskId dungeon_task_id){
		PROFILE_ME;

		const auto task_container = g_task_container.lock();
		if(!task_container){
			LOG_EMPERY_CENTER_WARNING("DungeonContainer has not been loaded.");
			return { };
		}

		const auto it = task_container->find<0>(dungeon_task_id);
		if(it == task_container->end<0>()){
			LOG_EMPERY_CENTER_TRACE("Dungeon not found: dungeon_task_id = ", dungeon_task_id);
			return { };
		}
		return boost::shared_ptr<const DungeonTask>(task_container, &*it);
	}
	boost::shared_ptr<const DungeonTask> DungeonTask::require(DungeonTaskId dungeon_task_id){
		PROFILE_ME;

		auto ret = get(dungeon_task_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("Dungeon not found: dungeon_task_id = ", dungeon_task_id);
			DEBUG_THROW(Exception, sslit("Dungeon not found"));
		}
		return ret;
	}
}

}
