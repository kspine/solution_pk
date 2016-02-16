#include "precompiled.hpp"
#include "task_box.hpp"
#include "flag_guard.hpp"
#include "transaction_element.hpp"
#include "msg/sc_task.hpp"
#include "mysql/task.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"
#include "data/task.hpp"
#include "data/global.hpp"
#include "reason_ids.hpp"
#include "singletons/account_map.hpp"
#include "account.hpp"
#include "account_utilities.hpp"
#include "account_attribute_ids.hpp"

namespace EmperyCenter {

namespace {
/*	void fill_task_info(TaskBox::TaskInfo &info, const boost::shared_ptr<MySql::Center_Task> &obj){
		PROFILE_ME;

		info.task_id = TaskId(obj->get_task_id());
		info.count   = obj->get_count();
	}

	void fill_task_message(Msg::SC_TaskChanged &msg, const boost::shared_ptr<MySql::Center_Task> &obj){
		PROFILE_ME;

		msg.task_id = obj->get_task_id();
		msg.count   = obj->get_count();
	}*/
}

TaskBox::TaskBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Task>> &tasks)
	: m_account_uuid(account_uuid)
{
	for(auto it = tasks.begin(); it != tasks.end(); ++it){
//		const auto &obj = *it;
//		m_tasks.emplace(TaskId(obj->get_task_id()), obj);
	}
}
TaskBox::~TaskBox(){
}
/*
void TaskBox::pump_status(){
	PROFILE_ME;

	const auto utc_now = Poseidon::get_utc_time();

	LOG_EMPERY_CENTER_TRACE("Checking for auto increment tasks: account_uuid = ", get_account_uuid());
	std::vector<TaskTransactionElement> transaction;
	std::vector<boost::shared_ptr<const Data::Task>> tasks_to_check;
	Data::Task::get_auto_inc(tasks_to_check);
	boost::container::flat_map<boost::shared_ptr<MySql::Center_Task>, std::uint64_t> new_timestamps;
	for(auto dit = tasks_to_check.begin(); dit != tasks_to_check.end(); ++dit){
		const auto &task_data = *dit;
		auto it = m_tasks.find(task_data->task_id);
		if(it == m_tasks.end()){
			auto obj = boost::make_shared<MySql::Center_Task>(get_account_uuid().get(), task_data->task_id.get(), 0, 0);
			obj->async_save(true);
			it = m_tasks.emplace(task_data->task_id, std::move(obj)).first;
		}
		const auto &obj = it->second;

		std::uint64_t auto_inc_period, auto_inc_offset;
		switch(task_data->auto_inc_type){
		case Data::Task::AIT_HOURLY:
			auto_inc_period = 3600 * 1000;
			auto_inc_offset = checked_mul(task_data->auto_inc_offset, (std::uint64_t)60000);
			break;
		case Data::Task::AIT_DAILY:
			auto_inc_period = 24 * 3600 * 1000;
			auto_inc_offset = checked_mul(task_data->auto_inc_offset, (std::uint64_t)60000);
			break;
		case Data::Task::AIT_WEEKLY:
			auto_inc_period = 7 * 24 * 3600 * 1000;
			auto_inc_offset = checked_mul(task_data->auto_inc_offset, (std::uint64_t)60000) + 3 * 24 * 3600 * 1000; // 注意 1970-01-01 是星期四。
			break;
		case Data::Task::AIT_PERIODIC:
			auto_inc_period = checked_mul(task_data->auto_inc_offset, (std::uint64_t)60000);
			auto_inc_offset = utc_now + 1; // 当前时间永远是区间中的最后一秒。
			break;
		default:
			auto_inc_period = 0;
			auto_inc_offset = 0;
			break;
		}
		if(auto_inc_period == 0){
			LOG_EMPERY_CENTER_WARNING("Task auto increment period is zero? task_id = ", task_data->task_id);
			continue;
		}
		auto_inc_offset %= auto_inc_period;

		const auto old_count = obj->get_count();
		const auto old_updated_time = obj->get_updated_time();

		const auto prev_interval = checked_sub(checked_add(old_updated_time, auto_inc_period), auto_inc_offset) / auto_inc_period;
		const auto cur_interval = checked_sub(utc_now, auto_inc_offset) / auto_inc_period;
		LOG_EMPERY_CENTER_TRACE("> Checking task: task_id = ", task_data->task_id,
			", prev_interval = ", prev_interval, ", cur_interval = ", cur_interval);
		if(cur_interval <= prev_interval){
			continue;
		}
		const auto interval_count = cur_interval - prev_interval;

		if(task_data->auto_inc_step >= 0){
			if(old_count < task_data->auto_inc_bound){
				const auto count_to_add = saturated_mul(static_cast<std::uint64_t>(task_data->auto_inc_step), interval_count);
				const auto new_count = std::min(saturated_add(old_count, count_to_add), task_data->auto_inc_bound);
				LOG_EMPERY_CENTER_TRACE("> Adding tasks: task_id = ", task_data->task_id, ", old_count = ", old_count, ", new_count = ", new_count);
				transaction.emplace_back(TaskTransactionElement::OP_ADD, task_data->task_id, new_count - old_count,
					ReasonIds::ID_AUTO_INCREMENT, task_data->auto_inc_type, task_data->auto_inc_offset, 0);
			}
		} else {
			if(old_count > task_data->auto_inc_bound){
				LOG_EMPERY_CENTER_TRACE("> Removing tasks: task_id = ", task_data->task_id, ", init_count = ", task_data->init_count);
				const auto count_to_remove = saturated_mul(static_cast<std::uint64_t>(-(task_data->auto_inc_step)), interval_count);
				const auto new_count = std::max(saturated_sub(old_count, count_to_remove), task_data->auto_inc_bound);
				LOG_EMPERY_CENTER_TRACE("> Removing tasks: task_id = ", task_data->task_id, ", old_count = ", old_count, ", new_count = ", new_count);
				transaction.emplace_back(TaskTransactionElement::OP_REMOVE, task_data->task_id, old_count - new_count,
					ReasonIds::ID_AUTO_INCREMENT, task_data->auto_inc_type, task_data->auto_inc_offset, 0);
			}
		}
		const auto new_updated_time = saturated_add(old_updated_time, saturated_mul(auto_inc_period, interval_count));
		new_timestamps.emplace(obj, new_updated_time);
	}
	commit_transaction(transaction, false,
		[&]{
			for(auto it = new_timestamps.begin(); it != new_timestamps.end(); ++it){
				it->first->set_updated_time(it->second);
			}
		});
}

void TaskBox::check_init_tasks(){
	PROFILE_ME;

	LOG_EMPERY_CENTER_TRACE("Checking for init tasks: account_uuid = ", get_account_uuid());
	std::vector<TaskTransactionElement> transaction;
	std::vector<boost::shared_ptr<const Data::Task>> tasks_to_check;
	Data::Task::get_init(tasks_to_check);
	for(auto dit = tasks_to_check.begin(); dit != tasks_to_check.end(); ++dit){
		const auto &task_data = *dit;
		const auto it = m_tasks.find(task_data->task_id);
		if(it == m_tasks.end()){
			LOG_EMPERY_CENTER_TRACE("> Adding tasks: task_id = ", task_data->task_id, ", init_count = ", task_data->init_count);
			transaction.emplace_back(TaskTransactionElement::OP_ADD, task_data->task_id, task_data->init_count,
				ReasonIds::ID_INIT_ITEMS, task_data->init_count, 0, 0);
		}
	}
	commit_transaction(transaction, false);
}

TaskBox::TaskInfo TaskBox::get(TaskId task_id) const {
	PROFILE_ME;

	TaskInfo info = { };
	info.task_id = task_id;

	const auto it = m_tasks.find(task_id);
	if(it == m_tasks.end()){
		return info;
	}
	fill_task_info(info, it->second);
	return info;
}
void TaskBox::get_all(std::vector<TaskBox::TaskInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_tasks.size());
	for(auto it = m_tasks.begin(); it != m_tasks.end(); ++it){
		TaskInfo info;
		fill_task_info(info, it->second);
		ret.emplace_back(std::move(info));
	}
}

TaskId TaskBox::commit_transaction_nothrow(const std::vector<TaskTransactionElement> &transaction, bool tax,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	std::vector<boost::shared_ptr<Poseidon::EventBaseWithoutId>> events;
	events.reserve(transaction.size());
	boost::container::flat_map<boost::shared_ptr<MySql::Center_Task>, std::uint64_t  new_count > temp_result_map;
	temp_result_map.reserve(transaction.size());
	std::uint64_t taxing_amount = 0;

	const FlagGuard transaction_guard(m_locked_by_transaction);

	const auto account_uuid = get_account_uuid();
	const auto account = AccountMap::require(account_uuid);

	bool shall_enable_gold_payment = false;

	for(auto tit = transaction.begin(); tit != transaction.end(); ++tit){
		const auto operation   = tit->m_operation;
		const auto task_id     = tit->m_some_id;
		const auto delta_count = tit->m_delta_count;

		if(delta_count == 0){
			continue;
		}

		const auto reason = tit->m_reason;
		const auto param1 = tit->m_param1;
		const auto param2 = tit->m_param2;
		const auto param3 = tit->m_param3;

		switch(operation){
		case TaskTransactionElement::OP_NONE:
			break;

		case TaskTransactionElement::OP_ADD:
			{
				auto it = m_tasks.find(task_id);
				if(it == m_tasks.end()){
					auto obj = boost::make_shared<MySql::Center_Task>(account_uuid.get(), task_id.get(), 0, 0);
					obj->enable_auto_saving(); // obj->async_save(true);
					it = m_tasks.emplace_hint(it, task_id, std::move(obj));
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				temp_it->second = checked_add(old_count, delta_count);
				const auto new_count = temp_it->second;

				if((task_id == TaskIds::ID_GOLD) && (new_count > 0)){
					shall_enable_gold_payment = true;
				}

				LOG_EMPERY_CENTER_DEBUG("& Task transaction: add: account_uuid = ", account_uuid,
					", task_id = ", task_id, ", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::TaskChanged>(
					account_uuid, task_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		case TaskTransactionElement::OP_REMOVE:
		case TaskTransactionElement::OP_REMOVE_SATURATED:
			{
				const auto it = m_tasks.find(task_id);
				if(it == m_tasks.end()){
					if(operation != TaskTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("Task not found: task_id = ", task_id);
						return task_id;
					}
					break;
				}
				const auto &obj = it->second;
				auto temp_it = temp_result_map.find(obj);
				if(temp_it == temp_result_map.end()){
					temp_it = temp_result_map.emplace_hint(temp_it, obj, obj->get_count());
				}
				const auto old_count = temp_it->second;
				if(temp_it->second >= delta_count){
					temp_it->second -= delta_count;
				} else {
					if(operation != TaskTransactionElement::OP_REMOVE_SATURATED){
						LOG_EMPERY_CENTER_DEBUG("No enough tasks: task_id = ", task_id,
							", temp_count = ", temp_it->second, ", delta_count = ", delta_count);
						return task_id;
					}
					temp_it->second = 0;
				}
				const auto new_count = temp_it->second;

				if((task_id == TaskIds::ID_DIAMONDS) || (task_id == TaskIds::ID_GOLD)){
					taxing_amount = checked_add(taxing_amount, saturated_sub(old_count, new_count));
				}

				LOG_EMPERY_CENTER_DEBUG("& Task transaction: remove: account_uuid = ", account_uuid,
					", task_id = ", task_id, ", old_count = ", old_count, ", delta_count = ", delta_count, ", new_count = ", new_count,
					", reason = ", reason, ", param1 = ", param1, ", param2 = ", param2, ", param3 = ", param3);
				events.emplace_back(boost::make_shared<Events::TaskChanged>(
					account_uuid, task_id, old_count, new_count, reason, param1, param2, param3));
			}
			break;

		default:
			LOG_EMPERY_CENTER_ERROR("Unknown task transaction operation: operation = ", (unsigned)operation);
			DEBUG_THROW(Exception, sslit("Unknown task transaction operation"));
		}
	}

	const auto withdrawn = boost::make_shared<bool>(true);
	for(auto it = events.begin(); it != events.end(); ++it){
		Poseidon::async_raise_event(*it, withdrawn);
	}
	if(callback){
		callback();
	}
	for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
		it->first->set_count(it->second);
	}
	*withdrawn = false;

	if(tax && (taxing_amount > 0)){
		LOG_EMPERY_CENTER_DEBUG("Calculating tax: account_uuid = ", account_uuid, ", taxing_amount = ", taxing_amount);

		try {
			accumulate_promotion_bonus(account_uuid, taxing_amount);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
		}
	}

	if(shall_enable_gold_payment){
		const auto gold_payment_enabled = account->cast_attribute<bool>(AccountAttributeIds::ID_GOLD_PAYMENT_ENABLED);
		if(!gold_payment_enabled){
			try {
				boost::container::flat_map<AccountAttributeId, std::string> modifiers;
				modifiers[AccountAttributeIds::ID_GOLD_PAYMENT_ENABLED] = "1";
				account->set_attributes(std::move(modifiers));
			} catch(std::exception &e){
				LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			}
			LOG_EMPERY_CENTER_INFO("Gold payment enabled: account_uuid = ", get_account_uuid());
		}
	}

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			for(auto it = temp_result_map.begin(); it != temp_result_map.end(); ++it){
				Msg::SC_TaskChanged msg;
				fill_task_message(msg, it->first);
				session->send(msg);
			}
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return { };
}
void TaskBox::commit_transaction(const std::vector<TaskTransactionElement> &transaction, bool tax,
	const boost::function<void ()> &callback)
{
	PROFILE_ME;

	const auto insuff_id = commit_transaction_nothrow(transaction, tax, callback);
	if(insuff_id != TaskId()){
		LOG_EMPERY_CENTER_DEBUG("Insufficient tasks in task box: account_uuid = ", get_account_uuid(), ", insuff_id = ", insuff_id);
		DEBUG_THROW(Exception, sslit("Insufficient tasks in task box"));
	}
}

void TaskBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	for(auto it = m_tasks.begin(); it != m_tasks.end(); ++it){
		Msg::SC_TaskChanged msg;
		fill_task_message(msg, it->second);
		session->send(msg);
	}
}
*/
}
