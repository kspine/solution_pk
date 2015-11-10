#ifndef EMPERY_CENTER_MYSQL_CASTLE_TECH_HPP_
#define EMPERY_CENTER_MYSQL_CASTLE_TECH_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME	Center_CastleTech
#define MYSQL_OBJECT_FIELDS	\
	FIELD_UUID				(mapObjectUuid)	\
	FIELD_INTEGER_UNSIGNED	(techId)	\
	FIELD_BIGINT_UNSIGNED	(techLevel)	\
	FIELD_INTEGER_UNSIGNED	(mission)	\
	FIELD_BIGINT_UNSIGNED	(missionDuration)	\
	FIELD_BIGINT_UNSIGNED	(missionParam2)	\
	FIELD_DATETIME			(missionTimeBegin)	\
	FIELD_DATETIME			(missionTimeEnd)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
