#include "precompiled.hpp"
#include "task_box.hpp"
#include <poseidon/json.hpp>
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
	std::string encode_progress(const boost::container::flat_map<std::uint64_t, std::uint64_t> &progress){
		PROFILE_ME;

		if(progress.empty()){
			return { };
		}
		Poseidon::JsonObject root;
		for(auto it = progress.begin(); it != progress.end(); ++it){
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
	boost::container::flat_map<std::uint64_t, std::uint64_t> decode_progress(const std::string &str){
		PROFILE_ME;

		boost::container::flat_map<std::uint64_t, std::uint64_t> progress;
		if(str.empty()){
			return progress;
		}
		std::istringstream iss(str);
		auto root = Poseidon::JsonParser::parse_object(iss);
		progress.reserve(root.size());
		for(auto it = root.begin(); it != root.end(); ++it){
			const auto key = boost::lexical_cast<std::uint64_t>(it->first);
			const auto count = static_cast<std::uint64_t>(it->second.get<double>());
			progress.emplace(key, count);
		}
		return progress;
	}

	using TaskObjectPair = std::pair<boost::shared_ptr<MySql::Center_Task>, TaskBox::Progress>;

	void fill_task_info(TaskBox::TaskInfo &info, const boost::shared_ptr<TaskObjectPair> &pair){
		PROFILE_ME;

		const auto &obj = pair->first;
		const auto &progress = pair->second;

		info.task_id      = TaskId(obj->get_task_id());
		info.created_time = obj->get_created_time();
		info.expiry_time  = obj->get_expiry_time();
		info.progress     = boost::shared_ptr<const TaskBox::Progress>(pair, &progress);
		info.rewarded     = obj->get_rewarded();
	}

	void fill_task_message(Msg::SC_TaskChanged &msg, const boost::shared_ptr<TaskObjectPair> &pair){
		PROFILE_ME;

		const auto &obj = pair->first;
		const auto &progress = pair->second;

		msg.task_id       = obj->get_task_id();
		msg.created_time  = obj->get_created_time();
		msg.expiry_time   = obj->get_expiry_time();
		msg.progress.reserve(progress.size());
		for(auto it = progress.begin(); it != progress.end(); ++it){
			auto &elem = *msg.progress.emplace(msg.progress.end());
			elem.key = it->first;
			elem.count = it->second;
		}
		msg.rewarded      = obj->get_rewarded();
	}
}

TaskBox::TaskBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_Task>> &tasks)
	: m_account_uuid(account_uuid)
{
	for(auto it = tasks.begin(); it != tasks.end(); ++it){
		const auto &obj = *it;
		const auto task_id = TaskId(obj->get_task_id());
		if(!task_id){
			m_timestamp = obj;
		} else {
			m_tasks.emplace(task_id,
				boost::make_shared<std::pair<boost::shared_ptr<MySql::Center_Task>, Progress>>(
					obj, decode_progress(obj->unlocked_get_progress())));
		}
	}
}
TaskBox::~TaskBox(){
}

void TaskBox::pump_status(){
	PROFILE_ME;
/*
	const auto utc_now = Poseidon::get_utc_time();

	LOG_EMPERY_CENTER_TRACE("Checking for auto increment tasks: account_uuid = ", get_account_uuid());
*/
	// TODO
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

void TaskBox::insert(TaskBox::TaskInfo info){
	PROFILE_ME;

	const auto task_id = info.task_id;
	const auto it = m_tasks.find(task_id);
	if(it != m_tasks.end()){
		LOG_EMPERY_CENTER_WARNING("Task exists: account_uuid = ", get_account_uuid(), ", task_id = ", task_id);
		DEBUG_THROW(Exception, sslit("Task exists"));
	}

	const auto obj = boost::make_shared<MySql::Center_Task>(get_account_uuid().get(), task_id.get(),
		info.created_time, info.expiry_time, encode_progress(*info.progress), info.rewarded);
	obj->async_save(true);
	const auto pair = boost::make_shared<TaskObjectPair>(obj, *info.progress);
	m_tasks.emplace(task_id, pair);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_TaskChanged msg;
			fill_task_message(msg, pair);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}
void TaskBox::update(TaskBox::TaskInfo info, bool throws_if_not_exists){
	PROFILE_ME;

	const auto task_id = info.task_id;
	const auto it = m_tasks.find(task_id);
	if(it == m_tasks.end()){
	    LOG_EMPERY_CENTER_WARNING("Task not found: account_uuid = ", get_account_uuid(), ", task_uuid = ", task_id);
		if(throws_if_not_exists){
			DEBUG_THROW(Exception, sslit("Task not found"));
		}
		return;
	}
	const auto &pair = it->second;

	std::string progress_str;
	if(pair.owner_before(info.progress) || info.progress.owner_before(pair)){
		progress_str = encode_progress(*info.progress);
	}

	pair->first->set_created_time(info.created_time);
	pair->first->set_expiry_time(info.expiry_time);
	pair->first->set_progress(std::move(progress_str));
	pair->first->set_rewarded(info.rewarded);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
	    try {
        	Msg::SC_TaskChanged msg;
        	fill_task_message(msg, pair);
        	session->send(msg);
    	} catch(std::exception &e){
        	LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
        	session->shutdown(e.what());
    	}
	}
}
bool TaskBox::remove(TaskId task_id) noexcept {
	PROFILE_ME;

	const auto it = m_tasks.find(task_id);
	if(it == m_tasks.end()){
		return false;
	}
	const auto pair = std::move(it->second);
	m_tasks.erase(it);

	pair->first->set_expiry_time(0);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_TaskChanged msg;
			fill_task_message(msg, pair);
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}

	return true;
}

void TaskBox::check(TaskTypeId type, std::uint64_t key, std::uint64_t count, std::uint64_t param1, std::uint64_t param2, std::uint64_t param3){
	PROFILE_ME;

	// TODO
}

void TaskBox::synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const {
	PROFILE_ME;

	for(auto it = m_tasks.begin(); it != m_tasks.end(); ++it){
		Msg::SC_TaskChanged msg;
		fill_task_message(msg, it->second);
		session->send(msg);
	}
}

}
