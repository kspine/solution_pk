#ifndef EMPERY_CENTER_TASK_BOX_HPP_
#define EMPERY_CENTER_TASK_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/container/flat_map.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_Task;
}

class PlayerSession;

class TaskBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum Category {
		CAT_PRIMARY     = 1,
		CAT_DAILY       = 2,
	};

	using Progress = boost::container::flat_map<std::uint64_t, std::uint64_t>;

	struct TaskInfo {
		TaskId task_id;
		Category category;
		std::uint64_t created_time;
		std::uint64_t expiry_time;
		boost::shared_ptr<const Progress> progress;
		bool rewarded;
	};

private:
	const AccountUuid m_account_uuid;

	boost::shared_ptr<MySql::Center_Task> m_stamps;

	boost::container::flat_map<TaskId,
		boost::shared_ptr<std::pair<boost::shared_ptr<MySql::Center_Task>, Progress>>
		> m_tasks;

public:
	TaskBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_Task>> &tasks);
	~TaskBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void check_init_tasks();

	TaskInfo get(TaskId task_id) const;
	void get_all(std::vector<TaskInfo> &ret) const;

	void insert(TaskInfo info);
	void update(TaskInfo info, bool throws_if_not_exists = true);
	bool remove(TaskId task_id) noexcept;

	bool has_been_accomplished(TaskId task_id) const;
	void check(TaskTypeId type, std::uint64_t key, std::uint64_t count,
		bool is_primary_castle, std::uint64_t param1, std::uint64_t param2, std::uint64_t param3);

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_task_box_with_player(const boost::shared_ptr<const TaskBox> &task_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	task_box->synchronize_with_player(session);
}
inline void synchronize_task_box_with_player(const boost::shared_ptr<TaskBox> &task_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	task_box->synchronize_with_player(session);
}

}

#endif

