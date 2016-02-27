#include "../precompiled.hpp"
#include "global_status.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../mysql/global_status.hpp"

namespace EmperyCenter {

namespace {
	using GlobalStatusContainer = boost::container::flat_map<GlobalStatus::Slot, boost::shared_ptr<MySql::Center_GlobalStatus>>;

	boost::shared_ptr<GlobalStatusContainer> g_global_status_map;

	MODULE_RAII_PRIORITY(handles, 1000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto global_status_map = boost::make_shared<GlobalStatusContainer>();
		LOG_EMPERY_CENTER_INFO("Loading global status...");
		conn->execute_sql("SELECT * FROM `Center_GlobalStatus`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::Center_GlobalStatus>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			global_status_map->emplace((GlobalStatus::Slot)obj->get_slot(), std::move(obj));
		}
		LOG_EMPERY_CENTER_INFO("Done loading global status.");
		g_global_status_map = global_status_map;
		handles.push(global_status_map);
	}
}

const std::string &GlobalStatus::get(GlobalStatus::Slot slot){
	PROFILE_ME;

	const auto it = g_global_status_map->find(slot);
	if(it == g_global_status_map->end()){
		LOG_EMPERY_CENTER_DEBUG("Global status not found: slot = ", (unsigned)slot);
		return Poseidon::EMPTY_STRING;
	}
	return it->second->unlocked_get_value();
}
void GlobalStatus::reserve(GlobalStatus::Slot slot){
	PROFILE_ME;

	const auto it = g_global_status_map->find(slot);
	if(it == g_global_status_map->end()){
		auto obj = boost::make_shared<MySql::Center_GlobalStatus>((unsigned)slot, std::string());
		obj->async_save(true);
		g_global_status_map->emplace(slot, std::move(obj));
	}
}
void GlobalStatus::set(GlobalStatus::Slot slot, std::string value){
	PROFILE_ME;

	const auto it = g_global_status_map->find(slot);
	if(it == g_global_status_map->end()){
		auto obj = boost::make_shared<MySql::Center_GlobalStatus>((unsigned)slot, std::move(value));
		obj->async_save(true);
		g_global_status_map->emplace(slot, std::move(obj));
	} else {
		it->second->set_value(std::move(value));
	}
}

}
