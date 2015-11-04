#ifndef EMPERY_CENTER_MYSQL_CASTLE_BUILDING_BASE_HPP_
#define EMPERY_CENTER_MYSQL_CASTLE_BUILDING_BASE_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME	Center_CastleBuildingBase
#define MYSQL_OBJECT_FIELDS	\
	FIELD_UUID				(mapObjectUuid)	\
	FIELD_INTEGER_UNSIGNED	(baseIndex)	\
	FIELD_INTEGER_UNSIGNED	(buildingId)	\
	FIELD_BIGINT_UNSIGNED	(buildingLevel)	\
	FIELD_INTEGER_UNSIGNED	(mission)	\
	FIELD_BIGINT_UNSIGNED	(missionParam1)	\
	FIELD_BIGINT_UNSIGNED	(missionParam2)	\
	FIELD_DATETIME			(missionTimeBegin)	\
	FIELD_DATETIME			(missionTimeEnd)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
