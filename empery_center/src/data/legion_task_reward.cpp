#include "../precompiled.hpp"
#include "legion_task_reward.hpp"
#include <poseidon/multi_index_map.hpp>
#include <string.h>
#include <poseidon/csv_parser.hpp>
#include <poseidon/json.hpp>
#include "../data_session.hpp"

namespace EmperyCenter
{
	namespace
	{
		MULTI_INDEX_MAP(LegionTaskRewardContainer, Data::LegionTaskReward,
			UNIQUE_MEMBER_INDEX(task_id))

			boost::weak_ptr<const LegionTaskRewardContainer> g_legionTaskReward_container;
		const char CORPS_TASK_FILE[] = "corps_task";

		MODULE_RAII_PRIORITY(handles, 1000)
		{
			/*
			auto csv = Data::sync_load_data(CORPS_TASK_FILE);
			const auto legionTaskReward_container = boost::make_shared<LegionTaskRewardContainer>();
			while (csv.fetch_row())
			{
				Data::LegionTaskReward elem = {};

				csv.get(elem.task_id, "task_id");
				csv.get(elem.task_stage_1, "task_stage_1");
				csv.get(elem.task_stage_2, "task_stage_2");
				csv.get(elem.task_stage_3, "task_stage_3");
				csv.get(elem.task_stage_4, "task_stage_4");

				Poseidon::JsonObject object;

				//军团任务 礼包奖励1
				csv.get(object, "reward1_id1");
				elem.task_package_reward_1_map.reserve(object.size());

				for (auto it = object.begin(); it != object.end(); ++it)
				{
					auto collection_name = std::string(it->first.get());
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.task_package_reward_1_map.emplace(std::move(collection_name), count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate task_package_reward_1_map : collection_name = ", collection_name);
						DEBUG_THROW(Exception, sslit("Duplicate task_package_reward_1_map "));
					}
				}
				//军团任务 礼包奖励2
				csv.get(object, "reward1_id1");
				elem.task_package_reward_2_map.reserve(object.size());

				for (auto it = object.begin(); it != object.end(); ++it)
				{
					auto collection_name = std::string(it->first.get());
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.task_package_reward_2_map.emplace(std::move(collection_name), count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate task_package_reward_2_map : collection_name = ", collection_name);
						DEBUG_THROW(Exception, sslit("Duplicate task_package_reward_2_map "));
					}
				}
				//军团任务 礼包奖励3
				csv.get(object, "reward1_id1");
				elem.task_package_reward_3_map.reserve(object.size());

				for (auto it = object.begin(); it != object.end(); ++it)
				{
					auto collection_name = std::string(it->first.get());
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.task_package_reward_3_map.emplace(std::move(collection_name), count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate task_package_reward_3_map : collection_name = ", collection_name);
						DEBUG_THROW(Exception, sslit("Duplicate task_package_reward_3_map "));
					}
				}
				//军团任务 礼包奖励4
				csv.get(object, "reward1_id1");
				elem.task_package_reward_4_map.reserve(object.size());

				for (auto it = object.begin(); it != object.end(); ++it)
				{
					auto collection_name = std::string(it->first.get());
					const auto count = static_cast<std::uint64_t>(it->second.get<double>());
					if (!elem.task_package_reward_4_map.emplace(std::move(collection_name), count).second)
					{
						LOG_EMPERY_CENTER_ERROR("Duplicate task_package_reward_4_map : collection_name = ", collection_name);
						DEBUG_THROW(Exception, sslit("Duplicate task_package_reward_4_map "));
					}
				}

				LOG_EMPERY_CENTER_ERROR("Load  corps_task.csv : success!!!");
			}
			*/
	//		g_legionTaskReward_container = legionTaskReward_container;
	//		handles.push(legionTaskReward_container);
	//		auto servlet = DataSession::create_servlet(CORPS_TASK_FILE, Data::encode_csv_as_json(csv, "task_id"));
	//		handles.push(std::move(servlet));
		}
	}

	namespace Data
	{
		boost::shared_ptr<const LegionTaskReward> LegionTaskReward::get(TaskId task_id)
		{
			PROFILE_ME;

			const auto legionTaskReward_container = g_legionTaskReward_container.lock();
			if (!legionTaskReward_container)
			{
				LOG_EMPERY_CENTER_WARNING("legionTaskReward_container has not been loaded.");
				return{};
			}

			const auto it = legionTaskReward_container->find<0>(task_id);
			if (it == legionTaskReward_container->end<0>())
			{
				LOG_EMPERY_CENTER_TRACE("corps_task not found: task_id = ", task_id);
				return{};
			}
			return boost::shared_ptr<const LegionTaskReward>(legionTaskReward_container, &*it);
		}

		boost::shared_ptr<const LegionTaskReward> LegionTaskReward::require(TaskId task_id)
		{
			PROFILE_ME;
			auto ret = get(task_id);
			if (!ret)
			{
				LOG_EMPERY_CENTER_WARNING("corps_task not found: task_id = ", task_id);
				DEBUG_THROW(Exception, sslit("corps_task not found"));
			}
			return ret;
		}
	}
}