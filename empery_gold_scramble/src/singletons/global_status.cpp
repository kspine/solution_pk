#include "../precompiled.hpp"
#include "global_status.hpp"
#include <boost/container/flat_map.hpp>
#include <poseidon/singletons/mysql_daemon.hpp>
#include "../mysql/global_status.hpp"
#include "../checked_arithmetic.hpp"

namespace EmperyGoldScramble {

namespace {
	using StatusMap = boost::container::flat_map<unsigned, boost::shared_ptr<MySql::GoldScramble_GlobalStatus>>;

	boost::shared_ptr<StatusMap> g_statusMap;

	MODULE_RAII_PRIORITY(handles, 2000){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		const auto statusMap = boost::make_shared<StatusMap>();
		statusMap->reserve(100);
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loading global status...");
		conn->executeSql("SELECT * FROM `GoldScramble_GlobalStatus`");
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			statusMap->emplace(obj->get_slot(), std::move(obj));
		}
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loaded ", statusMap->size(), " global status value(s).");
		g_statusMap = statusMap;
		handles.push(statusMap);
	}
}

boost::uint64_t GlobalStatus::get(unsigned slot){
	PROFILE_ME;

	const auto it = g_statusMap->find(slot);
	if(it == g_statusMap->end()){
		LOG_EMPERY_GOLD_SCRAMBLE_DEBUG("Global status value not found: slot = ", slot);
		return 0;
	}
	return it->second->get_value();
}
boost::uint64_t GlobalStatus::exchange(unsigned slot, boost::uint64_t newValue){
	PROFILE_ME;

	auto it = g_statusMap->find(slot);
	if(it == g_statusMap->end()){
		auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>(slot, 0);
		obj->asyncSave(true);
		it = g_statusMap->emplace_hint(it, slot, std::move(obj));
	}
	const auto oldValue = it->second->get_value();
	it->second->set_value(newValue);
	return oldValue;
}
boost::uint64_t GlobalStatus::fetchAdd(unsigned slot, boost::uint64_t deltaValue){
	PROFILE_ME;

	auto it = g_statusMap->find(slot);
	if(it == g_statusMap->end()){
		auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>(slot, 0);
		obj->asyncSave(true);
		it = g_statusMap->emplace_hint(it, slot, std::move(obj));
	}
	const auto oldValue = it->second->get_value();
	it->second->set_value(checkedAdd(oldValue, deltaValue));
	return oldValue;
}
boost::uint64_t GlobalStatus::fetchSub(unsigned slot, boost::uint64_t deltaValue){
	PROFILE_ME;

	auto it = g_statusMap->find(slot);
	if(it == g_statusMap->end()){
		auto obj = boost::make_shared<MySql::GoldScramble_GlobalStatus>(slot, 0);
		obj->asyncSave(true);
		it = g_statusMap->emplace_hint(it, slot, std::move(obj));
	}
	const auto oldValue = it->second->get_value();
	it->second->set_value(checkedSub(oldValue, deltaValue));
	return oldValue;
}

}
