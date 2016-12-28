#ifndef EMPERY_CENTER_LEGION_TASK_REWARD_BOX_HPP_
#define EMPERY_CENTER_LEGION_TASK_REWARD_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {
	namespace MySql {
		class Center_LegionTaskReward;
	}

	class PlayerSession;

	class LegionTaskRewardBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
	public:
		using Progress = boost::container::flat_map<std::uint64_t, std::uint64_t>;

		struct TaskRewardInfo {
			TaskTypeId task_type_id;
			std::uint64_t created_time;
			std::uint64_t last_reward_time;
			boost::shared_ptr<Progress> progress;
		};

	private:
		const AccountUuid m_account_uuid;

		boost::shared_ptr<MySql::Center_LegionTaskReward> m_stamps;

		boost::container::flat_map<TaskTypeId,
			std::pair<boost::shared_ptr<MySql::Center_LegionTaskReward>, boost::shared_ptr<Progress>>> m_tasks;

	public:
		LegionTaskRewardBox(AccountUuid account_uuid,
			const std::vector<boost::shared_ptr<MySql::Center_LegionTaskReward>> &tasks);
		~LegionTaskRewardBox();

	public:
		AccountUuid get_account_uuid() const {
			return m_account_uuid;
		}

		TaskRewardInfo get(TaskTypeId task_type_id) const;
		void get_all(std::vector<TaskRewardInfo> &ret) const;

		void insert(TaskRewardInfo info);
		void update(TaskRewardInfo info, bool throws_if_not_exists = true);
		void reset() noexcept;
		void check_legion_task_reward();
		void pump_status();
		void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
	};
}

#endif