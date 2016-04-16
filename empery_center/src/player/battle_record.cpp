#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_battle_record.hpp"
#include "../msg/sc_battle_record.hpp"
#include "../singletons/battle_record_box_map.hpp"
#include "../battle_record_box.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_BattleRecordGetPagedRecords, account, session, req){
	const auto battle_record_box = BattleRecordBoxMap::require(account->get_account_uuid());
	battle_record_box->pump_status();

	std::vector<BattleRecordBox::RecordInfo> records;
	battle_record_box->get_all(records);

	auto rbegin = records.rend(), rend = records.rend();
	if(req.begin < records.size()){
		rbegin = records.rbegin() + static_cast<std::ptrdiff_t>(req.begin);
	}
	if(req.count < static_cast<std::size_t>(rend - rbegin)){
		rend = rbegin + static_cast<std::ptrdiff_t>(req.count);
	}

	Msg::SC_BattleRecordPagedRecords msg;
	msg.total_count = records.size();
	msg.begin = req.begin;
	msg.records.reserve(static_cast<std::size_t>(rend - rbegin));
	for(auto it = rbegin; it != rend; ++it){
		if(it->second_account_uuid){
			AccountMap::cached_synchronize_account_with_player(it->second_account_uuid, session);
		}

		auto &record = *msg.records.emplace(msg.records.end());
		record.timestamp             = it->timestamp;
		record.first_object_type_id  = it->first_object_type_id.get();
		record.first_coord_x         = it->first_coord.x();
		record.first_coord_y         = it->first_coord.y();
		record.second_account_uuid   = it->second_account_uuid.str();
		record.second_object_type_id = it->second_object_type_id.get();
		record.second_coord_x        = it->second_coord.x();
		record.second_coord_y        = it->second_coord.y();
		record.result_type           = it->result_type;
		record.soldiers_wounded      = it->soldiers_wounded;
		record.result_param2         = it->result_param2;
		record.soldiers_damaged      = it->soldiers_damaged;
		record.soldiers_remaining    = it->soldiers_remaining;
	}
	session->send(msg);

	return Response();
}

}
