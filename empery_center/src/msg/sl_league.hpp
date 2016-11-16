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
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (league_uuid)
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
	FIELD_STRING        (legion_uuid)	\
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

#define MESSAGE_NAME    SL_ExpandLeagueReq
#define MESSAGE_ID      58811
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_ExpandLeagueRes
#define MESSAGE_ID      58812
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_QuitLeagueReq
#define MESSAGE_ID      58813
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_VUINT         (bCancle)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_KickLeagueMemberReq
#define MESSAGE_ID      58814
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (target_legion_uuid)	\
	FIELD_VUINT         (bCancle)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SL_disbandLeague
#define MESSAGE_ID      58815
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_AttornLeagueReq
#define MESSAGE_ID      58816
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (target_legion_uuid)	\
	FIELD_VUINT         (bCancle)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_LeaguenMemberGradeReq
#define MESSAGE_ID      58817
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (target_legion_uuid)	\
	FIELD_VUINT         (bup)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SL_banChatLeagueReq
#define MESSAGE_ID      58818
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (target_legion_uuid)	\
	FIELD_STRING        (target_account_uuid)	\
	FIELD_VUINT         (bban)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SL_LeagueChatReq
#define MESSAGE_ID      58819
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (chat_message_uuid)	\
	FIELD_VUINT         (channel)	\
	FIELD_VUINT         (type)	\
	FIELD_VUINT         (language_id)	\
	FIELD_VUINT         (created_time)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SL_LookLeagueLegionsReq
#define MESSAGE_ID      58820
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (league_uuid)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SL_disbandLegionReq
#define MESSAGE_ID      58821
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_OtherLeagueInfo
#define MESSAGE_ID      58822
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (league_uuid)	\
	FIELD_STRING        (to_account_uuid)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_ModifyLeagueNoticeReq
#define MESSAGE_ID      58823
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)   \
	FIELD_STRING        (content)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_ModifyLeagueNameReq
#define MESSAGE_ID      58824
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)   \
	FIELD_STRING        (name)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SL_ModifyLeagueSwitchStatusReq
#define MESSAGE_ID      58825
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)   \
	FIELD_VUINT         (switchstatus)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
