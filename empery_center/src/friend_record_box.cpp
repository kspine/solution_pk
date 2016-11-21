#include "precompiled.hpp"
#include "friend_record_box.hpp"
#include "mysql/friend.hpp"
#include "data/global.hpp"
#include "singletons/player_session_map.hpp"
#include "player_session.hpp"

namespace EmperyCenter {

namespace {
	void fill_record_info(FriendRecordBox::RecordInfo &info, const boost::shared_ptr<MySql::Center_FriendRecord> &obj){
		PROFILE_ME;

		info.auto_uuid              = obj->unlocked_get_auto_uuid();
		info.timestamp              = obj->get_timestamp();
		info.friend_account_uuid    = AccountUuid(obj->unlocked_get_friend_account_uuid());
		info.result_type            = obj->get_result_type();
	}
}

FriendRecordBox::FriendRecordBox(AccountUuid account_uuid,
	const std::vector<boost::shared_ptr<MySql::Center_FriendRecord>> &records)
	: m_account_uuid(account_uuid)
{
	for(auto it = records.begin(); it != records.end(); ++it){
		const auto &record = *it;
		m_records.emplace_back(record);
	}
	std::sort(m_records.begin(), m_records.end(),
		[](const boost::shared_ptr<MySql::Center_FriendRecord> &lhs, const boost::shared_ptr<MySql::Center_FriendRecord> &rhs){
			return lhs->unlocked_get_auto_uuid() < rhs->unlocked_get_auto_uuid();
		});
}
FriendRecordBox::~FriendRecordBox(){
}

void FriendRecordBox::pump_status(){
	PROFILE_ME;

	const auto expiry_days = Data::Global::as_unsigned(Data::Global::SLOT_FRIEND_RECORD_EXPIRY_DAYS);
	const auto expired_before = Poseidon::get_utc_time_from_local((Poseidon::get_local_time() / 86400000 - expiry_days) * 86400000);
	for(;;){
		if(m_records.empty()){
			break;
		}
		const auto obj = m_records.front();
		if(obj->get_timestamp() >= expired_before){
			break;
		}
		LOG_EMPERY_CENTER_DEBUG("Removing expired friend record: account_uuid = ", get_account_uuid(),
			", auto_uuid = ", obj->unlocked_get_auto_uuid());
		m_records.pop_front();
		obj->set_deleted(true);
	}
}

void FriendRecordBox::get_all(std::vector<FriendRecordBox::RecordInfo> &ret) const {
	PROFILE_ME;

	ret.reserve(ret.size() + m_records.size());
	for(auto it = m_records.begin(); it != m_records.end(); ++it){
		RecordInfo info;
		fill_record_info(info, *it);
		ret.emplace_back(std::move(info));
	}
}
void FriendRecordBox::push(std::uint64_t timestamp,AccountUuid friend_account_uuid,int result_type)
{
	PROFILE_ME;

	const auto obj = boost::make_shared<MySql::Center_FriendRecord>(Poseidon::Uuid::random(),get_account_uuid().get(), timestamp,friend_account_uuid.get(),result_type,0);
	obj->async_save(true);
	m_records.emplace_back(obj);

	const auto max_record_count = Data::Global::as_unsigned(Data::Global::SLOT_MAX_FRIEND_RECORD_COUNT);
	while(m_records.size() > max_record_count){
		const auto obj = std::move(m_records.front());
		m_records.pop_front();
		obj->set_deleted(true);
	}
}

}
