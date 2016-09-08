#include "../precompiled.hpp"
#include "task.hpp"
#include <poseidon/multi_index_map.hpp>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
	namespace
	{
		using TaskPrimaryContainer = boost::container::flat_map<TaskId, Data::TaskPrimary>;
		boost::weak_ptr<const TaskPrimaryContainer> g_task_primary_container;
		const char TASK_PRIMARY_FILE[] = "task";

		using TaskDailyContainer = boost::container::flat_map<TaskId, Data::TaskDaily>;
		boost::weak_ptr<const TaskDailyContainer> g_task_daily_container;

		///***********************************************************************************************/
		//using TaskLegionPackageContainer = boost::container::flat_map<TaskId, Data::TaskLegionPackage>;
		//boost::weak_ptr<const TaskLegionPackageContainer> g_task_legion_package_container;
		///***********************************************************************************************/

		const char TASK_LEGION_FILE[] = "corps_task";
		using TaskLegionContainer = boost::container::flat_map<TaskId, Data::TaskLegion>;
		boost::weak_ptr<const TaskLegionContainer> g_task_legion_container;

		const char TASK_DAILY_FILE[] = "task_everyday";

		template <typename ElementT>
		void read_task_abstract(ElementT& elem, const Poseidon::CsvParser& csv,bool read_award = true)
		{
			csv.get(elem.task_id, "task_id");
			csv.get(elem.castle_category, "task_city");
			csv.get(elem.type, "task_type");
			csv.get(elem.accumulative, "accumulation");

			Poseidon::JsonObject object;
			csv.get(object, "task_need");
			elem.objective.reserve(object.size());
			for (auto it = object.begin(); it != object.end(); ++it)
			{
				const auto key = boost::lexical_cast<std::uint64_t>(it->first);
				const auto& value_array = it->second.get<Poseidon::JsonArray>();
				std::vector<double> values;
				values.reserve(value_array.size());
				for (auto vit = value_array.begin(); vit != value_array.end(); ++vit)
				{
					values.emplace_back(vit->get<double>());
				}
				if (!elem.objective.emplace(key, std::move(values)).second)
				{
					LOG_EMPERY_CENTER_ERROR("Duplicate task objective: key = ", key);
					DEBUG_THROW(Exception, sslit("Duplicate task objective"));
				}
			}

			object.clear();
			if(read_award){
				csv.get(object, "task_reward");
				elem.rewards.reserve(object.size());
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.rewards.emplace(resource_id, count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate task reward: resource_id = ", resource_id);
						DEBUG_THROW(Exception, sslit("Duplicate task reward"));
					}
				}
			}
		}

		const char TASK_LEGION_CONTRIBUTION_FILE[] = "corps_task_contribution";
		using TaskLegionContributionContainer = boost::container::flat_map<TaskLegionKeyId, Data::TaskLegionContribution>;
		boost::weak_ptr<const TaskLegionContributionContainer> g_task_legion_contribution_container;

		MODULE_RAII_PRIORITY(handles, 1000)
		{
			auto csv = Data::sync_load_data(TASK_PRIMARY_FILE);
			const auto task_primary_container = boost::make_shared<TaskPrimaryContainer>();
			while (csv.fetch_row())
			{
				Data::TaskPrimary elem = {};

				read_task_abstract(elem, csv);

				csv.get(elem.previous_id, "task_last");

				if (!task_primary_container->emplace(elem.task_id, std::move(elem)).second)
				{
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
			while (csv.fetch_row())
			{
				Data::TaskDaily elem = {};
				csv.get(elem.task_class_1, "task_class_1");

				read_task_abstract(elem, csv);

				Poseidon::JsonArray array;
				csv.get(array, "task_class");
				elem.level_limit_min = array.at(0).get<double>();
				elem.level_limit_max = array.at(1).get<double>();

				if (!task_daily_container->emplace(elem.task_id, std::move(elem)).second)
				{
					DEBUG_THROW(Exception, sslit("Duplicate TaskDaily"));
				}
			}
			g_task_daily_container = task_daily_container;
			handles.push(task_daily_container);

			///***********************************************************************************************/
			//csv = Data::sync_load_data(TASK_DAILY_FILE);
			//const auto task_legion_package_container = boost::make_shared<TaskLegionPackageContainer>();
			//while (csv.fetch_row())
			//{
			//	Data::TaskLegionPackage elem = {};
			//	csv.get(elem.task_class_1, "task_class_1");

			//	read_task_abstract(elem, csv);

			//	Poseidon::JsonArray array;
			//	csv.get(array, "task_class");

			//	elem.level_limit_min = array.at(0).get<double>();
			//	elem.level_limit_max = array.at(1).get<double>();

			//	if (elem.task_class_1 == (uint64_t)Data::TaskLegionPackage::ETaskLegionPackage_Class)
			//	{
			//		if (!task_legion_package_container->emplace(elem.task_id, std::move(elem)).second)
			//		{
			//			DEBUG_THROW(Exception, sslit("Duplicate TaskDaily"));
			//		}
			//	}
			//}

			//g_task_legion_package_container = task_legion_package_container;
			//handles.push(task_legion_package_container);

			///***********************************************************************************************/
			servlet = DataSession::create_servlet(TASK_DAILY_FILE, Data::encode_csv_as_json(csv, "task_id"));
			handles.push(std::move(servlet));

			csv = Data::sync_load_data(TASK_LEGION_FILE);
			const auto task_legion_container = boost::make_shared<TaskLegionContainer>();
			while (csv.fetch_row())
			{
				Data::TaskLegion elem = {};
				read_task_abstract(elem, csv,false);
				Poseidon::JsonArray array;
				csv.get(array, "task_class");
				elem.level_limit_min = array.at(0).get<double>();
				elem.level_limit_max = array.at(1).get<double>();
				std::uint64_t stage1 = 0,stage2 = 0, stage3 = 0, stage4 = 0;
				csv.get(stage1, "task_stage1");
				csv.get(stage2, "task_stage2");
				csv.get(stage3, "task_stage3");
				csv.get(stage4, "task_stage4");
				Poseidon::JsonObject object;
				std::vector<std::pair<ResourceId, std::uint64_t>> rewards;
				csv.get(object, "reward1_id1");
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					rewards.push_back(std::make_pair(resource_id,count));
				}
				if(!elem.stage_reward.emplace(stage1, rewards).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate task reward: stage1 = ", stage1);
					DEBUG_THROW(Exception, sslit("Duplicate task stage1 reward"));
				}

				object.clear();
				rewards.clear();
				csv.get(object, "reward1_id2");
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					rewards.push_back(std::make_pair(resource_id,count));
				}
				if(!elem.stage_reward.emplace(stage2, rewards).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate task reward: stage2 = ", stage2);
					DEBUG_THROW(Exception, sslit("Duplicate task stage2 reward"));
				}

				object.clear();
				rewards.clear();
				csv.get(object, "reward1_id3");
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					rewards.push_back(std::make_pair(resource_id,count));
				}
				if(!elem.stage_reward.emplace(stage3, rewards).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate task reward: stage3 = ", stage3);
					DEBUG_THROW(Exception, sslit("Duplicate task stage3 reward"));
				}

				object.clear();
				rewards.clear();
				csv.get(object, "reward1_id4");
				for (auto it = object.begin(); it != object.end(); ++it)
				{
					const auto resource_id = boost::lexical_cast<ResourceId>(it->first);
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					rewards.push_back(std::make_pair(resource_id,count));
				}
				if(!elem.stage_reward.emplace(stage4, rewards).second){
					LOG_EMPERY_CENTER_ERROR("Duplicate task reward: stage4 = ", stage4);
					DEBUG_THROW(Exception, sslit("Duplicate task stage4 reward"));
				}

				if (!task_legion_container->emplace(elem.task_id, std::move(elem)).second)
				{
					LOG_EMPERY_CENTER_ERROR("Duplicate TaskLegion: task_id = ", elem.task_id);
					DEBUG_THROW(Exception, sslit("Duplicate TaskLegion"));
				}
			}
			g_task_legion_container = task_legion_container;
			handles.push(task_legion_container);
			servlet = DataSession::create_servlet(TASK_LEGION_FILE, Data::encode_csv_as_json(csv, "task_id"));
			handles.push(std::move(servlet));

			csv = Data::sync_load_data(TASK_LEGION_CONTRIBUTION_FILE);
			const auto task_legion_contribution_container = boost::make_shared<TaskLegionContributionContainer>();
			while (csv.fetch_row())
			{
				Data::TaskLegionContribution elem = {};
				csv.get(elem.task_legion_key_id, "contribution_id");
				csv.get(elem.out_number, "output_number");
				csv.get(elem.legion_number, "corps_number");
				csv.get(elem.personal_number, "personal_number");
				if (!task_legion_contribution_container->emplace(elem.task_legion_key_id, std::move(elem)).second)
				{
					LOG_EMPERY_CENTER_ERROR("Duplicate TaskLegion contribution: task_legion_key_id = ", elem.task_legion_key_id);
					DEBUG_THROW(Exception, sslit("Duplicate TaskLegion contribution"));
				}
			}
			g_task_legion_contribution_container = task_legion_contribution_container;
			handles.push(task_legion_contribution_container);
			servlet = DataSession::create_servlet(TASK_LEGION_CONTRIBUTION_FILE, Data::encode_csv_as_json(csv, "contribution_id"));
			handles.push(std::move(servlet));
		}
	}

	namespace Data
	{
		boost::shared_ptr<const TaskAbstract> TaskAbstract::get(TaskId task_id)
		{
			PROFILE_ME;

			boost::shared_ptr<const TaskAbstract> ret;
			if (!!(ret = TaskPrimary::get(task_id)))
			{
				return ret;
			}
			if (!!(ret = TaskDaily::get(task_id)))
			{
				return ret;
			}
			return ret;
		}

		boost::shared_ptr<const TaskAbstract> TaskAbstract::require(TaskId task_id)
		{
			PROFILE_ME;

			auto ret = get(task_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("TaskAbstract not found: task_id = ", task_id);
				DEBUG_THROW(Exception, sslit("TaskAbstract not found"));
			}
			return ret;
		}

		boost::shared_ptr<const TaskPrimary> TaskPrimary::get(TaskId task_id)
		{
			PROFILE_ME;

			const auto task_primary_container = g_task_primary_container.lock();
			if (!task_primary_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskPrimaryContainer has not been loaded.");
				return{};
			}

			const auto it = task_primary_container->find(task_id);
			if (it == task_primary_container->end())
			{
				LOG_EMPERY_CENTER_TRACE("TaskPrimary not found: task_id = ", task_id);
				return{};
			}
			return boost::shared_ptr<const TaskPrimary>(task_primary_container, &(it->second));
		}

		boost::shared_ptr<const TaskPrimary> TaskPrimary::require(TaskId task_id)
		{
			PROFILE_ME;

			auto ret = get(task_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("TaskPrimary not found: task_id = ", task_id);
				DEBUG_THROW(Exception, sslit("TaskPrimary not found"));
			}
			return ret;
		}

		void TaskPrimary::get_all(std::vector<boost::shared_ptr<const TaskPrimary>>& ret)
		{
			PROFILE_ME;

			const auto task_primary_container = g_task_primary_container.lock();
			if (!task_primary_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskPrimaryContainer has not been loaded.");
				return;
			}

			ret.reserve(ret.size() + task_primary_container->size());
			for (auto it = task_primary_container->begin(); it != task_primary_container->end(); ++it)
			{
				ret.emplace_back(task_primary_container, &(it->second));
			}
		}

		boost::shared_ptr<const TaskDaily> TaskDaily::get(TaskId task_id)
		{
			PROFILE_ME;

			const auto task_daily_container = g_task_daily_container.lock();
			if (!task_daily_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskDailyContainer has not been loaded.");
				return{};
			}

			const auto it = task_daily_container->find(task_id);
			if (it == task_daily_container->end())
			{
				LOG_EMPERY_CENTER_TRACE("TaskDaily not found: task_id = ", task_id);
				return{};
			}
			return boost::shared_ptr<const TaskDaily>(task_daily_container, &(it->second));
		}

		boost::shared_ptr<const TaskDaily> TaskDaily::require(TaskId task_id)
		{
			PROFILE_ME;

			auto ret = get(task_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("TaskDaily not found: task_id = ", task_id);
				DEBUG_THROW(Exception, sslit("TaskDaily not found"));
			}
			return ret;
		}

		void TaskDaily::get_all(std::vector<boost::shared_ptr<const TaskDaily>>& ret)
		{
			PROFILE_ME;

			const auto task_daily_container = g_task_daily_container.lock();
			if (!task_daily_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskDailyContainer has not been loaded.");
				return;
			}

			ret.reserve(ret.size() + task_daily_container->size());
			for (auto it = task_daily_container->begin(); it != task_daily_container->end(); ++it)
			{
				ret.emplace_back(task_daily_container, &(it->second));
			}
		}

		void TaskDaily::get_all_legion_package_task(std::vector<boost::shared_ptr<const TaskDaily>>& ret)
		{
			PROFILE_ME;

			const auto task_daily_container = g_task_daily_container.lock();
			if (!task_daily_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskDailyContainer has not been loaded.");
				return;
			}

			ret.reserve(ret.size() + task_daily_container->size());
			for (auto it = task_daily_container->begin(); it != task_daily_container->end(); ++it)
			{
				if ((it->second).task_class_1 == ETaskLegionPackage_Class)
				{
					ret.emplace_back(task_daily_container, &(it->second));
				}
			}
		}

		std::uint64_t TaskDaily::get_legion_package_task_num()
		{
			PROFILE_ME;

			std::vector<boost::shared_ptr<const TaskDaily>> ret;

			get_all_legion_package_task(ret);

			return ret.size();
		}
		///***********************************************************************************************/
		//boost::shared_ptr<const TaskLegionPackage> TaskLegionPackage::get(TaskId task_id)
		//{
		//	PROFILE_ME;

		//	const auto task_legion_package_container = g_task_legion_package_container.lock();
		//	if (!task_legion_package_container)
		//	{
		//		LOG_EMPERY_CENTER_WARNING("task_legion_package_container has not been loaded.");
		//		return{};
		//	}

		//	const auto it = task_legion_package_container->find(task_id);
		//	if (it == task_legion_package_container->end())
		//	{
		//		LOG_EMPERY_CENTER_TRACE("task_legion_package_container not found: task_id = ", task_id);
		//		return{};
		//	}
		//	return boost::shared_ptr<const TaskLegionPackage>(task_legion_package_container, &(it->second));
		//}

		//boost::shared_ptr<const TaskLegionPackage> TaskLegionPackage::require(TaskId task_id)
		//{
		//	PROFILE_ME;

		//	auto ret = get(task_id);
		//	if (!ret)
		//	{
		//		LOG_EMPERY_CENTER_WARNING("TaskLegionPackage not found: task_id = ", task_id);
		//		DEBUG_THROW(Exception, sslit("TaskLegionPackage not found"));
		//	}
		//	return ret;
		//}

		//void TaskLegionPackage::get_all(std::vector<boost::shared_ptr<const TaskLegionPackage>>& ret)
		//{
		//	PROFILE_ME;

		//	const auto task_legion_package_container = g_task_legion_package_container.lock();
		//	if (!task_legion_package_container)
		//	{
		//		LOG_EMPERY_CENTER_WARNING("task_legion_package_container has not been loaded.");
		//		return;
		//	}

		//	LOG_EMPERY_CENTER_ERROR(" ret.size()->", ret.size(), "task_legion_package_container->size()->", task_legion_package_container->size());
		//	ret.reserve(ret.size() + task_legion_package_container->size());
		//	for (auto it = task_legion_package_container->begin(); it != task_legion_package_container->end(); ++it)
		//	{
		//		LOG_EMPERY_CENTER_ERROR(" TaskLegionPackage::get_all");
		//		ret.emplace_back(task_legion_package_container, &(it->second));
		//	}
		//}

		//std::uint64_t TaskLegionPackage::get_legion_package_task_num()
		//{
		//	PROFILE_ME;

		//	std::vector<boost::shared_ptr<const TaskLegionPackage>> ret;

		//	get_all(ret);

		//	return ret.size();
		//}
		///***********************************************************************************************/

		boost::shared_ptr<const TaskLegion> TaskLegion::get(TaskId task_id){
			PROFILE_ME;

			const auto task_legion_container = g_task_legion_container.lock();
			if (!task_legion_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskLegionContainer has not been loaded.");
				return{};
			}

			const auto it = task_legion_container->find(task_id);
			if (it == task_legion_container->end())
			{
				LOG_EMPERY_CENTER_TRACE("TaskLegion not found: task_id = ", task_id);
				return{};
			}
			return boost::shared_ptr<const TaskLegion>(task_legion_container, &(it->second));
		}

		boost::shared_ptr<const TaskLegion> TaskLegion::require(TaskId task_id){
			PROFILE_ME;

			auto ret = get(task_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("TaskLegion not found: task_id = ", task_id);
				DEBUG_THROW(Exception, sslit("TaskLegion not found"));
			}
			return ret;
		}

		void TaskLegion::get_all(std::vector<boost::shared_ptr<const TaskLegion>>& ret){
			PROFILE_ME;

			const auto task_legion_container = g_task_legion_container.lock();
			if (!task_legion_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskLegionContainer has not been loaded.");
				return;
			}

			ret.reserve(ret.size() + task_legion_container->size());
			for (auto it = task_legion_container->begin(); it != task_legion_container->end(); ++it)
			{
				ret.emplace_back(task_legion_container, &(it->second));
			}
		}
		boost::shared_ptr<const TaskLegionContribution> TaskLegionContribution::get(TaskLegionKeyId task_legion_key_id){
			PROFILE_ME;

			const auto task_legion_contribution_container = g_task_legion_contribution_container.lock();
			if (!task_legion_contribution_container)
			{
				LOG_EMPERY_CENTER_WARNING("TaskLegionContributionContainer has not been loaded.");
				return{};
			}

			const auto it = task_legion_contribution_container->find(task_legion_key_id);
			if (it == task_legion_contribution_container->end())
			{
				LOG_EMPERY_CENTER_TRACE("TaskLegionContribution not found: task_legion_key_id = ", task_legion_key_id);
				return{};
			}
			return boost::shared_ptr<const TaskLegionContribution>(task_legion_contribution_container, &(it->second));
		}

		boost::shared_ptr<const TaskLegionContribution> TaskLegionContribution::require(TaskLegionKeyId task_legion_key_id){
			PROFILE_ME;

			auto ret = get(task_legion_key_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("TaskLegionContribution not found: task_legion_key_id = ", task_legion_key_id);
				DEBUG_THROW(Exception, sslit("TaskLegionContribution not found"));
			}
			return ret;
		}
	}
}