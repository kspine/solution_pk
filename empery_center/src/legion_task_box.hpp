#ifndef EMPERY_CENTER_LEGION_TASK_BOX_HPP_
#define EMPERY_CENTER_LEGION_TASK_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {
	namespace MySql {
		class Center_LegionTask;
	}

	class Castle;
	class PlayerSession;

	class LegionTaskBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
	public:
		enum Category {
			CAT_NULL = 0,
			CAT_LEGION = 5,
		};

		enum CastleCategory {
			TCC_PRIMARY = 1,
			TCC_ALL = 2,
			TCC_NON_PRIMARY = 3,
		};

		using Progress = boost::container::flat_map<std::uint64_t, std::uint64_t>;

		struct TaskInfo {
			TaskId task_id;
			Category category;
			std::uint64_t created_time;
			std::uint64_t expiry_time;
			boost::shared_ptr<const Progress> progress;
			boost::shared_ptr<const Progress> rewarded_progress;
			bool rewarded;
		};

	private:
		const LegionUuid m_legion_uuid;

		boost::shared_ptr<MySql::Center_LegionTask> m_stamps;
		boost::container::flat_map<TaskId,
			std::pair<boost::shared_ptr<MySql::Center_LegionTask>, std::pair<boost::shared_ptr<Progress>,boost::shared_ptr<Progress>>>> m_tasks;

	public:
		LegionTaskBox(LegionUuid legion_uuid,
			const std::vector<boost::shared_ptr<MySql::Center_LegionTask>> &tasks);
		~LegionTaskBox();

	public:
		virtual void pump_status();

		LegionUuid get_legion_uuid() const {
			return m_legion_uuid;
		}

		void check_legion_tasks();

		TaskInfo get(TaskId task_id) const;
		void get_all(std::vector<TaskInfo> &ret) const;

		void insert(TaskInfo info);
		void update(TaskInfo info, bool throws_if_not_exists = true);
		bool remove(TaskId task_id) noexcept;

		void check_stage_accomplished(TaskId task_id);
		void check(TaskTypeId type, TaskLegionKeyId key, std::uint64_t count,
			const AccountUuid account_uuid, std::int64_t param1, std::int64_t param2);
		void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
		void synchronize_with_members() const;
		std::uint64_t get_legion_task_day(std::uint64_t utc_time);
		std::uint64_t get_legion_task_date(std::uint64_t day);
	};
}

#endif