#ifndef EMPERY_CENTER_MSG_SL_LEAGUE_HPP_
#define EMPERY_CENTER_MSG_SL_LEAGUE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SL_LeagueCreated
#define MESSAGE_ID      58801
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (name)	\
	FIELD_STRING        (icon)	\
	FIELD_STRING        (content)	\
	FIELD_STRING		(language)	\
	FIELD_VUINT         (bautojoin)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_LeagueInfo
#define MESSAGE_ID      58802
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_GetAllLeagueInfo
#define MESSAGE_ID      58803
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_SearchLeague
#define MESSAGE_ID      58804
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (league_name)	\
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SL_ApplyJoinLeague
#define MESSAGE_ID      58805
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (league_uuid)	\
	FIELD_VUINT         (bCancle)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_GetApplyJoinLeague
#define MESSAGE_ID      58806
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_LeagueAuditingRes
#define MESSAGE_ID      58807
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (target_legion_uuid)	\
	FIELD_VUINT          (bagree)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_LeagueInviteJoin
#define MESSAGE_ID      58808
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (target_legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_LeagueInviteList
#define MESSAGE_ID      58809
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_LeagueInviteJoinRes
#define MESSAGE_ID      58810
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (league_uuid)	\
	FIELD_VUINT         (bagree)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
