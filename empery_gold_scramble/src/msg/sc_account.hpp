#ifndef EMPERY_GOLD_SCRAMBLE_MSG_SC_ACCOUNT_HPP_
#define EMPERY_GOLD_SCRAMBLE_MSG_SC_ACCOUNT_HPP_

#include <poseidon/cbpp/message_base.hpp>

namespace EmperyGoldScramble {

namespace Msg {

#define MESSAGE_NAME    SC_AccountLoginSuccess
#define MESSAGE_ID      299
#define MESSAGE_FIELDS  \
	FIELD_STRING        (login_name)	\
	FIELD_STRING        (nick)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountAccountBalance
#define MESSAGE_ID      298
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (gold_coins)	\
	FIELD_VUINT         (account_balance)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountAuctionStatus
#define MESSAGE_ID      297
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (server_time)	\
	FIELD_VUINT         (begin_time)	\
	FIELD_VUINT         (end_duration)	\
	FIELD_VUINT         (gold_coins_in_pot)	\
	FIELD_VUINT         (account_balance_in_pot)	\
	FIELD_VUINT         (number_of_winners)	\
	FIELD_VUINT         (percent_winners)	\
	FIELD_ARRAY         (records,	\
		FIELD_VUINT         (record_auto_id)	\
		FIELD_VUINT         (timestamp)	\
		FIELD_STRING        (login_name)	\
		FIELD_STRING        (nick)	\
		FIELD_VUINT         (gold_coins)	\
		FIELD_VUINT         (account_balance)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountGameHistory
#define MESSAGE_ID      296
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (record_auto_id)	\
	FIELD_VUINT         (timestamp)	\
	FIELD_STRING        (login_name)	\
	FIELD_STRING        (nick)	\
	FIELD_VUINT         (gold_coins_won)	\
	FIELD_VUINT         (account_balance_won)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME    SC_AccountLastLog
#define MESSAGE_ID      295
#define MESSAGE_FIELDS  \
	FIELD_VUINT         (game_begin_time)	\
	FIELD_VUINT         (gold_coins_in_pot)	\
	FIELD_VUINT         (account_balance_in_pot)	\
	FIELD_VUINT         (number_of_winners)	\
	FIELD_ARRAY         (players,	\
		FIELD_STRING        (login_name)	\
		FIELD_STRING        (nick)	\
	)
#include <poseidon/cbpp/message_generator.hpp>

}

}

#endif
