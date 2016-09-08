#ifndef EMPERY_CENTER_MSG_CS_LEAGUE_HPP_
#define EMPERY_CENTER_MSG_CS_LEAGUE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_LeagueCreateReqMessage
#define MESSAGE_ID      1800
#define MESSAGE_FIELDS  \
	FIELD_STRING         (name)	\
	FIELD_STRING         (icon)	\
	FIELD_STRING         (content)	\
	FIELD_STRING		 (language)	\
	FIELD_VUINT          (bautojoin)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_GetLeagueBaseInfoMessage
#define MESSAGE_ID      1801
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_GetLeagueMemberInfoMessage
#define MESSAGE_ID      1802
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_GetAllLeagueMessage
#define MESSAGE_ID      1803
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_SearchLeagueMessage
#define MESSAGE_ID      1804
#define MESSAGE_FIELDS  \
	FIELD_STRING         (league_name)	\
	FIELD_STRING		 (leader_name)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ApplyJoinLeaguenMessage
#define MESSAGE_ID      1805
#define MESSAGE_FIELDS  \
	FIELD_STRING         (league_uuid) \
	FIELD_VUINT          (bCancle)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_GetApplyJoinLeague
#define MESSAGE_ID      1806
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_LeagueAuditingResMessage
#define MESSAGE_ID      1807
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_uuid)	\
	FIELD_VUINT          (bagree)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_LeagueInviteJoinReqMessage
#define MESSAGE_ID      1808
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_name)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_GetLeagueInviteListMessage
#define MESSAGE_ID      1809
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_LeagueInviteJoinResMessage
#define MESSAGE_ID      1810
#define MESSAGE_FIELDS  \
	FIELD_STRING         (league_uuid) \
    FIELD_VUINT          (bagree)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ExpandLeague
#define MESSAGE_ID      1811
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_QuitLeagueReqMessage
#define MESSAGE_ID      1812
#define MESSAGE_FIELDS  \
	FIELD_VUINT          (bCancle)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_KickLeagueMemberReqMessage
#define MESSAGE_ID      1813
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_uuid)	\
	FIELD_VUINT          (bCancle)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_disbandLeagueMessage
#define MESSAGE_ID      1814
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
