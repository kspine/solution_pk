#ifndef EMPERY_CENTER_MSG_LS_LEAGUE_HPP_
#define EMPERY_CENTER_MSG_LS_LEAGUE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    LS_LeagueRegisterServer
#define MESSAGE_ID      51000
#define MESSAGE_FIELDS  \
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_LeagueInfo
#define MESSAGE_ID      51001
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (res)	\
	FIELD_VUINT         (rewrite)	\
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (league_uuid)	\
	FIELD_STRING        (league_name)	\
	FIELD_STRING        (league_icon)	\
	FIELD_STRING        (league_notice)	\
	FIELD_VUINT         (league_level)	\
	FIELD_VUINT         (league_max_member)	\
	FIELD_STRING        (leader_legion_uuid)	\
	FIELD_STRING        (create_account_uuid)	\
	FIELD_ARRAY         (members,	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_VUINT         (titleid)	\
		FIELD_STRING        (quit_time)	\
		FIELD_STRING        (kick_time)	\
		FIELD_STRING        (attorn_time) \
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_Leagues
#define MESSAGE_ID      51002
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_ARRAY         (leagues,	\
		FIELD_STRING        (league_uuid)	\
		FIELD_STRING        (league_name)	\
		FIELD_STRING        (league_icon)	\
		FIELD_STRING        (league_notice)	\
		FIELD_STRING        (league_leader_uuid)	\
		FIELD_STRING        (autojoin)	\
		FIELD_STRING        (league_create_time) \
		FIELD_STRING        (isapplyjoin) \
		FIELD_VUINT         (legion_max_count)	\
		FIELD_ARRAY         (legions,	\
			FIELD_STRING        (legion_uuid)	\
		)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_CreateLeagueRes
#define MESSAGE_ID      51003
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (res)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_ApplyJoinList
#define MESSAGE_ID      51004
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_ARRAY         (applies,	\
		FIELD_STRING        (legion_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    LS_InvieJoinList
#define MESSAGE_ID      51005
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_ARRAY         (invites,	\
		FIELD_STRING        (league_uuid)	\
		FIELD_STRING        (league_name)	\
		FIELD_STRING        (league_icon)	\
		FIELD_STRING        (league_leader_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_ExpandLeagueReq
#define MESSAGE_ID      51006
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_ARRAY         (consumes,	\
		FIELD_STRING        (consue_type)	\
		FIELD_VUINT         (num)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    LS_AttornLeagueRes
#define MESSAGE_ID      51007
#define MESSAGE_FIELDS  \
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (target_legion_uuid)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    LS_banChatLeagueReq
#define MESSAGE_ID      51008
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_VUINT         (bban)	\
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    LS_LeagueChat
#define MESSAGE_ID      51009
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (chat_message_uuid)	\
	FIELD_VUINT         (channel)	\
	FIELD_VUINT         (type)	\
	FIELD_VUINT         (language_id)	\
	FIELD_VUINT         (created_time)	\
	FIELD_ARRAY         (segments,	\
		FIELD_VUINT         (slot)	\
		FIELD_STRING        (value)	\
	) \
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_LeagueNoticeMsg
#define MESSAGE_ID      51010
#define MESSAGE_FIELDS  \
	FIELD_STRING        (league_uuid)	\
	FIELD_VUINT         (msgtype)	\
	FIELD_STRING        (nick)	\
	FIELD_STRING        (ext1) 	\
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    LS_LeagueEamilMsg
#define MESSAGE_ID      51011
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (ntype)	\
	FIELD_STRING        (ext1) 	\
	FIELD_STRING        (mandator)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_LookLeagueLegionsReq
#define MESSAGE_ID      51012
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_uuid)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    LS_disbandLegionRes
#define MESSAGE_ID      51013
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (legion_uuid)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    LS_disbandLeagueRes
#define MESSAGE_ID      51014
#define MESSAGE_FIELDS  \
	FIELD_STRING        (account_uuid)	\
	FIELD_STRING        (league_name)	\
	FIELD_STRING        (legion_uuid)

#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
