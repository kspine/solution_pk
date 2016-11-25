#ifndef EMPERY_CENTER_MSG_CS_ACTIVITY_HPP_
#define EMPERY_CENTER_MSG_CS_ACTIVITY_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_MapActivityInfo
#define MESSAGE_ID      1400
#define MESSAGE_FIELDS  \
        //
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_MapActivityKillSolidersRank
#define MESSAGE_ID      1401
#define MESSAGE_FIELDS  \
        //
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_WorldActivityInfo
#define MESSAGE_ID      1402
#define MESSAGE_FIELDS  \
        //
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_WorldActivityRank
#define MESSAGE_ID      1403
#define MESSAGE_FIELDS  \
        //
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_WorldBossPos
#define MESSAGE_ID      1404
#define MESSAGE_FIELDS  \
        //
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ClaimWorldActivity
#define MESSAGE_ID      1405
#define MESSAGE_FIELDS  \
        FIELD_VUINT         (unique_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ClaimWorldActivityRank
#define MESSAGE_ID      1406
#define MESSAGE_FIELDS  \
       //
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_WorldActivityDropData
#define MESSAGE_ID      1407
#define MESSAGE_FIELDS  \
       FIELD_VUINT         (unique_id)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ActivityRankAwardData
#define MESSAGE_ID      1408
#define MESSAGE_FIELDS  \
       FIELD_VUINT         (unique_id)
#include <poseidon/cbpp/message_generator.hpp>




}

}

#endif
