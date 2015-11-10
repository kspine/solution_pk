#include "../precompiled.hpp"
#include "game_record_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include "global_status.hpp"
#include "../mysql/game_history.hpp"

namespace EmperyGoldScramble {

namespace {
	using RecordQueue = std::deque<boost::shared_ptr<MySql::GoldScramble_GameHistory>>;

	std::size_t g_maxCount = 100;
	RecordQueue g_queue;

	MODULE_RAII_PRIORITY(/* handles */, 5000){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		getConfig(g_maxCount, "max_game_record_count");

		RecordQueue queue;
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loading game records...");
		std::ostringstream oss;
		oss <<"SELECT * FROM `GoldScramble_GameHistory` ORDER BY `recordAutoId` ASC LIMIT " <<g_maxCount;
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::GoldScramble_GameHistory>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			queue.emplace_back(std::move(obj));
		}
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loaded ", queue.size(), " record(s).");
		g_queue.swap(queue);
	}
}

void GameRecordMap::append(std::string loginName, std::string nick, boost::uint64_t goldCoins, boost::uint64_t accountBalance){
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::GoldScramble_GameHistory>(
		GlobalStatus::fetchAdd(GlobalStatus::SLOT_RECORD_AUTO_ID, 1),
		Poseidon::getUtcTime(), std::move(loginName), std::move(nick), goldCoins, accountBalance);
	obj->asyncSave(true);
	g_queue.emplace_back(std::move(obj));
	if(g_queue.size() >= g_maxCount){
		g_queue.pop_front();
	}
}
void GameRecordMap::getAll(std::vector<GameRecordMap::Record> &ret){
	PROFILE_ME;

	ret.reserve(ret.size() + g_queue.size());
	for(auto it = g_queue.begin(); it != g_queue.end(); ++it){
		const auto &obj = *it;
		Record record;
		record.recordAutoId   = obj->get_recordAutoId();
		record.timestamp      = obj->get_timestamp();
		record.loginName      = obj->unlockedGet_loginName();
		record.nick           = obj->unlockedGet_nick();
		record.goldCoins      = obj->get_goldCoins();
		record.accountBalance = obj->get_accountBalance();
		ret.emplace_back(std::move(record));
	}
}

}
