#ifndef EMPERY_CENTER_DATA_TASK_HPP_
#define EMPERY_CENTER_DATA_TASK_HPP_

#include "common.hpp"
#include <boost/container/flat_map.hpp>
#include <vector>

namespace EmperyCenter
{
	namespace Data
	{
		class TaskAbstract
		{
		public:
			enum CastleCategory
			{
				CC_PRIMARY = 1,
				CC_ALL = 2,
				CC_NON_PRIMARY = 3,
			};

		public:
			static boost::shared_ptr<const TaskAbstract> get(TaskId task_id);
			static boost::shared_ptr<const TaskAbstract> require(TaskId task_id);

		public:
			TaskId task_id;
			unsigned castle_category;
			TaskTypeId type;
			bool accumulative;

			boost::container::flat_map<std::uint64_t, std::vector<double>> objective;
			boost::container::flat_map<ItemId, std::uint64_t> rewards;
			boost::container::flat_map<ResourceId, std::uint64_t> rewards_resources;
		};

		class TaskPrimary : public TaskAbstract
		{
		public:
			static boost::shared_ptr<const TaskPrimary> get(TaskId task_id);
			static boost::shared_ptr<const TaskPrimary> require(TaskId task_id);

			static void get_all(std::vector<boost::shared_ptr<const TaskPrimary>>& ret);

		public:
			TaskId previous_id;
		};

		class TaskDaily : public TaskAbstract
		{
		public:
			enum ETaskLegionPackage
			{
				ETaskLegionPackage_Class = 4,
			};
			enum ETaskLegion
			{
				ETaskLegion_Class = 5,
			};
		public:
			static boost::shared_ptr<const TaskDaily> get(TaskId task_id);
			static boost::shared_ptr<const TaskDaily> require(TaskId task_id);

			static void get_all(std::vector<boost::shared_ptr<const TaskDaily>>& ret);
			static void get_all_legion_package_task(std::vector<boost::shared_ptr<const TaskDaily>>& ret);
			static void get_all_legion_task(std::vector<boost::shared_ptr<const TaskDaily>>& ret);
			static unsigned get_legion_package_task_num(unsigned level);

		public:
			std::uint64_t task_class_1;
			unsigned level_limit_min;
			unsigned level_limit_max;
			unsigned task_level;
		};

		class TaskLegion : public TaskAbstract
		{
		public:
			static boost::shared_ptr<const TaskLegion> get(TaskId task_id);
			static boost::shared_ptr<const TaskLegion> require(TaskId task_id);

			static void get_all(std::vector<boost::shared_ptr<const TaskLegion>>& ret);
		public:
			unsigned level_limit_min;
			unsigned level_limit_max;
			boost::container::flat_map<std::uint64_t, std::vector<std::pair<ResourceId, std::uint64_t>>> stage_reward;
			boost::container::flat_map<std::uint64_t, std::vector<std::pair<ResourceId, std::uint64_t>>> stage_legion_reward;
			boost::container::flat_map<std::uint64_t, std::vector<std::pair<ResourceId, std::uint64_t>>> stage_personal_reward;
		};
		class TaskLegionContribution
		{
		public:
			static boost::shared_ptr<const TaskLegionContribution> get(TaskLegionKeyId task_legion_key_id);
			static boost::shared_ptr<const TaskLegionContribution> require(TaskLegionKeyId task_legion_key_id);
		public:
			TaskLegionKeyId  task_legion_key_id;
			std::uint64_t    out_number;
			std::uint64_t    legion_number;
			std::uint64_t    personal_number;
		};
	}
}

#endif