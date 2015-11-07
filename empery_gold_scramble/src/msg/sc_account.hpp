#ifndef EMPERY_GOLD_SCRAMBLE_MSG_SC_ACCOUNT_HPP_
#define EMPERY_GOLD_SCRAMBLE_MSG_SC_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyGoldScramble {

namespace Msg {

#define MESSAGE_NAME	SC_AccountLoginSuccess
#define MESSAGE_ID		299
#define MESSAGE_FIELDS	\
	FIELD_STRING		(loginName)	\
	FIELD_STRING		(nick)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	SC_AccountAccountBalance
#define MESSAGE_ID		298
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(goldCoins)	\
	FIELD_VUINT			(accountBalance)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	SC_AccountAuctionStatus
#define MESSAGE_ID		297
#define MESSAGE_FIELDS  \
	FIELD_VUINT			(beginTime)	\
	FIELD_VUINT			(endDuration)	\
	FIELD_VUINT			(goldCoinsInPot)	\
	FIELD_VUINT			(accountBalanceInPot)	\
	FIELD_STRING		(lastUserLoginName)	\
	FIELD_STRING		(lastUserNick)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	SC_AccountAuctionHistory
#define MESSAGE_ID		296
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(autoId)	\
	FIELD_VUINT			(timestamp)	\
	FIELD_VUINT			(goldCoins)	\
	FIELD_VUINT			(accountBalance)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	SC_AccountGameHistory
#define MESSAGE_ID		295
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(autoId)	\
	FIELD_VUINT			(timestamp)	\
	FIELD_VUINT			(goldCoinsInPot)	\
	FIELD_VUINT			(accountBalanceInPot)	\
	FIELD_STRING		(lastUserLoginName)	\
	FIELD_STRING		(lastUserNick)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
