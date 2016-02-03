#include "../precompiled.hpp"
#include "common.hpp"
#include "../mmain.hpp"
#include "../msg/cs_tax.hpp"
#include "../msg/sc_tax.hpp"
#include "../singletons/tax_box_map.hpp"
#include "../tax_box.hpp"

namespace EmperyCenter {

PLAYER_SERVLET(Msg::CS_TaxGetRecordsPaged, account, session, req){
	const auto tax_box = TaxBoxMap::require(account->get_account_uuid());
	tax_box->pump_status();

	std::vector<TaxBox::RecordInfo> records;
	tax_box->get_all(records);

	auto rbegin = records.rend(), rend = records.rend();
	if(req.begin < records.size()){
		rbegin = records.rbegin() + static_cast<std::ptrdiff_t>(req.begin);
	}
	if(req.count < static_cast<std::size_t>(rend - rbegin)){
		rend = rbegin + static_cast<std::ptrdiff_t>(req.count);
	}

	Msg::SC_TaxRecordsPaged msg;
	msg.total_count = records.size();
	msg.begin = req.begin;
	msg.records.reserve(static_cast<std::size_t>(rbegin - rend));
	for(auto it = rbegin; it != rend; ++it){
		auto &record = *msg.records.emplace(msg.records.end());
		record.auto_inc          = it->auto_inc;
		record.timestamp         = it->timestamp;
		record.from_account_uuid = it->from_account_uuid.str();
		record.reason            = it->reason.get();
		record.old_amount        = it->old_amount;
		record.new_amount        = it->new_amount;
	}
	session->send(msg);

	return Response();
}

}
