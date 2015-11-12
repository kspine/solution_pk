#ifndef EMPERY_GOLD_SCRAMBLE_MYSQL_BID_HISTORY_HPP_
#define EMPERY_GOLD_SCRAMBLE_MYSQL_BID_HISTORY_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyGoldScramble {

namespace MySql {

#define MYSQL_OBJECT_NAME   GoldScramble_BidHistory
#define MYSQL_OBJECT_FIELDS \
	FIELD_BIGINT_UNSIGNED   (recordAutoId)	\
	FIELD_BIGINT_UNSIGNED   (gameAutoId)	\
	FIELD_DATETIME          (timestamp)	\
	FIELD_STRING            (loginName)	\
	FIELD_STRING            (nick)	\
	FIELD_BIGINT_UNSIGNED   (goldCoins)	\
	FIELD_BIGINT_UNSIGNED   (accountBalance)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
