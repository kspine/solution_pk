#ifndef EMPERY_GOLD_SCRAMBLE_MSG_CS_ACCOUNT_HPP_
#define EMPERY_GOLD_SCRAMBLE_MSG_CS_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyGoldScramble {

namespace Msg {

#define MESSAGE_NAME	CS_AccountLogin
#define MESSAGE_ID		200
#define MESSAGE_FIELDS	\
	FIELD_STRING		(sessionId)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_AccountBidUsingGoldCoins
#define MESSAGE_ID		201
#define MESSAGE_FIELDS	\
	//
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	CS_AccountBidUsingAccountBalance
#define MESSAGE_ID		202
#define MESSAGE_FIELDS	\
	//
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
