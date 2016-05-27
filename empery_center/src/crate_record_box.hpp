#ifndef EMPERY_CENTER_CRATE_RECORD_BOX_HPP_
#define EMPERY_CENTER_CRATE_RECORD_BOX_HPP_

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
	class Center_BattleRecordCrate;
}

class CrateRecordBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
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
		Coord second_coord;
		ResourceId resource_id;
		std::uint64_t resource_harvested;
		std::uint64_t resource_gained;
		std::uint64_t resource_remaining;
	};

private:
	const AccountUuid m_account_uuid;

	std::deque<boost::shared_ptr<MySql::Center_BattleRecordCrate>> m_records;

public:
	CrateRecordBox(AccountUuid account_uuid,
		const std::vector<boost::shared_ptr<MySql::Center_BattleRecordCrate>> &records);
	~CrateRecordBox();

public:
	virtual void pump_status();

	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	void get_all(std::vector<RecordInfo> &ret) const;

	void push(std::uint64_t timestamp, MapObjectTypeId first_object_type_id, Coord first_coord, Coord second_coord,
		ResourceId resource_id, std::uint64_t resource_harvested, std::uint64_t resource_gained, std::uint64_t resource_remaining);
};

}

#endif
