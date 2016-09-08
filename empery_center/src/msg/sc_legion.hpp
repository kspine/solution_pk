#ifndef EMPERY_CENTER_MSG_SC_LEGION_HPP_
#define EMPERY_CENTER_MSG_SC_LEGION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    SC_LegionInfo
#define MESSAGE_ID      1653
#define MESSAGE_FIELDS  \
	FIELD_STRING        (legion_uuid)	\
	FIELD_STRING        (legion_name)	\
	FIELD_STRING        (legion_leadername)	\
	FIELD_STRING        (legion_icon)	\
	FIELD_STRING        (legion_notice)	\
	FIELD_STRING         (legion_level)	\
	FIELD_STRING         (legion_rank)	\
	FIELD_STRING         (legion_money)	\
	FIELD_STRING         (legion_titleid)	\
	FIELD_STRING         (legion_donate)	\
	FIELD_STRING         (legion_member_count)
#include <poseidon/cbpp/message_generator.hpp>

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

#define MESSAGE_NAME    SC_LegionContributions
#define MESSAGE_ID      1663
#define MESSAGE_FIELDS  \
	FIELD_STRING        (legion_uuid)	\
	FIELD_ARRAY         (contributions,	\
		FIELD_STRING        (account_uuid)	\
		FIELD_STRING        (account_nick)	\
		FIELD_VUINT         (day_contribution)	\
		FIELD_VUINT         (week_contribution)	\
		FIELD_VUINT         (total_contribution) \
	)

#include <poseidon/cbpp/message_generator.hpp>


}

}

#endif
