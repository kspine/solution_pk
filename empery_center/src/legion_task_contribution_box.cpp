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
		void fill_task_contribution_info(LegionTaskContributionBox::TaskContributionInfo &info,boost::shared_ptr<MySql::Center_LegionTaskContribution> obj){
			info.account_uuid        = AccountUuid(obj->get_account_uuid());
		    info.day_contribution    = obj->get_day_contribution();
		    info.week_contribution   = obj->get_week_contribution();
		    info.total_contribution  = obj->get_total_contribution();
		    info.last_update_time        = obj->get_last_update_time();
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
	void LegionTaskContributionBox::update(AccountUuid account_uuid,std::uint64_t delta,std::uint64_t personal_contribute) {
		PROFILE_ME;
		const auto utc_now = Poseidon::get_utc_time();
		const auto it = m_contributions.find(account_uuid);
		if (it == m_contributions.end()) {
			const auto obj = boost::make_shared<MySql::Center_LegionTaskContribution>(get_legion_uuid().get(), account_uuid.get(),delta,delta,delta,utc_now);
			obj->async_save(true);
			m_contributions.emplace(account_uuid,obj);
			return;
		}
		auto &obj = it->second;
		obj->set_day_contribution(obj->get_day_contribution() + delta);
		obj->set_week_contribution(obj->get_week_contribution() + delta);
		obj->set_total_contribution(obj->get_total_contribution() + delta);
		obj->set_last_update_time(utc_now);
	}

	void LegionTaskContributionBox::reset(std::uint64_t now) noexcept {
		PROFILE_ME;
		for(auto it = m_contributions.begin(); it != m_contributions.end(); ++it){
			const auto &obj = it->second;
			obj->set_day_contribution(0);
			obj->set_week_contribution(0);
			obj->set_total_contribution(0);
			obj->set_last_update_time(now);
		}
	}

	void LegionTaskContributionBox::reset_day_contribution(std::uint64_t now) noexcept{
		PROFILE_ME;
		for(auto it = m_contributions.begin(); it != m_contributions.end(); ++it){
			const auto &obj = it->second;
			obj->set_day_contribution(0);
			obj->set_last_update_time(now);
		}
	}

	void LegionTaskContributionBox::reset_week_contribution(std::uint64_t now) noexcept{
		PROFILE_ME;
		for(auto it = m_contributions.begin(); it != m_contributions.end(); ++it){
			const auto &obj = it->second;
			obj->set_week_contribution(0);
			obj->set_last_update_time(now);
		}
	}
	void LegionTaskContributionBox::reset_account_contribution(AccountUuid account_uuid) noexcept{
		PROFILE_ME;

		const auto it = m_contributions.find(account_uuid);
		if (it == m_contributions.end()) {
			LOG_EMPERY_CENTER_WARNING("reset account contribute LegionUuid = ",get_legion_uuid(), " can not find account_uuid = ",account_uuid);
			return;
		}
		auto &obj = it->second;
		obj->set_day_contribution(0);
		obj->set_week_contribution(0);
		obj->set_total_contribution(0);
		obj->set_last_update_time(0);
	}
}