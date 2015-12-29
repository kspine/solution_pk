#include "../precompiled.hpp"
#include "global_status.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../mysql/global_status.hpp"

namespace EmperyGoldScramble {

namespace {
	using StatusMap = boost::container::flat_map<unsigned, boost::shared_ptr<MySql::GoldScramble_GlobalStatus>>;

	boost::shared_ptr<StatusMap> g_status_map;

	MODULE_RAII_PRIORITY(handles, 2000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		const auto status_map = boost::make_shared<StatusMap>();
		status_map->reserve(100);
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loading global status...");
		conn->execute_sql("SELECT * FROM `GoldScramble_GlobalStatus`");
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			status_map->emplace(obj->get_slot(), std::move(obj));
		}
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loaded ", status_map->size(), " global status value(s).");
		g_status_map = status_map;
		handles.push(status_map);
	}
}

std::uint64_t GlobalStatus::get(unsigned slot){
	PROFILE_ME;

	const auto it = g_status_map->find(slot);
	if(it == g_status_map->end()){
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Global status value not found: slot = ", slot);
		return 0;
	}
	return it->second->get_value();
}
std::uint64_t GlobalStatus::exchange(unsigned slot, std::uint64_t new_value){
	PROFILE_ME;

	auto it = g_status_map->find(slot);
	if(it == g_status_map->end()){
		auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>(slot, 0);
		obj->async_save(true);
		it = g_status_map->emplace_hint(it, slot, std::move(obj));
	}
	const auto old_value = it->second->get_value();
	it->second->set_value(new_value);
	return old_value;
}
std::uint64_t GlobalStatus::fetch_add(unsigned slot, std::uint64_t delta_value){
	PROFILE_ME;

	auto it = g_status_map->find(slot);
	if(it == g_status_map->end()){
		auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>(slot, 0);
		obj->async_save(true);
		it = g_status_map->emplace_hint(it, slot, std::move(obj));
	}
	const auto old_value = it->second->get_value();
	it->second->set_value(checked_add(old_value, delta_value));
	return old_value;
}
std::uint64_t GlobalStatus::fetch_sub(unsigned slot, std::uint64_t delta_value){
	PROFILE_ME;

	auto it = g_status_map->find(slot);
	if(it == g_status_map->end()){
		auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>(slot, 0);
		obj->async_save(true);
		it = g_status_map->emplace_hint(it, slot, std::move(obj));
	}
	const auto old_value = it->second->get_value();
	it->second->set_value(checked_sub(old_value, delta_value));
	return old_value;
}

}
