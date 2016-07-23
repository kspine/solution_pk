#include "../precompiled.hpp"
#include "task.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter {

namespace {
	using TaskPrimaryContainer = boost::container::flat_map<TaskId, Data::TaskPrimary>;
	boost::weak_ptr<const TaskPrimaryContainer> g_task_primary_container;
	const char TASK_PRIMARY_FILE[] = "task";

	using TaskDailyContainer = boost::container::flat_map<TaskId, Data::TaskDaily>;
	boost::weak_ptr<const TaskDailyContainer> g_task_daily_container;
	const char TASK_DAILY_FILE[] = "task_everyday";

	template<typename ElementT>
	void read_task_abstract(ElementT &elem, const Poseidon::CsvParser &csv){
		csv.get(elem.task_id,         "task_id");
		csv.get(elem.castle_category, "task_city");
		csv.get(elem.type,            "task_type");
		csv.get(elem.accumulative,    "accumulation");

		Poseidon::JsonObject object;
		csv.get(object, "task_need");
		elem.objective.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto key = boost::lexical_cast<std::uint64_t>(it->first);
			const auto &value_array = it->second.get<Poseidon::JsonArray>();
			std::vector<double> values;
			values.reserve(value_array.size());
			for(auto vit = value_array.begin(); vit != value_array.end(); ++vit){
				values.emplace_back(vit->get<double>());
			}
			if(!elem.objective.emplace(key, std::move(values)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate task objective: key = ", key);
				DEBUG_THROW(Exception, sslit("Duplicate task objective"));
			}
		}

		object.clear();
		csv.get(object, "task_reward");
		elem.reward.reserve(object.size());
		for(auto it = object.begin(); it != object.end(); ++it){
			const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
			const auto count = static_cast<std::uint64_t>(it->second.get<double>());
			if(!elem.reward.emplace(resource_id, count).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate task reward: resource_id = ", resource_id);
				DEBUG_THROW(Exception, sslit("Duplicate task reward"));
			}
		}
	}

	MODULE_RAII_PRIORITY(handles, 1000){
		auto csv = Data::sync_load_data(TASK_PRIMARY_FILE);
		const auto task_primary_container = boost::make_shared<TaskPrimaryContainer>();
		while(csv.fetch_row()){
			Data::TaskPrimary elem = { };

			read_task_abstract(elem, csv);

			csv.get(elem.previous_id, "task_last");

			if(!task_primary_container->emplace(elem.task_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate TaskPrimary: task_id = ", elem.task_id);
				DEBUG_THROW(Exception, sslit("Duplicate TaskPrimary"));
			}
		}
		g_task_primary_container = task_primary_container;
		handles.push(task_primary_container);
		auto servlet = DataSession::create_servlet(TASK_PRIMARY_FILE, Data::encode_csv_as_json(csv, "task_id"));
		handles.push(std::move(servlet));

		csv = Data::sync_load_data(TASK_DAILY_FILE);
		const auto task_daily_container = boost::make_shared<TaskDailyContainer>();
		while(csv.fetch_row()){
			Data::TaskDaily elem = { };

			read_task_abstract(elem, csv);

			Poseidon::JsonArray array;
			csv.get(array, "task_class");
			elem.level_limit_min = array.at(0).get<double>();
			elem.level_limit_max = array.at(1).get<double>();

			if(!task_daily_container->emplace(elem.task_id, std::move(elem)).second){
				LOG_EMPERY_CENTER_ERROR("Duplicate TaskDaily: task_id = ", elem.task_id);
				DEBUG_THROW(Exception, sslit("Duplicate TaskDaily"));
			}
		}
		g_task_daily_container = task_daily_container;
		handles.push(task_daily_container);
		servlet = DataSession::create_servlet(TASK_DAILY_FILE, Data::encode_csv_as_json(csv, "task_id"));
		handles.push(std::move(servlet));
	}
}

namespace Data {
	boost::shared_ptr<const TaskAbstract> TaskAbstract::get(TaskId task_id){
		PROFILE_ME;

		boost::shared_ptr<const TaskAbstract> ret;
		if(!!(ret = TaskPrimary::get(task_id))){
			return ret;
		}
		if(!!(ret = TaskDaily::get(task_id))){
			return ret;
		}
		return ret;
	}
	boost::shared_ptr<const TaskAbstract> TaskAbstract::require(TaskId task_id){
		PROFILE_ME;

		auto ret = get(task_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("TaskAbstract not found: task_id = ", task_id);
			DEBUG_THROW(Exception, sslit("TaskAbstract not found"));
		}
		return ret;
	}

	boost::shared_ptr<const TaskPrimary> TaskPrimary::get(TaskId task_id){
		PROFILE_ME;

		const auto task_primary_container = g_task_primary_container.lock();
		if(!task_primary_container){
			LOG_EMPERY_CENTER_WARNING("TaskPrimaryContainer has not been loaded.");
			return { };
		}

		const auto it = task_primary_container->find(task_id);
		if(it == task_primary_container->end()){
			LOG_EMPERY_CENTER_TRACE("TaskPrimary not found: task_id = ", task_id);
			return { };
		}
		return boost::shared_ptr<const TaskPrimary>(task_primary_container, &(it->second));
	}
	boost::shared_ptr<const TaskPrimary> TaskPrimary::require(TaskId task_id){
		PROFILE_ME;

		auto ret = get(task_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("TaskPrimary not found: task_id = ", task_id);
			DEBUG_THROW(Exception, sslit("TaskPrimary not found"));
		}
		return ret;
	}

	void TaskPrimary::get_all(std::vector<boost::shared_ptr<const TaskPrimary>> &ret){
		PROFILE_ME;

		const auto task_primary_container = g_task_primary_container.lock();
		if(!task_primary_container){
			LOG_EMPERY_CENTER_WARNING("TaskPrimaryContainer has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + task_primary_container->size());
		for(auto it = task_primary_container->begin(); it != task_primary_container->end(); ++it){
			ret.emplace_back(task_primary_container, &(it->second));
		}
	}

	boost::shared_ptr<const TaskDaily> TaskDaily::get(TaskId task_id){
		PROFILE_ME;

		const auto task_daily_container = g_task_daily_container.lock();
		if(!task_daily_container){
			LOG_EMPERY_CENTER_WARNING("TaskDailyContainer has not been loaded.");
			return { };
		}

		const auto it = task_daily_container->find(task_id);
		if(it == task_daily_container->end()){
			LOG_EMPERY_CENTER_TRACE("TaskDaily not found: task_id = ", task_id);
			return { };
		}
		return boost::shared_ptr<const TaskDaily>(task_daily_container, &(it->second));
	}
	boost::shared_ptr<const TaskDaily> TaskDaily::require(TaskId task_id){
		PROFILE_ME;

		auto ret = get(task_id);
		if(!ret){
			LOG_EMPERY_CENTER_WARNING("TaskDaily not found: task_id = ", task_id);
			DEBUG_THROW(Exception, sslit("TaskDaily not found"));
		}
		return ret;
	}

	void TaskDaily::get_all(std::vector<boost::shared_ptr<const TaskDaily>> &ret){
		PROFILE_ME;

		const auto task_daily_container = g_task_daily_container.lock();
		if(!task_daily_container){
			LOG_EMPERY_CENTER_WARNING("TaskDailyContainer has not been loaded.");
			return;
		}

		ret.reserve(ret.size() + task_daily_container->size());
		for(auto it = task_daily_container->begin(); it != task_daily_container->end(); ++it){
			ret.emplace_back(task_daily_container, &(it->second));
		}
	}
}

}
