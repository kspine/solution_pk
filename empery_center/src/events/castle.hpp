#ifndef EMPERY_CENTER_EVENTS_CASTLE_HPP_
#define EMPERY_CENTER_EVENTS_CASTLE_HPP_

#include <poseidon/event_base.hpp>
#include "../id_types.hpp"

namespace EmperyCenter {

namespace Events {
	struct CastleResourceChanged : public Poseidon::EventBase<340301> {
		MapObjectUuid map_object_uuid;
		AccountUuid owner_uuid;
		ResourceId resource_id;
		std::uint64_t old_amount;
		std::uint64_t new_amount;

		ReasonId reason;
		std::int64_t param1;
		std::int64_t param2;
		std::int64_t param3;

		CastleResourceChanged(MapObjectUuid map_object_uuid_, AccountUuid owner_uuid_,
			ResourceId resource_id_, std::uint64_t old_amount_, std::uint64_t new_amount_,
			ReasonId reason_, std::int64_t param1_, std::int64_t param2_, std::int64_t param3_)
			: map_object_uuid(map_object_uuid_), owner_uuid(owner_uuid_)
			, resource_id(resource_id_), old_amount(old_amount_), new_amount(new_amount_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};

	struct CastleSoldierChanged : public Poseidon::EventBase<340302> {
		MapObjectUuid map_object_uuid;
		AccountUuid owner_uuid;
		MapObjectTypeId map_object_type_id;
		std::uint64_t old_count;
		std::uint64_t new_count;

		ReasonId reason;
		std::int64_t param1;
		std::int64_t param2;
		std::int64_t param3;

		CastleSoldierChanged(MapObjectUuid map_object_uuid_, AccountUuid owner_uuid_,
			MapObjectTypeId map_object_type_id_, std::uint64_t old_count_, std::uint64_t new_count_,
			ReasonId reason_, std::int64_t param1_, std::int64_t param2_, std::int64_t param3_)
			: map_object_uuid(map_object_uuid_), owner_uuid(owner_uuid_)
			, map_object_type_id(map_object_type_id_), old_count(old_count_), new_count(new_count_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};

	struct CastleWoundedSoldierChanged : public Poseidon::EventBase<340303> {
		MapObjectUuid map_object_uuid;
		AccountUuid owner_uuid;
		MapObjectTypeId map_object_type_id;
		std::uint64_t old_count;
		std::uint64_t new_count;

		ReasonId reason;
		std::int64_t param1;
		std::int64_t param2;
		std::int64_t param3;

		CastleWoundedSoldierChanged(MapObjectUuid map_object_uuid_, AccountUuid owner_uuid_,
			MapObjectTypeId map_object_type_id_, std::uint64_t old_count_, std::uint64_t new_count_,
			ReasonId reason_, std::int64_t param1_, std::int64_t param2_, std::int64_t param3_)
			: map_object_uuid(map_object_uuid_), owner_uuid(owner_uuid_)
			, map_object_type_id(map_object_type_id_), old_count(old_count_), new_count(new_count_)
			, reason(reason_), param1(param1_), param2(param2_), param3(param3_)
		{
		}
	};

	struct CastleProtection : public Poseidon::EventBase<340304> {
		MapObjectUuid map_object_uuid;
		AccountUuid owner_uuid;
		std::uint64_t delta_preparation_duration;
		std::uint64_t delta_protection_duration;
		std::uint64_t protection_end;

		CastleProtection(MapObjectUuid map_object_uuid_, AccountUuid owner_uuid_,
			std::uint64_t delta_preparation_duration_, std::uint64_t delta_protection_duration_,
			std::uint64_t protection_end_)
			: map_object_uuid(map_object_uuid_), owner_uuid(owner_uuid_)
			, delta_preparation_duration(delta_preparation_duration_), delta_protection_duration(delta_protection_duration_)
			, protection_end(protection_end_)
		{
		}
	};
}

}

#endif
