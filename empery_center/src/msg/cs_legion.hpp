#ifndef EMPERY_CENTER_MSG_CS_LEGION_HPP_
#define EMPERY_CENTER_MSG_CS_LEGION_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME    CS_LegionCreateReqMessage
#define MESSAGE_ID      1600
#define MESSAGE_FIELDS  \
	FIELD_STRING         (name)	\
	FIELD_STRING         (icon)	\
	FIELD_STRING         (content)	\
	FIELD_STRING		 (language)	\
	FIELD_VUINT          (bshenhe)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_GetLegionBaseInfoMessage
#define MESSAGE_ID      1601
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_GetLegionMemberInfoMessage
#define MESSAGE_ID      1602
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_GetAllLegionMessage
#define MESSAGE_ID      1603
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_ApplyJoinLegionMessage
#define MESSAGE_ID      1604
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_uuid) \
	FIELD_VUINT          (bCancle)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_GetApplyJoinLegionMessage
#define MESSAGE_ID      1605
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_LegionAuditingResMessage
#define MESSAGE_ID      1606
#define MESSAGE_FIELDS  \
	FIELD_STRING         (account_uuid)	\
	FIELD_VUINT          (bagree)
#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_LegionInviteJoinReqMessage
#define MESSAGE_ID      1607
#define MESSAGE_FIELDS  \
	FIELD_STRING         (nick)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_QuitLegionReqMessage
#define MESSAGE_ID      1608
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_CancelQuitLegionReqMessage
#define MESSAGE_ID      1609
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_LookLegionMembersReqMessage
#define MESSAGE_ID      1610
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_uuid)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_LegionMemberGradeReqMessage
#define MESSAGE_ID      1611
#define MESSAGE_FIELDS  \
	FIELD_STRING         (account_uuid)	\
	FIELD_VUINT          (bup)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_KickLegionMemberReqMessage
#define MESSAGE_ID      1612
#define MESSAGE_FIELDS  \
	FIELD_STRING         (account_uuid)	\
	FIELD_VUINT          (bCancle)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_AttornLegionReqMessage
#define MESSAGE_ID      1613
#define MESSAGE_FIELDS  \
	FIELD_STRING         (account_uuid)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_CancleAttornLegionReqMessage
#define MESSAGE_ID      1614
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_disbandLegionReqMessage
#define MESSAGE_ID      1615
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_banChatLegionReqMessage
#define MESSAGE_ID      1616
#define MESSAGE_FIELDS  \
	FIELD_STRING         (account_uuid)	\
	FIELD_VUINT          (bban)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_LegionNoticeReqMessage
#define MESSAGE_ID      1617
#define MESSAGE_FIELDS  \
	FIELD_STRING         (notice)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_LegionInviteJoinResMessage
#define MESSAGE_ID      1618
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_uuid) \
    FIELD_VUINT          (bagree)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_GetLegionInviteListMessage
#define MESSAGE_ID      1619
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_SearchLegionMessage
#define MESSAGE_ID      1620
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_name)	\
	FIELD_STRING		 (leader_name)	\
	FIELD_STRING         (language)

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_LegionChatMessage
#define MESSAGE_ID      1621
#define MESSAGE_FIELDS  \
    FIELD_STRING         (content)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_LegionDonateMessage
#define MESSAGE_ID      1622
#define MESSAGE_FIELDS  \
    FIELD_VUINT         (num)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_LegionExchangeItemMessage
#define MESSAGE_ID      1623
#define MESSAGE_FIELDS  \
	FIELD_VUINT          (goodsid) \
    FIELD_VUINT         (num)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_LegionExchangeItemRecordMessage
#define MESSAGE_ID      1624
#define MESSAGE_FIELDS  \

#include <poseidon/cbpp/message_generator.hpp>


#define MESSAGE_NAME    CS_LegionContribution
#define MESSAGE_ID      1637
#define MESSAGE_FIELDS  \
	FIELD_STRING         (legion_uuid)

#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ModifyLegionNameReqMessage
#define MESSAGE_ID      1639
#define MESSAGE_FIELDS  \
	FIELD_STRING         (name)	
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ModifyLegionIconReqMessage
#define MESSAGE_ID      1640
#define MESSAGE_FIELDS  \
	FIELD_STRING         (icon)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ModifyLegionLanguageReqMessage
#define MESSAGE_ID      1641
#define MESSAGE_FIELDS  \
	FIELD_STRING		 (language)	
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    CS_ModifyLegionSwitchStatusReqMessage
#define MESSAGE_ID      1642
#define MESSAGE_FIELDS  \
	FIELD_VUINT          (switchstatus)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
