#ifndef EMPERY_CENTER_MYSQL_AUCTION_HPP_
#define EMPERY_CENTER_MYSQL_AUCTION_HPP_

#include <poseidon/mysql/object_base.hpp>

namespace EmperyCenter {

namespace MySql {

#define MYSQL_OBJECT_NAME   Center_AuctionTransferRequest
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_UUID              (map_object_uuid)	\
	FIELD_DATETIME          (created_time)	\
	FIELD_DATETIME          (due_time)
#include <poseidon/mysql/object_generator.hpp>

#define MYSQL_OBJECT_NAME   Center_AuctionTransferRequestItem
#define MYSQL_OBJECT_FIELDS \
	FIELD_UUID              (account_uuid)	\
	FIELD_UUID              (map_object_uuid)	\
	FIELD_INTEGER_UNSIGNED  (item_id)	\
	FIELD_BIGINT_UNSIGNED   (count)
#include <poseidon/mysql/object_generator.hpp>

}

}

#endif
