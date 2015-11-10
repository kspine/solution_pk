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
	FIELD_VUINT			(serverTime)	\
	FIELD_VUINT			(beginTime)	\
	FIELD_VUINT			(endDuration)	\
	FIELD_VUINT			(goldCoinsInPot)	\
	FIELD_VUINT			(accountBalanceInPot)	\
	FIELD_VUINT			(numberOfWinners)	\
	FIELD_VUINT			(percentWinners)	\
	FIELD_ARRAY			(records,	\
		FIELD_VUINT			(recordAutoId)	\
		FIELD_VUINT			(timestamp)	\
		FIELD_STRING		(loginName)	\
		FIELD_STRING		(nick)	\
		FIELD_VUINT			(goldCoins)	\
		FIELD_VUINT			(accountBalance)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME	SC_AccountGameHistory
#define MESSAGE_ID		296
#define MESSAGE_FIELDS	\
	FIELD_VUINT			(recordAutoId)	\
	FIELD_VUINT			(timestamp)	\
	FIELD_STRING		(loginName)	\
	FIELD_STRING		(nick)	\
	FIELD_VUINT			(goldCoinsWon)	\
	FIELD_VUINT			(accountBalanceWon)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
