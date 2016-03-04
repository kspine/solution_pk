#ifndef EMPERY_CENTER_BATTLE_RECORD_BOX_HPP_
#define EMPERY_CENTER_BATTLE_RECORD_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <deque>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"
#include "coord.hpp"

namespace EmperyCenter {

namespace MySql {
	class Center_BattleRecord;
}

class BattleRecordBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
public:
	enum ResultType {
		RT_DAMAGE_NORMAL    = 1, // 普通伤害
		RT_DAMAGE_DODGED    = 2, // 闪避
		RT_DAMAGE_CRITICAL  = 3, // 暴击
	};

	struct RecordInfo {
		Poseidon::Uuid auto_uuid;
		std::uint64_t timestamp;
		MapObjectTypeId first_object_type_id;
		Coord first_coord;
		AccountUuid second_account_uuid;
		MapObjectTypeId second_object_type_id;
		Coord second_coord;
		int result_type;
		std::int64_t result_param1;
		std::int64_t result_param2;
		std::uint64_t soldiers_damaged;
		std::uint64_t soldiers_remaining;
	};

private:
	const AccountUuid m_account_uuid;

	std::deque<boost::shared_ptr<MySql::Center_BattleRecord>> m_records;

public:
	BattleRecordBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_BattleRecord>> &records);
	~BattleRecordBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void get_all(std::vector<RecordInfo> &ret) const;

	void push(std::uint64_t timestamp, MapObjectTypeId first_object_type_id, Coord first_coord,
		AccountUuid second_account_uuid, MapObjectTypeId second_object_type_id, Coord second_coord,
		int result_type, std::int64_t result_param1, std::int64_t result_param2, std::uint64_t soldiers_damaged, std::uint64_t soldiers_remaining);
};

}

#endif
