#include "precompiled.hpp"
#include "legion_task_contribution_box.hpp"
#include <poseidon/json.hpp>
#include "mysql/task.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/task.hpp"
#include "task_type_ids.hpp"
#include "account_utilities.hpp"

namespace EmperyCenter {
	namespace {
		fill_task_contribution_info(LegionTaskContributionBox::TaskContributionInfo &info,boost::shared_ptr<MySql::Center_LegionTaskContribution> obj){
			info.account_uuid        = obj->get_account_uuid();
		    info.day_contribution    = obj->get_day_contribution();
		    info.week_contribution   = obj->get_week_contribution();
		    info.total_contribution  = obj->get_total_contribution();
		    info.created_time        = obj->get_created_time();
		}
	}
	
	LegionTaskContributionBox::LegionTaskContributionBox(LegionUuid legion_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_LegionTaskContribution>> &contributions)
		: m_legion_uuid(legion_uuid)
	{
		for (auto it = contributions.begin(); it != contributions.end(); ++it) {
			const auto &obj = *it;
			const auto account_uuid = AccountUuid(obj->get_account_uuid());
			if (!account_uuid) {
				m_stamps = obj;
			}
			else {
				m_contributions.emplace(account_uuid,obj);
			}
		}
	}
	LegionTaskContributionBox::~LegionTaskContributionBox() {
	}

	LegionTaskContributionBox::TaskContributionInfo LegionTaskContributionBox::get(AccountUuid account_uuid) const {
		PROFILE_ME;

		TaskContributionInfo info = {};
		info.account_uuid = account_uuid;

		const auto it = m_contributions.find(account_uuid);
		if (it == m_contributions.end()) {
			return info;
		}
		fill_task_contribution_info(info, it->second);
		return info;
	}
	void LegionTaskContributionBox::get_all(std::vector<LegionTaskContributionBox::TaskContributionInfo> &ret) const {
		PROFILE_ME;

		ret.reserve(ret.size() + m_contributions.size());
		for (auto it = m_contributions.begin(); it != m_contributions.end(); ++it) {
			TaskContributionInfo info;
			fill_task_contribution_info(info, it->second);
			ret.emplace_back(std::move(info));
		}
	}

	void LegionTaskContributionBox::insert(LegionTaskContributionBox::TaskContributionInfo info) {
		PROFILE_ME;

		const auto account_uuid = info.account_uuid;
		auto it = m_contributions.find(account_uuid);
		if (it != m_contributions.end()) {
			LOG_EMPERY_CENTER_WARNING("Task contribution exists: legion_uuid = ", get_legion_uuid(), ", account_uuid = ", account_uuid);
			DEBUG_THROW(Exception, sslit("Task contribution exists"));
		}
		const auto obj = boost::make_shared<MySql::Center_LegionTaskContribution>(get_legion_uuid().get(), account_uuid.get(),info.day_contribution,info.week_contribution,info.total_contribution,info.created_time);
		obj->async_save(true);
		it = m_contributions.emplace(account_uuid,obj).first;
	}
	void LegionTaskContributionBox::update(LegionTaskContributionBox::TaskContributionInfo info, bool throws_if_not_exists) {
		PROFILE_ME;

		const auto account_uuid = info.account_uuid;
		const auto it = m_contributions.find(account_uuid);
		if (it == m_contributions.end()) {
			LOG_EMPERY_CENTER_WARNING("Task not found: account_uuid = ", get_account_uuid(), ", account_uuid = ", account_uuid);
			if (throws_if_not_exists) {
				DEBUG_THROW(Exception, sslit("Task not found"));
			}
			return;
		}
		auto &obj = it->second;
		obj->set_day_contribution(info.day_contribution);
		obj->set_week_contribution(info.week_contribution);
		obj->set_total_contribution(info.total_contribution);
		obj->set_created_time(info.created_time);	
	}

	void LegionTaskContributionBox::reset() noexcept {
		PROFILE_ME;
		for(auto it = m_contributions.begin(); it != m_contributions.end(); ++it){
			const auto &obj = it->second;
			obj->set_day_contribution(0);
			obj->set_week_contribution(0);
			obj->set_total_contribution(0);
			obj->set_created_time(0);	
		}
	}
}