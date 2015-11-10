#include "../precompiled.hpp"
#include "bid_record_map.hpp"
#include <poseidon/singletons/mysql_daemon.hpp>
#include <poseidon/multi_index_map.hpp>
#include "global_status.hpp"
#include "../mysql/bid_history.hpp"

namespace EmperyGoldScramble {

namespace {
	struct RecordElement {
		boost::shared_ptr<MySql::GoldScramble_BidHistory> obj;

		boost::uint64_t recordAutoId;
		std::string loginName;

		explicit RecordElement(boost::shared_ptr<MySql::GoldScramble_BidHistory> obj_)
			: obj(std::move(obj_))
			, recordAutoId(obj->get_recordAutoId()), loginName(obj->unlockedGet_loginName())
		{
		}
	};

	MULTI_INDEX_MAP(RecordQueue, RecordElement,
		UNIQUE_MEMBER_INDEX(recordAutoId)
		UNIQUE_MEMBER_INDEX(loginName)
	)

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
			const auto it = queue.find<1>(obj->unlockedGet_loginName());
			if(it == queue.end<1>()){
				queue.insert(RecordElement(std::move(obj)));
			} else {
				queue.replace<1>(it, RecordElement(std::move(obj)));
			}
		}
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loaded ", queue.size(), " record(s).");
		g_queue.swap(queue);
	}
}

void BidRecordMap::append(std::string loginName, std::string nick, boost::uint64_t goldCoins, boost::uint64_t accountBalance){
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::GoldScramble_BidHistory>(
		GlobalStatus::fetchAdd(GlobalStatus::SLOT_RECORD_AUTO_ID, 1), GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID),
		Poseidon::getUtcTime(), std::move(loginName), std::move(nick), goldCoins, accountBalance);
	obj->asyncSave(true);
	const auto it = g_queue.find<1>(obj->unlockedGet_loginName());
	if(it == g_queue.end<1>()){
		g_queue.insert(RecordElement(std::move(obj)));
	} else {
		g_queue.replace<1>(it, RecordElement(std::move(obj)));
	}
}
std::size_t BidRecordMap::getSize(){
	PROFILE_ME;

	return g_queue.size();
}
void BidRecordMap::getAll(std::vector<BidRecordMap::Record> &ret, std::size_t limit){
	PROFILE_ME;

	const auto gameAutoId = GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID);

	auto begin = g_queue.begin<0>();
	if(g_queue.size() > limit){
		std::advance(begin, static_cast<std::ptrdiff_t>(g_queue.size() - limit));
	}
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, g_queue.end<0>())));
	for(auto it = begin; it != g_queue.end<0>(); ++it){
		const auto &obj = it->obj;
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
