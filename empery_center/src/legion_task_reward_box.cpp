#include "precompiled.hpp"
#include "legion_task_reward_box.hpp"
#include <poseidon/json.hpp>
#include "mysql/task.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/task.hpp"
#include "task_type_ids.hpp"
#include "account_utilities.hpp"

namespace EmperyCenter {
	namespace {
		std::string encode_progress(const LegionTaskRewardBox::Progress &progress) {
			PROFILE_ME;

			if (progress.empty()) {
				return{};
			}
			Poseidon::JsonObject root;
			for (auto it = progress.begin(); it != progress.end(); ++it) {
				const auto key = it->first;
				const auto count = it->second;
				char str[256];
				unsigned len = (unsigned)std::sprintf(str, "%lu", (unsigned long)key);
				root[SharedNts(str, len)] = count;
			}
			std::ostringstream oss;
			root.dump(oss);
			return oss.str();
		}
		LegionTaskRewardBox::Progress decode_progress(const std::string &str) {
			PROFILE_ME;

			LegionTaskRewardBox::Progress progress;
			if (str.empty()) {
				return progress;
			}
			std::istringstream iss(str);
			auto root = Poseidon::JsonParser::parse_object(iss);
			progress.reserve(root.size());
			for (auto it = root.begin(); it != root.end(); ++it) {
				const auto key = boost::lexical_cast<std::uint64_t>(it->first);
				const auto count = static_cast<std::uint64_t>(it->second.get<double>());
				progress.emplace(key, count);
			}
			return progress;
		}

		using TaskAwardObjectPair = std::pair<boost::shared_ptr<MySql::Center_LegionTaskReward>, boost::shared_ptr<LegionTaskRewardBox::Progress>>;

		void fill_task_reward_info(LegionTaskRewardBox::TaskRewardInfo &info, const TaskAwardObjectPair &pair) {
			PROFILE_ME;

			const auto &obj = pair.first;
			const auto &progress = pair.second;

			info.task_id = TaskId(obj->get_task_id());
			info.created_time  = obj->get_created_time();
			info.last_reward_time = obj->get_last_reward_time();
			info.progress = progress;
		}
	}

	LegionTaskRewardBox::LegionTaskRewardBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_LegionTaskReward>> &tasks)
		: m_account_uuid(account_uuid)
	{
		for (auto it = tasks.begin(); it != tasks.end(); ++it) {
			const auto &obj = *it;
			const auto task_id = TaskId(obj->get_task_id());
			if (!task_id) {
				m_stamps = obj;
			}
			else {
				m_tasks.emplace(task_id,
					std::make_pair(obj,
						boost::make_shared<Progress>(decode_progress(obj->unlocked_get_progress()))));
			}
		}
	}
	LegionTaskRewardBox::~LegionTaskRewardBox() {
	}

	LegionTaskRewardBox::TaskRewardInfo LegionTaskRewardBox::get(TaskId task_id) const {
		PROFILE_ME;

		TaskRewardInfo info = {};
		info.task_id = task_id;

		const auto it = m_tasks.find(task_id);
		if (it == m_tasks.end()) {
			return info;
		}
		fill_task_reward_info(info, it->second);
		return info;
	}
	void LegionTaskRewardBox::get_all(std::vector<LegionTaskRewardBox::TaskRewardInfo> &ret) const {
		PROFILE_ME;

		ret.reserve(ret.size() + m_tasks.size());
		for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
			TaskRewardInfo info;
			fill_task_reward_info(info, it->second);
			ret.emplace_back(std::move(info));
		}
	}

	void LegionTaskRewardBox::insert(LegionTaskRewardBox::TaskRewardInfo info) {
		PROFILE_ME;

		const auto task_id = info.task_id;
		auto it = m_tasks.find(task_id);
		if (it != m_tasks.end()) {
			LOG_EMPERY_CENTER_WARNING("Task reward exists: account_uuid = ", get_account_uuid(), ", task_id = ", task_id);
			DEBUG_THROW(Exception, sslit("Task reward exists"));
		}
		const auto progress = boost::make_shared<Progress>();
		if (info.progress) {
			*progress = *info.progress;
		}
		const auto obj = boost::make_shared<MySql::Center_LegionTaskReward>(get_account_uuid().get(), task_id.get(),encode_progress(*progress),info.created_time, info.last_reward_time);
		obj->async_save(true);
		it = m_tasks.emplace(task_id, std::make_pair(obj, progress)).first;
	}
	void LegionTaskRewardBox::update(LegionTaskRewardBox::TaskRewardInfo info, bool throws_if_not_exists) {
		PROFILE_ME;

		const auto task_id = info.task_id;
		const auto it = m_tasks.find(task_id);
		if (it == m_tasks.end()) {
			LOG_EMPERY_CENTER_WARNING("Task not found: account_uuid = ", get_account_uuid(), ", task_id = ", task_id);
			if (throws_if_not_exists) {
				DEBUG_THROW(Exception, sslit("Task not found"));
			}
			return;
		}
		const auto &pair = it->second;
		const auto &obj = pair.first;
		std::string progress_str;
		progress_str = encode_progress(*info.progress);
		*(pair.second) = *(info.progress);
		obj->set_progress(std::move(progress_str));
		obj->set_last_reward_time(info.last_reward_time);
	}

	void LegionTaskRewardBox::reset() noexcept {
		PROFILE_ME;

		for(auto it = m_tasks.begin(); it != m_tasks.end(); ++it){
			const auto pair = std::move(it->second);
			const auto &obj = pair.first;
			std::string progress_str;
			const auto progress = boost::make_shared<Progress>();
			progress_str = encode_progress(*progress);
			obj->set_last_reward_time(0);
			obj->set_progress(progress_str);
		}
	}
}