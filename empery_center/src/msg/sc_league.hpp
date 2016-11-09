#ifndef EMPERY_CENTER_MSG_SC_LEAGUE_HPP_
#define EMPERY_CENTER_MSG_SC_LEAGUE_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_LeagueInfo
#define MESSAGE_ID      1850
#define MESSAGE_FIELDS  \
	FIELD_STRING        (league_uuid)	\
	FIELD_STRING        (league_name)	\
	FIELD_STRING        (league_icon)	\
	FIELD_STRING        (league_notice)	\
	FIELD_VUINT         (league_level)	\
	FIELD_VUINT         (league_max_member)	\
	FIELD_STRING        (leader_uuid)	\
	FIELD_STRING        (leader_name)	\
	FIELD_ARRAY         (members,	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_STRING        (legion_name)	\
		FIELD_STRING        (legion_icon)	\
		FIELD_STRING        (legion_leader_name)	\
		FIELD_VUINT         (titleid)	\
		FIELD_STRING        (quit_time)	\
		FIELD_STRING        (kick_time)	\
		FIELD_STRING        (attorn_time) \
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SC_Leagues
#define MESSAGE_ID      1851
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (leagues,	\
		FIELD_STRING        (league_uuid)	\
		FIELD_STRING        (league_name)	\
		FIELD_STRING        (league_icon)	\
		FIELD_STRING        (league_notice)	\
		FIELD_STRING        (league_leader_name)	\
		FIELD_STRING        (autojoin)	\
		FIELD_STRING        (league_create_time)	\
		FIELD_STRING        (isapplyjoin)	\
		FIELD_VUINT         (legion_count)	\
		FIELD_VUINT         (legion_max_count)	\
		FIELD_VUINT         (totalmembercount)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SC_CreateLeagueRes
#define MESSAGE_ID      1852
#define MESSAGE_FIELDS  \
	FIELD_VUINT        (res)
#include <poseidon/cbpp/message_generator.hpp>



#define MESSAGE_NAME    SC_ApplyJoinList
#define MESSAGE_ID      1853
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (applies,	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_STRING        (legion_name)	\
		FIELD_STRING        (icon)	\
		FIELD_STRING        (leader_name)	\
		FIELD_STRING        (level)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_InvieJoinList
#define MESSAGE_ID      1854
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (invites,	\
		FIELD_STRING        (league_uuid)	\
		FIELD_STRING        (league_name)	\
		FIELD_STRING        (league_icon)	\
		FIELD_STRING        (leader_name)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_LeagueNoticeMsg
#define MESSAGE_ID      1855
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (msgtype)	\
	FIELD_STRING        (nick)	\
	FIELD_STRING        (ext1)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_LookLeagueMembers
#define MESSAGE_ID      1856
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (members,	\
		FIELD_STRING        (account_uuid)	\
		FIELD_STRING        (titleid)	\
		FIELD_STRING        (nick)	\
		FIELD_STRING        (icon)	\
		FIELD_VUINT         (prosperity)	\
		FIELD_STRING        (speakflag)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_LookLeagueLegions
#define MESSAGE_ID      1857
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_name)	\
		FIELD_STRING        (legion_icon)	\
		FIELD_STRING        (legion_leader_name)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


/*
#define MESSAGE_NAME    SC_LegionMembers
#define MESSAGE_ID      1654
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (members,	\
		FIELD_STRING        (account_uuid)	\
		FIELD_STRING        (speakflag)	\
		FIELD_STRING        (titleid)	\
		FIELD_STRING        (nick)	\
		FIELD_STRING        (icon)	\
		FIELD_VUINT         (prosperity)	\
		FIELD_STRING        (fighting)	\
		FIELD_STRING        (quit_time)	\
		FIELD_STRING        (kick_time)	\
		FIELD_STRING        (attorn_time) \
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_Legions
#define MESSAGE_ID      1655
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_STRING        (legion_name)	\
		FIELD_STRING        (legion_icon)	\
		FIELD_STRING        (legion_leadername)	\
		FIELD_STRING        (level)	\
		FIELD_STRING        (membercount)	\
		FIELD_STRING        (autojoin)	\
		FIELD_STRING        (rank)	\
		FIELD_STRING        (notice)	\
		FIELD_STRING        (legion_Leader_maincity_loard_level) \
		FIELD_STRING        (legion_create_time) \
		FIELD_STRING        (isapplyjoin) \
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_ApplyList
#define MESSAGE_ID      1656
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (applies,	\
		FIELD_STRING        (account_uuid)	\
		FIELD_STRING        (nick)	\
		FIELD_STRING        (icon)	\
		FIELD_VUINT         (prosperity)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_LookLegionMembers
#define MESSAGE_ID      1657
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (members,	\
		FIELD_STRING        (titleid)	\
		FIELD_STRING        (nick)	\
		FIELD_STRING        (icon)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SC_LegionInviteList
#define MESSAGE_ID      1658
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (invites,	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_STRING        (legion_name)	\
		FIELD_STRING        (icon)	\
		FIELD_STRING        (leader_name) \
		FIELD_STRING        (level)	\
		FIELD_STRING        (membercount)	\
		FIELD_STRING        (autojoin)	\
		FIELD_STRING        (rank)	\
		FIELD_STRING        (notice)	\
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SC_SearchLegionRes
#define MESSAGE_ID      1659
#define MESSAGE_FIELDS  \
	FIELD_ARRAY         (legions,	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_STRING        (legion_name)	\
		FIELD_STRING        (legion_icon)	\
		FIELD_STRING        (legion_leadername)	\
		FIELD_STRING        (level)	\
		FIELD_STRING        (membercount)	\
		FIELD_STRING        (autojoin)	\
		FIELD_STRING        (rank)	\
		FIELD_STRING        (notice)	\
		FIELD_STRING        (legion_Leader_maincity_loard_level) \
		FIELD_STRING        (legion_create_time) \
		FIELD_STRING        (isapplyjoin) \
	)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SC_LegionNoticeMsg
#define MESSAGE_ID      1660
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (msgtype)	\
	FIELD_STRING        (nick)	\
	FIELD_STRING        (ext1)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    SC_LegionStoreRecordInfo
#define MESSAGE_ID      1661
#define MESSAGE_FIELDS  \
	FIELD_STRING        (recordinfo)

#include <poseidon/cbpp/message_generator.hpp>

*/

#define MESSAGE_NAME    SC_OtherLeagueInfo
#define MESSAGE_ID      1858
#define MESSAGE_FIELDS  \
	FIELD_STRING        (league_uuid)	\
	FIELD_STRING        (league_name)	\
	FIELD_STRING        (league_icon)	\
	FIELD_STRING        (league_notice)	\
	FIELD_VUINT         (league_level)	\
	FIELD_VUINT         (league_max_member)	\
	FIELD_STRING        (leader_uuid)	\
	FIELD_STRING        (leader_name)	\
	FIELD_ARRAY         (members,	\
		FIELD_STRING        (legion_uuid)	\
		FIELD_STRING        (legion_name)	\
		FIELD_STRING        (legion_icon)	\
		FIELD_STRING        (legion_leader_name)	\
		FIELD_VUINT         (titleid)	\
		FIELD_STRING        (quit_time)	\
		FIELD_STRING        (kick_time)	\
		FIELD_STRING        (attorn_time) \
	)\
	FIELD_STRING        (other_account_uuid)
#include <poseidon/cbpp/message_generator.hpp>
}

}

#endif
