#include "../precompiled.hpp"
#include "bid_record_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include "global_status.hpp"
#include "../mysql/bid_history.hpp"

namespace EmperyGoldScramble {

namespace {
	using RecordQueue = std::deque<boost::shared_ptr<MySql::GoldScramble_BidHistory>>;

	RecordQueue g_queue;

	MODULE_RAII_PRIORITY(/* handles */, 3000){
		const auto conn = Poseidon::MySqlDaemon::createConnection();

		RecordQueue queue;
		const auto gameAutoId = GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID);
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loading bid records: gameAutoId = ", gameAutoId);
		std::ostringstream oss;
		oss <<"SELECT * FROM `GoldScramble_BidHistory` WHERE `gameAutoId` = " <<gameAutoId <<" ORDER BY `recordAutoId` ASC";
		while(conn->fetchRow()){
			auto obj = boost::make_shared<MySql::GoldScramble_BidHistory>();
			obj->syncFetch(conn);
			obj->enableAutoSaving();
			queue.emplace_back(std::move(obj));
		}
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loaded ", queue.size(), " record(s).");
		g_queue.swap(queue);
	}
}

void BidRecordMap::append(std::string loginName, std::string nick, boost::uint64_t goldCoins, boost::uint64_t accountBalance){
	PROFILE_ME;

	g_queue.emplace_back(boost::make_shared<MySql::GoldScramble_BidHistory>(
		GlobalStatus::fetchAdd(GlobalStatus::SLOT_RECORD_AUTO_ID, 1), GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID),
		Poseidon::getUtcTime(), std::move(loginName), std::move(nick), goldCoins, accountBalance));
}
std::size_t BidRecordMap::getSize(){
	PROFILE_ME;

	return g_queue.size();
}
void BidRecordMap::getAll(std::vector<BidRecordMap::Record> &ret, std::size_t limit){
	PROFILE_ME;

	const auto gameAutoId = GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID);

	const auto count = std::min(g_queue.size(), limit);
	ret.reserve(ret.size() + count);
	for(auto it = g_queue.end() - static_cast<std::ptrdiff_t>(count); it != g_queue.end(); ++it){
		const auto &obj = *it;
		if(obj->get_gameAutoId() != gameAutoId){
			continue;
		}
		Record record;
		record.recordAutoId   = obj->get_recordAutoId();
		record.gameAutoId     = obj->get_gameAutoId();
		record.timestamp      = obj->get_timestamp();
		record.loginName      = obj->unlockedGet_loginName();
		record.nick           = obj->unlockedGet_nick();
		record.goldCoins      = obj->get_goldCoins();
		record.accountBalance = obj->get_accountBalance();
		ret.emplace_back(std::move(record));
	}
}
void BidRecordMap::clear() noexcept {
	PROFILE_ME;

	g_queue.clear();
}

}
