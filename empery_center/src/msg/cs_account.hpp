#ifndef EMPERY_CENTER_MSG_CS_ACCOUNT_HPP_
#define EMPERY_CENTER_MSG_CS_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyCenter {

namespace Msg {

#define MESSAGE_NAME	CS_AccountLogin
#define MESSAGE_ID		200
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(platformId)	\
	FIELD_STRING		(loginName)	\
	FIELD_STRING		(loginToken)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_AccountSetAttribute
#define MESSAGE_ID		201
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(slot)	\
	FIELD_STRING		(value)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_AccountSetNick
#define MESSAGE_ID		202
#define MESSAGE_FIELDS	\
	FIELD_STRING		(nick)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_AccountFindByNick
#define MESSAGE_ID		203
#define MESSAGE_FIELDS	\
	FIELD_STRING		(nick)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_AccountQueryAttributes
#define MESSAGE_ID		204
#define MESSAGE_FIELDS	\
	FIELD_ARRAY			(accounts,	\
		FIELD_STRING		(accountUuid)	\
		FIELD_VUINT			(detailFlags)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif