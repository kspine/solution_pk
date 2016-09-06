ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -Wall -Wextra -Werror -Wsign-conversion -Wno-error=unused-parameter -Wno-missing-field-initializers -pipe -fPIC -DPIC -pthread
AM_CXXFLAGS =

%.hpp.gch: %.hpp
	$(CXX) -x c++-header $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) $< -o $@

BUILT_SOURCES = \
	src/precompiled.hpp.gch

CLEANFILES = \
	src/precompiled.hpp.gch

lib_LTLIBRARIES = \
	lib/libposeidon-empery-promotion.la

lib_libposeidon_empery_promotion_la_SOURCES = \
	src/data/promotion.cpp	\
	src/singletons/account_http_server.cpp	\
	src/singletons/admin_http_server.cpp	\
	src/singletons/synuser_http_server.cpp	\
	src/singletons/statistic_daemon.cpp	\
	src/singletons/account_map.cpp	\
	src/singletons/item_map.cpp	\
	src/singletons/global_status.cpp	\
	src/singletons/shared_recharge_map.cpp	\
	src/account/try_login_name.cpp	\
	src/account/query_account_attributes.cpp	\
	src/account/set_account_attributes.cpp	\
	src/account/query_account_items.cpp	\
	src/account/query_global_status.cpp	\
	src/account/check_login.cpp	\
	src/account/create.cpp	\
	src/account/upgrade.cpp	\
	src/account/transfer.cpp	\
	src/account/withdraw.cpp	\
	src/account/set_password.cpp	\
	src/account/set_deal_password.cpp	\
	src/account/get_pyramid.cpp	\
	src/account/get_income_balance_history.cpp	\
	src/account/get_outcome_balance_history.cpp	\
	src/account/get_both_balance_history.cpp	\
	src/account/get_gold_coin_history.cpp	\
	src/account/get_withdrawal_requests.cpp	\
	src/account/commit_withdrawal.cpp	\
	src/account/rollback_withdrawal.cpp	\
	src/account/create_bill.cpp	\
	src/account/settle_bill.cpp	\
	src/account/query_bill_state.cpp	\
	src/account/get_economy_summary.cpp	\
	src/account/get_account_list.cpp	\
	src/account/find_by_phone_number.cpp	\
	src/account/deactivate.cpp	\
	src/account/is_subordinate_of.cpp	\
	src/account/buy_more_cards.cpp	\
	src/account/query_account_performance.cpp	\
	src/account/add_account_items.cpp	\
	src/account/remove_account_items.cpp	\
	src/account/notify_gold_scramble_bid.cpp	\
	src/account/notify_gold_scramble_reward.cpp	\
	src/account/check_login_backtrace.cpp	\
	src/account/sell_acceleration_cards.cpp	\
	src/account/buy_diamonds.cpp	\
	src/account/buy_gift_box.cpp	\
	src/account/buy_large_gift_box.cpp	\
	src/account/buy_gold_coins.cpp	\
	src/account/auction_transfer_in.cpp	\
	src/account/auction_transfer_out.cpp	\
	src/account/enable_auction_center.cpp	\
	src/account/accumulate_balance_bonus.cpp	\
	src/account/get_shared_recharge_list.cpp	\
	src/account/get_shared_recharge_list_all.cpp	\
	src/account/request_shared_recharge.cpp	\
	src/account/accept_shared_recharge.cpp	\
	src/account/decline_shared_recharge.cpp	\
	src/account/commit_shared_recharge.cpp	\
	src/account/rollback_shared_recharge.cpp	\
	src/synuser/checkaccount.cpp	\
	src/synuser/notify.cpp	\
	src/synuser/createpayment.cpp	\
	src/synuser/querypayment.cpp	\
	src/synuser/create.cpp	\
	src/synuser/update.cpp	\
	src/checked_arithmetic.cpp	\
	src/account_http_session.cpp	\
	src/admin_http_session.cpp	\
	src/synuser_http_session.cpp	\
	src/utilities.cpp	\
	src/mmain.cpp
