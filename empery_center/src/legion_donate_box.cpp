#include "precompiled.hpp"
#include "legion_donate_box.hpp"
#include <poseidon/json.hpp>
#include "mysql/legion.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "account_utilities.hpp"

namespace EmperyCenter {
	namespace {
		void fill_donate_info(LegionDonateBox::DonateInfo &info,boost::shared_ptr<MySql::Center_LegionDonate> obj){
			info.account_uuid        = AccountUuid(obj->get_account_uuid());
		    info.day_donate          = obj->get_day_donate();
		    info.week_donate         = obj->get_week_donate();
		    info.total_donate        = obj->get_total_donate();
		    info.last_update_time    = obj->get_last_update_time();
		}
	}
	LegionDonateBox::LegionDonateBox(LegionUuid legion_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_LegionDonate>> & donations)
		: m_legion_uuid(legion_uuid)
	{
		for (auto it = donations.begin(); it != donations.end(); ++it) {
			const auto &obj = *it;
			const auto account_uuid = AccountUuid(obj->get_account_uuid());
			if (!account_uuid) {
				m_stamps = obj;
			}
			else {
				m_donations.emplace(account_uuid,obj);
			}
		}
	}
	LegionDonateBox::~LegionDonateBox() {
	}

	LegionDonateBox::DonateInfo LegionDonateBox::get(AccountUuid account_uuid) const {
		PROFILE_ME;

		DonateInfo info = {};
		info.account_uuid = account_uuid;

		const auto it = m_donations.find(account_uuid);
		if (it == m_donations.end()) {
			return info;
		}
		fill_donate_info(info, it->second);
		return info;
	}

	void LegionDonateBox::get_all(std::vector<LegionDonateBox::DonateInfo> &ret) const {
		PROFILE_ME;

		ret.reserve(ret.size() + m_donations.size());
		for (auto it = m_donations.begin(); it != m_donations.end(); ++it) {
			DonateInfo info;
			fill_donate_info(info, it->second);
			ret.emplace_back(std::move(info));
		}
	}
	void LegionDonateBox::update(AccountUuid account_uuid,std::uint64_t delta) {
		PROFILE_ME;
		const auto utc_now = Poseidon::get_utc_time();
		const auto it = m_donations.find(account_uuid);
		if (it == m_donations.end()) {
			const auto obj = boost::make_shared<MySql::Center_LegionDonate>(get_legion_uuid().get(), account_uuid.get(),delta,delta,delta,utc_now);
			obj->async_save(true);
			m_donations.emplace(account_uuid,obj);
			return;
		}
		auto &obj = it->second;
		obj->set_day_donate(obj->get_day_donate() + delta);
		obj->set_week_donate(obj->get_week_donate() + delta);
		obj->set_total_donate(obj->get_total_donate() + delta);
		obj->set_last_update_time(utc_now);
	}

	void LegionDonateBox::reset(std::uint64_t now) noexcept {
		PROFILE_ME;
		for(auto it = m_donations.begin(); it != m_donations.end(); ++it){
			const auto &obj = it->second;
			obj->set_day_donate(0);
			obj->set_week_donate(0);
			obj->set_total_donate(0);
			obj->set_last_update_time(now);
		}
	}

	void LegionDonateBox::reset_day_donate(std::uint64_t now) noexcept{
		PROFILE_ME;
		for(auto it = m_donations.begin(); it != m_donations.end(); ++it){
			const auto &obj = it->second;
			obj->set_day_donate(0);
			obj->set_last_update_time(now);
		}
	}

	void LegionDonateBox::reset_week_donate(std::uint64_t now) noexcept{
		PROFILE_ME;
		for(auto it = m_donations.begin(); it != m_donations.end(); ++it){
			const auto &obj = it->second;
			obj->set_week_donate(0);
			obj->set_last_update_time(now);
		}
	}
	void LegionDonateBox::reset_account_donate(AccountUuid account_uuid) noexcept{
		PROFILE_ME;

		const auto it = m_donations.find(account_uuid);
		if (it == m_donations.end()) {
			LOG_EMPERY_CENTER_WARNING("reset account donate LegionUuid = ",get_legion_uuid(), " can not find account_uuid = ",account_uuid);
			return;
		}
		auto &obj = it->second;
		obj->set_day_donate(0);
		obj->set_week_donate(0);
		obj->set_total_donate(0);
		obj->set_last_update_time(0);
	}
}