#include "precompiled.hpp"
#include "tax_box.hpp"
#include "mysql/tax_record.hpp"
#include "data/global.hpp"
#include "msg/sc_tax.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

namespace {
	void fill_record_info(TaxBox::RecordInfo &info, const boost::shared_ptr<MySql::Center_TaxRecord> &obj){
		PROFILE_ME;

		info.auto_uuid         = obj->unlocked_get_auto_uuid();
		info.timestamp         = obj->get_timestamp();
		info.from_account_uuid = AccountUuid(obj->get_from_account_uuid());
		info.reason            = ReasonId(obj->get_reason());
		info.old_amount        = obj->get_old_amount();
		info.new_amount        = obj->get_new_amount();
	}
}

TaxBox::TaxBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_TaxRecord>> &records)
	: m_account_uuid(account_uuid)
{
	for(auto it = records.begin(); it != records.end(); ++it){
		const auto &record = *it;
		m_records.emplace_back(record);
	}
	std::sort(m_records.begin(), m_records.end(),
		[](const boost::shared_ptr<MySql::Center_TaxRecord> &lhs, const boost::shared_ptr<MySql::Center_TaxRecord> &rhs){
			return lhs->unlocked_get_auto_uuid() < rhs->unlocked_get_auto_uuid();
		});
}
TaxBox::~TaxBox(){
}

void TaxBox::pump_status(){
	PROFILE_ME;

	const auto expiry_days = Data::Global::as_unsigned(Data::Global::SLOT_TAX_RECORD_EXPIRY_DAYS);
	const auto expired_before = Poseidon::get_utc_time_from_local((Poseidon::get_local_time() / 86400000 - expiry_days) * 86400000);
	for(;;){
		if(m_records.empty()){
			break;
		}
		const auto obj = m_records.front();
		if(obj->get_timestamp() >= expired_before){
			break;
		}
		LOG_EMPERY_CENTER_DEBUG("Removing expired tax record: account_uuid = ", obj->unlocked_get_account_uuid(),
			", auto_uuid = ", obj->unlocked_get_auto_uuid());
		obj->set_deleted(true);
		m_records.pop_front();
	}
}

void TaxBox::get_all(std::vector<TaxBox::RecordInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_records.size());
	for(auto it = m_records.begin(); it != m_records.end(); ++it){
		RecordInfo info;
		fill_record_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}
void TaxBox::push(std::uint64_t timestamp, AccountUuid from_account_uuid,
	ReasonId reason, std::uint64_t old_amount, std::uint64_t new_amount)
{
	PROFILE_ME;

	const auto obj = boost::make_shared<MySql::Center_TaxRecord>(Poseidon::Uuid::random(),
		get_account_uuid().get(), timestamp, from_account_uuid.get(),
		reason.get(), old_amount, new_amount, static_cast<std::int64_t>(new_amount - old_amount), false);
	obj->async_save(true);
	m_records.emplace_back(obj);

	const auto session = PlayerSessionMap::get(get_account_uuid());
	if(session){
		try {
			Msg::SC_TaxNewRecord msg;
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}
}

}
