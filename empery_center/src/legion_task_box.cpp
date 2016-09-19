#include "precompiled.hpp"
#include "legion_task_box.hpp"
#include <poseidon/json.hpp>
#include "msg/sc_task.hpp"
#include "mysql/task.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/task.hpp"
#include "data/item.hpp"
#include "data/global.hpp"
#include "item_ids.hpp"
#include "map_object_type_ids.hpp"
#include "task_type_ids.hpp"
#include "singletons/world_map.hpp"
#include "castle.hpp"
#include "account_utilities.hpp"
#include "legion_attribute_ids.hpp"
#include "dungeon/common.hpp"
#include "singletons/legion_member_map.hpp"
#include "singletons/legion_map.hpp"
#include "legion_member.hpp"
#include "legion_task_reward_box.hpp"
#include "singletons/legion_task_reward_box_map.hpp"
#include <poseidon/async_job.hpp>
#include "mail_box.hpp"
#include "singletons/mail_box_map.hpp"
#include "mail_data.hpp"
#include "chat_message_type_ids.hpp"
#include "chat_message_slot_ids.hpp"
#include "legion_task_contribution_box.hpp"
#include "singletons/legion_task_contribution_box_map.hpp"
#include "singletons/account_map.hpp"
#include "account_attribute_ids.hpp"
#include "account.hpp"
#include <poseidon/string.hpp>
#include "legion_member_attribute_ids.hpp"
#include "msg/sc_legion.hpp"

