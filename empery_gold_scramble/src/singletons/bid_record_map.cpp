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

		boost::uint64_t record_auto_id;
		std::string login_name;

		explicit RecordElement(boost::shared_ptr<MySql::GoldScramble_BidHistory> obj_)
			: obj(std::move(obj_))
			, record_auto_id(obj->get_record_auto_id()), login_name(obj->unlocked_get_login_name())
		{
		}
	};

	MULTI_INDEX_MAP(RecordQueue, RecordElement,
		UNIQUE_MEMBER_INDEX(record_auto_id)
		UNIQUE_MEMBER_INDEX(login_name)
	)

	RecordQueue g_queue;

	MODULE_RAII_PRIORITY(/* handles */, 3000){
		const auto conn = Poseidon::MySqlDaemon::create_connection();

		RecordQueue queue;
		const auto game_auto_id = GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID);
		LOG_EMPERY_GOLD_SCRAMBLE_INFO("Loading bid records: game_auto_id = ", game_auto_id);
		std::ostringstream oss;
		oss <<"SELECT * FROM `GoldScramble_BidHistory` WHERE `game_auto_id` = " <<game_auto_id <<" ORDER BY `record_auto_id` ASC";
		conn->execute_sql(oss.str());
		while(conn->fetch_row()){
			auto obj = boost::make_shared<MySql::GoldScramble_BidHistory>();
			obj->fetch(conn);
			obj->enable_auto_saving();
			const auto it = queue.find<1>(obj->unlocked_get_login_name());
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

void BidRecordMap::append(std::string login_name, std::string nick, boost::uint64_t gold_coins, boost::uint64_t account_balance){
	PROFILE_ME;

	auto obj = boost::make_shared<MySql::GoldScramble_BidHistory>(
		GlobalStatus::fetch_add(GlobalStatus::SLOT_RECORD_AUTO_ID, 1), GlobalStatus::get(GlobalStatus::SLOT_GAME_AUTO_ID),
		Poseidon::get_utc_time(), std::move(login_name), std::move(nick), gold_coins, account_balance);
	obj->async_save(true);
	const auto it = g_queue.find<1>(obj->unlocked_get_login_name());
	if(it == g_queue.end<1>()){
		g_queue.insert(RecordElement(std::move(obj)));
	} else {
		g_queue.replace<1>(it, RecordElement(std::move(obj)));
	}
}
std::size_t BidRecordMap::get_size(){
	PROFILE_ME;

	return g_queue.size();
}
void BidRecordMap::get_all(std::vector<BidRecordMap::Record> &ret, std::size_t limit){
	PROFILE_ME;

	auto begin = g_queue.begin<0>();
	if(g_queue.size() > limit){
		std::advance(begin, static_cast<std::ptrdiff_t>(g_queue.size() - limit));
	}
	ret.reserve(ret.size() + static_cast<std::size_t>(std::distance(begin, g_queue.end<0>())));
	for(auto it = begin; it != g_queue.end<0>(); ++it){
		BidRecordMap::Record record;
		record.record_auto_id   = it->obj->get_record_auto_id();
		record.game_auto_id     = it->obj->get_game_auto_id();
		record.timestamp      = it->obj->get_timestamp();
		record.login_name      = it->obj->unlocked_get_login_name();
		record.nick           = it->obj->unlocked_get_nick();
		record.gold_coins      = it->obj->get_gold_coins();
		record.account_balance = it->obj->get_account_balance();
		ret.emplace_back(std::move(record));
	}
}
void BidRecordMap::clear() noexcept {
	PROFILE_ME;

	g_queue.clear();
}

}