namespace EmperyCenter {
	namespace {
		uint64_t CaculateWeekDay(unsigned y, unsigned m, unsigned d)
		{
			if(m==1||m==2) //把一月和二月换算成上一年的十三月和是四月
			{
				m+=12;
				y--;
			}
			// 返回1就是周一 返回7就是周日
			return (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7 + 1;
		}

		std::string encode_progress(const LegionTaskBox::Progress &progress) {
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
		LegionTaskBox::Progress decode_progress(const std::string &str) {
			PROFILE_ME;

			LegionTaskBox::Progress progress;
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

		using TaskObjectPair = std::pair<boost::shared_ptr<MySql::Center_LegionTask>, boost::shared_ptr<LegionTaskBox::Progress>>;

		void fill_task_info(LegionTaskBox::TaskInfo &info, const TaskObjectPair &pair) {
			PROFILE_ME;

			const auto &obj = pair.first;
			const auto &progress = pair.second;

			info.task_id = TaskId(obj->get_task_id());
			info.category = LegionTaskBox::Category(obj->get_category());
			info.created_time = obj->get_created_time();
			info.expiry_time = obj->get_expiry_time();
			info.progress = progress;
			info.rewarded = obj->get_rewarded();
		}

		void fill_task_message(Msg::SC_TaskChanged &msg, const TaskObjectPair &pair, std::uint64_t utc_now) {
			PROFILE_ME;

			const auto &obj = pair.first;
			const auto &progress = pair.second;

			msg.task_id = obj->get_task_id();
			msg.category = obj->get_category();
			msg.created_time = obj->get_created_time();
			msg.expiry_duration = saturated_sub(obj->get_expiry_time(), utc_now);
			msg.progress.reserve(progress->size());
			for (auto it = progress->begin(); it != progress->end(); ++it) {
				auto &elem = *msg.progress.emplace(msg.progress.end());
				elem.key = it->first;
				elem.count = it->second;
			}
			msg.rewarded = obj->get_rewarded();
		}
	}

	LegionTaskBox::LegionTaskBox(LegionUuid legion_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_LegionTask>> &tasks)
		: m_legion_uuid(legion_uuid)
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
	LegionTaskBox::~LegionTaskBox() {
	}

	void LegionTaskBox::pump_status() {
		PROFILE_ME;
		LOG_EMPERY_CENTER_TRACE("Checking tasks: legion_uuid = ", get_legion_uuid());

		const auto utc_now = Poseidon::get_utc_time();	
		LOG_EMPERY_CENTER_FATAL("LegionTaskBox::pump_status,tasks.size() = ",m_tasks.size());
		auto it = m_tasks.begin();
		while (it != m_tasks.end()) {
			const auto task_id = it->first;
			const auto &obj = it->second.first;
			if (utc_now > obj->get_expiry_time()) {
				++it;
				continue;
			}
			if(obj->get_rewarded()){
				++it;
				continue;
			}
			check_stage_accomplished(task_id);
			++it;
			LOG_EMPERY_CENTER_DEBUG("> Removing expired task: legion_uuid = ", get_legion_uuid(), ", task_id = ", task_id);
		}

		check_legion_tasks();
	}
	void LegionTaskBox::check_legion_tasks(){
		PROFILE_ME;
		const auto legion = LegionMap::require(get_legion_uuid());
		const auto legion_level = boost::lexical_cast<std::uint64_t>(legion->get_attribute(LegionAttributeIds::ID_LEVEL));
		const auto legion_uuid = get_legion_uuid();
		const auto utc_now = Poseidon::get_utc_time();

		if (!m_stamps) {
			auto obj = boost::make_shared<MySql::Center_LegionTask>(get_legion_uuid().get(), 0, 0, 0, 0, std::string(), false);
			obj->async_save(true);
			m_stamps = std::move(obj);
		}
		// 特殊：
		// created_time 是上次任务刷新时间。

		const auto last_refreshed_time = m_stamps->get_created_time();
		const auto last_refreshed_day = get_legion_task_day(last_refreshed_time);
		const auto today = get_legion_task_day(utc_now);
		if (last_refreshed_day < today) {
			const auto legion_task_refresh_time = get_legion_task_date(today + 1);
			std::vector<TaskId> task_candidates;
			std::vector<boost::shared_ptr<const Data::TaskLegion>> task_data_legion;
			Data::TaskLegion::get_all(task_data_legion);
			for (auto it = task_data_legion.begin(); it != task_data_legion.end(); ++it) {
				const auto &task_data = *it;
				if ((legion_level < task_data->level_limit_min) || (task_data->level_limit_max < legion_level)) {
					continue;
				}
				const auto task_id = task_data->task_id;
				task_candidates.emplace_back(task_id);
			}
			//删除已有的已经不符合的任务
			for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
				const auto task_id = it->first;
				const auto cit = std::find(task_candidates.begin(), task_candidates.end(), task_id);
				if (cit == task_candidates.end()) {
					m_tasks.erase(task_id);
				}
			}
			for (auto it = task_candidates.begin(); it != task_candidates.end(); ++it) {
				const auto task_id = *it;
				auto info = get(task_id);
				const bool nonexistent = (info.created_time == 0);
				info.task_id = task_id;
				info.category = CAT_LEGION;
				info.created_time = utc_now;
				info.expiry_time = legion_task_refresh_time;
				const auto progress = boost::make_shared<Progress>();
				info.progress = progress;
				info.rewarded = 0;
				if (nonexistent) {
					LOG_EMPERY_CENTER_DEBUG("New legion task: legion_uuid = ", legion_uuid, ", task_id = ", task_id);
					insert(std::move(info));
				}
				else {
					LOG_EMPERY_CENTER_DEBUG("Unrewarded legion task: account_uuid = ", legion_uuid, ", task_id = ", task_id);
					update(std::move(info));
				}
			}
			m_stamps->set_created_time(utc_now);
			//TODO 更新军团成员领奖记录和帐号获得个人贡献记录
			try{
				std::vector<boost::shared_ptr<LegionMember>> members;
				LegionMemberMap::get_by_legion_uuid(members, get_legion_uuid());
				Poseidon::enqueue_async_job([=]() mutable {
					for(auto it = members.begin(); it != members.end(); ++it)
					{
						auto account_uuid = (*it)->get_account_uuid();
						auto legion_task_reward_box = LegionTaskRewardBoxMap::require(account_uuid);
						LOG_EMPERY_CENTER_FATAL("reset task reward account_uuid = ",account_uuid, " members size:",members.size());
						if(last_refreshed_time != 0){//第1次创建时不重置。（退出军团加入新的军团，领奖记录保留）
							legion_task_reward_box->reset();
							const auto account = AccountMap::get(account_uuid);
							if(!account){
								continue;
							}
							boost::container::flat_map<AccountAttributeId, std::string> modifiers;
							modifiers[AccountAttributeIds::ID_LEGION_PERSONAL_CONTRIBUTION] = "0";
							account->set_attributes(std::move(modifiers));
						}
					}
				});
			}catch (std::exception &e) {
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
			//更新军团贡献每日记录
			//周二 更新军团贡献每周记录
			try{
				Poseidon::enqueue_async_job([=]() mutable {
					auto legion_task_contribution_box = LegionTaskContributionBoxMap::require(get_legion_uuid());
					legion_task_contribution_box->reset_day_contribution(utc_now);
					const auto dt = Poseidon::break_down_time(utc_now);
					if(CaculateWeekDay(dt.yr, dt.mon,dt.day) == Data::Global::as_unsigned(Data::Global::SLOT_LEGION_TASK_WEEK_REFRESH)){
						legion_task_contribution_box->reset_week_contribution(utc_now);
					}
				});
			}catch (std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	}

	LegionTaskBox::TaskInfo LegionTaskBox::get(TaskId task_id) const {
		PROFILE_ME;

		TaskInfo info = {};
		info.task_id = task_id;

		const auto it = m_tasks.find(task_id);
		if (it == m_tasks.end()) {
			return info;
		}
		fill_task_info(info, it->second);
		return info;
	}
	void LegionTaskBox::get_all(std::vector<LegionTaskBox::TaskInfo> &ret) const {
		PROFILE_ME;

		ret.reserve(ret.size() + m_tasks.size());
		for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
			TaskInfo info;
			fill_task_info(info, it->second);
			ret.emplace_back(std::move(info));
		}
	}

	void LegionTaskBox::insert(LegionTaskBox::TaskInfo info) {
		PROFILE_ME;

		const auto task_id = info.task_id;
		auto it = m_tasks.find(task_id);
		if (it != m_tasks.end()) {
			LOG_EMPERY_CENTER_WARNING("Task exists: legion_uuid = ", get_legion_uuid(), ", task_id = ", task_id);
			DEBUG_THROW(Exception, sslit("Task exists"));
		}

		const auto task_data = Data::TaskLegion::require(task_id);
		const auto utc_now = Poseidon::get_utc_time();

		const auto progress = boost::make_shared<Progress>();
		if (info.progress) {
			*progress = *info.progress;
		}
		const auto obj = boost::make_shared<MySql::Center_LegionTask>(get_legion_uuid().get(), task_id.get(),
			info.category, info.created_time, info.expiry_time, encode_progress(*progress), info.rewarded);
		obj->async_save(true);
		it = m_tasks.emplace(task_id, std::make_pair(obj, progress)).first;

		const auto legion = LegionMap::get(get_legion_uuid());
		if (legion) {
			try {
				Msg::SC_TaskChanged msg;
				fill_task_message(msg, it->second, utc_now);
				legion->broadcast_to_members(msg);
			}
			catch (std::exception &e) {
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	}
	void LegionTaskBox::update(LegionTaskBox::TaskInfo info, bool throws_if_not_exists) {
		PROFILE_ME;

		const auto task_id = info.task_id;
		const auto it = m_tasks.find(task_id);
		if (it == m_tasks.end()) {
			LOG_EMPERY_CENTER_WARNING("Task not found: legion_uuid = ", get_legion_uuid(), ", task_id = ", task_id);
			if (throws_if_not_exists) {
				DEBUG_THROW(Exception, sslit("Task not found"));
			}
			return;
		}
		const auto &pair = it->second;
		const auto &obj = pair.first;

		const auto task_data = Data::TaskLegion::require(task_id);
		const auto utc_now = Poseidon::get_utc_time();

		std::string progress_str;
		bool reset_progress = false;
		if (pair.second != info.progress) {
			progress_str = encode_progress(*info.progress);
			reset_progress = true;
		}

		obj->set_category(info.category);
		obj->set_created_time(info.created_time);
		obj->set_expiry_time(info.expiry_time);
		obj->set_rewarded(info.rewarded);
		if (reset_progress) {
			obj->set_progress(std::move(progress_str));
			*(pair.second) = *(info.progress);
		}

		const auto legion = LegionMap::get(get_legion_uuid());
		if (legion) {
			try {
				Msg::SC_TaskChanged msg;
				fill_task_message(msg, pair, utc_now);
				legion->broadcast_to_members(msg);
			}
			catch (std::exception &e) {
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
	}
	bool LegionTaskBox::remove(TaskId task_id) noexcept {
		PROFILE_ME;

		const auto it = m_tasks.find(task_id);
		if (it == m_tasks.end()) {
			return false;
		}
		const auto pair = std::move(it->second);
		m_tasks.erase(it);
		const auto &obj = pair.first;

		const auto utc_now = Poseidon::get_utc_time();

		obj->set_expiry_time(0);

		const auto legion = LegionMap::get(get_legion_uuid());
		if (legion) {
			try {
				Msg::SC_TaskChanged msg;
				fill_task_message(msg, pair, utc_now);
				legion->broadcast_to_members(msg);
			}
			catch (std::exception &e) {
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
		}
		return true;
	}

	void LegionTaskBox::check_stage_accomplished(TaskId task_id){
		PROFILE_ME;
		Poseidon::enqueue_async_job([=]() mutable {
			const auto it = m_tasks.find(task_id);
			if (it == m_tasks.end()) {
				return;
			}
			const auto &pair = it->second;
			const auto &obj = pair.first;
			const auto &progress = pair.second;
			if(obj->get_rewarded()){
				return;
			}
			const auto utc_now = Poseidon::get_utc_time();
			const auto task_data = Data::TaskLegion::require(task_id);
			unsigned stage_size = task_data->stage_reward.size();
			unsigned stage_accomplished_size = 0;
			std::vector<boost::shared_ptr<LegionMember>> members;
			LegionMemberMap::get_by_legion_uuid(members, get_legion_uuid());
			auto legion = LegionMap::require(get_legion_uuid());
			for(auto its = task_data->stage_reward.begin(); its != task_data->stage_reward.end(); ++its){
				unsigned objective_size = task_data->objective.size();
				unsigned accomplished_size = 0;
				for (auto it = task_data->objective.begin(); it != task_data->objective.end(); ++it) {
					const auto key = it->first;
					const auto count_finish = static_cast<std::uint64_t>(it->second.at(0));
					const auto dest = count_finish * its->first /100;
					const auto pit = progress->find(key);
					if (pit == progress->end()) {
						LOG_EMPERY_CENTER_DEBUG("Progress element not found: key = ", key);
						break;
					}
					if (pit->second < dest) {
						LOG_EMPERY_CENTER_DEBUG("Progress element unmet: key = ", key, ", count = ", pit->second, ", dest_finish = ", dest);
						break;
					}
					accomplished_size += 1;
				}
				if(accomplished_size == objective_size){
					stage_accomplished_size +=1;
					//发送军团奖励
					auto it_legion_reward = task_data->stage_legion_reward.find(its->first);
					const auto &legion_rewards  = it_legion_reward->second;
					std::uint64_t legion_donate = 0;
					for(auto itl = legion_rewards.begin(); itl != legion_rewards.end(); ++itl){
						if(itl->first.get() == 5500001){
							legion_donate += itl->second;
						}
					}
					if(legion && (legion_donate > 0)){
						boost::container::flat_map<LegionAttributeId, std::string> Attributes;
						std::string donate = legion->get_attribute(LegionAttributeIds::ID_MONEY);
						if(donate.empty()){
							Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(legion_donate);
						}else{
							Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(donate) + legion_donate);
						}
						legion->set_attributes(Attributes);
					}


					//TODO 发送军团成员阶段奖励
					for(auto it = members.begin(); it != members.end(); ++it)
					{
						auto account_uuid = (*it)->get_account_uuid();
						auto legion_task_reward_box = LegionTaskRewardBoxMap::get(account_uuid);
						LegionTaskRewardBox::TaskRewardInfo task_reward_info = legion_task_reward_box->get(task_data->type);
						if(task_reward_info.created_time == 0){
							task_reward_info.created_time = utc_now;
							task_reward_info.last_reward_time = utc_now;
							auto progress = boost::make_shared<LegionTaskRewardBox::Progress>();
							progress->emplace(its->first,1);
							task_reward_info.progress = progress;
							legion_task_reward_box->insert(std::move(task_reward_info));
						}else{
							auto &progress = task_reward_info.progress;
							auto pit = progress->find(its->first);
							if((pit != progress->end()) && (pit->second > 0)){
								continue;
							}
							task_reward_info.last_reward_time = utc_now;
							auto new_progress = boost::make_shared<Progress>(*progress);
							(*new_progress)[its->first] = 1;
							task_reward_info.progress = new_progress;
							legion_task_reward_box->update(std::move(task_reward_info));
						}
						//TODO 发送邮件
						try {
							const auto mail_box = MailBoxMap::require(account_uuid);
							const auto mail_uuid = MailUuid(Poseidon::Uuid::random());
							const auto language_id = LanguageId(); //
							std::vector<std::pair<ChatMessageSlotId, std::string>> segments;
							segments.reserve(5);
							auto legion_nick = legion->get_nick();
							segments.emplace_back(ChatMessageSlotIds::ID_LEGEION_NAME, boost::lexical_cast<std::string>(legion_nick));
							segments.emplace_back(ChatMessageSlotIds::ID_LEGEION_TASK_ID, boost::lexical_cast<std::string>(task_id.get()));
							segments.emplace_back(ChatMessageSlotIds::ID_LEGION_STAGE, boost::lexical_cast<std::string>(its->first));
							const auto &rewards = its->second;
							boost::container::flat_map<ItemId, std::uint64_t> attachments;
							attachments.reserve(rewards.size());
							for(auto itr = rewards.begin(); itr != rewards.end(); ++itr){
								attachments.emplace(ItemId(itr->first.get()),itr->second);
							}
							const auto mail_data = boost::make_shared<MailData>(mail_uuid, language_id, utc_now,
								ChatMessageTypeIds::ID_LEGION_TASK_REWARD, AccountUuid(), std::string(), std::move(segments),std::move(attachments));
							MailBoxMap::insert_mail_data(mail_data);
							MailBox::MailInfo mail_info = { };
							mail_info.mail_uuid   = mail_uuid;
							mail_info.expiry_time = UINT64_MAX;
							mail_info.system      = true;
							mail_box->insert(std::move(mail_info));
						} catch(std::exception &e){
							LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
						}
					}
				}
			}
			if(stage_size == stage_accomplished_size){
				//TODO 设置完成状态
				obj->set_rewarded(true);
			}
		});
	}
	void LegionTaskBox::check(TaskTypeId type, TaskLegionKeyId key, std::uint64_t count,
			const AccountUuid account_uuid, std::int64_t param1, std::int64_t param2)
	{
		PROFILE_ME;

		(void)param1;
		(void)param2;
		try {
			Poseidon::enqueue_async_job([=]() mutable {
				const auto utc_now = Poseidon::get_utc_time();
				const auto legion = LegionMap::require(get_legion_uuid());
				const auto task_contribution_data = Data::TaskLegionContribution::require(key);
				auto legion_task_contribution_box = LegionTaskContributionBoxMap::require(get_legion_uuid());
				for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
					const auto task_id = it->first;
					auto &pair = it->second;
					const auto &obj = pair.first;

					if (obj->get_rewarded()) {
						continue;
					}
					const auto task_data = Data::TaskLegion::require(task_id);
					if (task_data->type != type) {
						continue;
					}
					LOG_EMPERY_CENTER_DEBUG("Checking task: legion_uuid = ", get_legion_uuid(), ", task_id = ", task_id);
					const auto oit = task_data->objective.find(key.get());
					if (oit == task_data->objective.end()) {
						continue;
					}
					const auto old_progress = pair.second;
					std::uint64_t count_old, count_new;
					const auto cit = old_progress->find(key.get());
					if (cit != old_progress->end()) {
						count_old = cit->second;
					}
					else {
						count_old = 0;
					}
					if (task_data->accumulative) {
						count_new = saturated_add(count_old, count);
					}
					else {
						count_new = std::max(count_old, count);
					}
					const auto count_finish = static_cast<std::uint64_t>(oit->second.at(0));
					if (count_new > count_finish) {
						count_new = count_finish;
					}
					if (count_new == count_old) {
						continue;
					}
					const auto delta = count_new - count_old;
					auto new_progress = boost::make_shared<Progress>(*old_progress);
					(*new_progress)[key.get()] = count_new;
					auto new_progress_str = encode_progress(*new_progress);
					pair.second = std::move(new_progress);
					obj->set_progress(std::move(new_progress_str));
					try {
						Msg::SC_TaskChanged msg;
						fill_task_message(msg, pair, utc_now);
						LOG_EMPERY_CENTER_DEBUG("legion broadcast to members: ",msg);
						legion->broadcast_to_members(msg);
					}
					catch (std::exception &e) {
						LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
					}
					//TODO 累计个人贡献
					(void)delta;
					std::uint64_t person_contribution = delta * task_contribution_data->personal_number / task_contribution_data->out_number;
					std::uint64_t legion_contribution = delta * task_contribution_data->legion_number / task_contribution_data->out_number;
					//
					bool should_award_personal_contribution = false;
					const auto account = AccountMap::get(account_uuid);
					if(!account){
						continue;
					}
					const std::string day_personal_contribution_str = account->get_attribute(AccountAttributeIds::ID_LEGION_PERSONAL_CONTRIBUTION);
					std::uint64_t day_personal_contribution = 0;
					if(!day_personal_contribution_str.empty()){
						day_personal_contribution = boost::lexical_cast<std::uint64_t>(day_personal_contribution_str);
					}
					std::uint64_t person_contribution_finish = static_cast<std::uint64_t>(Data::Global::as_double(Data::Global::SLOT_LEGION_TASK_PERSONAL_CONTRIBUTE_THRESHOLD));
					if(day_personal_contribution < person_contribution_finish){
						should_award_personal_contribution = true;
					}
					std::uint64_t total_contribution = day_personal_contribution + person_contribution;
					if(total_contribution > person_contribution_finish){
						person_contribution = person_contribution_finish - day_personal_contribution;
					}
					legion_task_contribution_box->update(account_uuid,delta,person_contribution);
					const auto legion_member = LegionMemberMap::get_by_account_uuid(account_uuid);
					if(legion_member && (person_contribution > 0) && should_award_personal_contribution){
						boost::container::flat_map<LegionMemberAttributeId, std::string> legion_attributes_modifer;
						std::string donate = legion_member->get_attribute(LegionMemberAttributeIds::ID_DONATE);
						if(donate.empty()){
							legion_attributes_modifer[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(person_contribution);
						}else{
							legion_attributes_modifer[LegionMemberAttributeIds::ID_DONATE] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(donate) + person_contribution);
						}
						legion_member->set_attributes(std::move(legion_attributes_modifer));
						boost::container::flat_map<AccountAttributeId, std::string> modifiers;
						modifiers[AccountAttributeIds::ID_LEGION_PERSONAL_CONTRIBUTION] = boost::lexical_cast<std::string>(day_personal_contribution + person_contribution);
						account->set_attributes(std::move(modifiers));
					}

					if(legion && (legion_contribution > 0)){
						boost::container::flat_map<LegionAttributeId, std::string> Attributes;
						std::string donate = legion->get_attribute(LegionAttributeIds::ID_MONEY);

						if(donate.empty()){
							Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(legion_contribution);
						}else{
							std::uint64_t old_donate = boost::lexical_cast<uint64_t>(donate);
							LOG_EMPERY_CENTER_FATAL("legion money:",old_donate);
							Attributes[LegionAttributeIds::ID_MONEY] = boost::lexical_cast<std::string>(boost::lexical_cast<uint64_t>(donate) + legion_contribution);
						}
						legion->set_attributes(Attributes);
						// 广播通知
						Msg::SC_LegionNoticeMsg msg;
						msg.msgtype = Legion::LEGION_NOTICE_MSG_TYPE::LEGION_NOTICE_MSG_TYPE_TASK_CHANGE;
						msg.nick = "";
						msg.ext1 = "";
						legion->sendNoticeMsg(msg);
						donate = legion->get_attribute(LegionAttributeIds::ID_MONEY);
						std::uint64_t new_donate = boost::lexical_cast<uint64_t>(donate);
						LOG_EMPERY_CENTER_FATAL("legion money:",new_donate);
					}
				}
			});
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_ERROR("std::exception thrown: what = ", e.what());
		}
	}

	void LegionTaskBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const{
		const auto utc_now = Poseidon::get_utc_time();
		for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
			Msg::SC_TaskChanged msg;
			fill_task_message(msg, it->second, utc_now);
			LOG_EMPERY_CENTER_DEBUG("LegionTaskBox task synchronize :",msg);
			session->send(msg);
		}
	}

	void LegionTaskBox::synchronize_with_members() const{
		PROFILE_ME;
		const auto utc_now = Poseidon::get_utc_time();
		const auto legion = LegionMap::get(get_legion_uuid());
		if(!legion){
			return;
		}
		for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
			Msg::SC_TaskChanged msg;
			fill_task_message(msg, it->second, utc_now);
			legion->broadcast_to_members(msg);
		}
	}
	std::uint64_t LegionTaskBox::get_legion_task_day(std::uint64_t utc_time){
		const auto legion_refresh_minutes = Data::Global::as_unsigned(Data::Global::SLOT_LEGION_TASK_DAY_REFRESH);
		const auto auto_inc_offset = checked_mul<std::uint64_t>(legion_refresh_minutes, 60000);
		return saturated_sub(utc_time, auto_inc_offset) / 86400000;
	}
	std::uint64_t LegionTaskBox::get_legion_task_date(std::uint64_t day){
		const auto legion_refresh_minutes = Data::Global::as_unsigned(Data::Global::SLOT_LEGION_TASK_DAY_REFRESH);
		const auto auto_inc_offset = checked_mul<std::uint64_t>(legion_refresh_minutes, 60000);
		return day * 86400000 + auto_inc_offset;
	}
}