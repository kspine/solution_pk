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
	lib/libposeidon-empery-center.la

lib_libposeidon_empery_center_la_SOURCES = \
	src/singletons/player_websocket_server.cpp	\
	src/singletons/cluster_cbpp_server.cpp	\
	src/singletons/data_http_server.cpp	\
	src/singletons/admin_http_server.cpp	\
	src/singletons/auction_http_server.cpp	\
	src/singletons/account_http_server.cpp	\
	src/singletons/payment_http_server.cpp	\
	src/singletons/account_map.cpp	\
	src/singletons/player_session_map.cpp	\
	src/singletons/world_map.cpp	\
	src/singletons/item_box_map.cpp	\
	src/singletons/mail_box_map.cpp	\
	src/singletons/chat_box_map.cpp	\
	src/singletons/announcement_map.cpp	\
	src/singletons/activation_code_map.cpp	\
	src/singletons/simple_http_client_daemon.cpp	\
	src/singletons/auction_center_map.cpp	\
	src/singletons/payment_transaction_map.cpp	\
	src/singletons/auction_transaction_map.cpp	\
	src/singletons/global_status.cpp	\
	src/singletons/tax_box_map.cpp	\
	src/data/common.cpp	\
	src/data/global.cpp	\
	src/data/castle.cpp	\
	src/data/item.cpp	\
	src/data/signing_in.cpp	\
	src/data/map.cpp	\
	src/data/map_object.cpp	\
	src/data/vip.cpp	\
	src/player/account.cpp	\
	src/player/map.cpp	\
	src/player/castle.cpp	\
	src/player/item.cpp	\
	src/player/mail.cpp	\
	src/player/chat.cpp	\
	src/player/auction.cpp	\
	src/player/announcement.cpp	\
	src/player/tax.cpp	\
	src/admin/account.cpp	\
	src/admin/item.cpp	\
	src/admin/castle.cpp	\
	src/admin/announcement.cpp	\
	src/admin/mail.cpp	\
	src/admin/activation_code.cpp	\
	src/admin/payment.cpp	\
	src/admin/auction.cpp	\
	src/auction/query.cpp	\
	src/auction/transfer.cpp	\
	src/auction/create_bill.cpp	\
	src/auction/commit_bill.cpp	\
	src/account/promotion.cpp	\
	src/payment/create_bill.cpp	\
	src/payment/promotion_callback.cpp	\
	src/cluster/map.cpp	\
	src/string_utilities.cpp	\
	src/map_utilities.cpp	\
	src/account_utilities.cpp	\
	src/castle_utilities.cpp	\
	src/player_session.cpp	\
	src/cluster_session.cpp	\
	src/data_session.cpp	\
	src/admin_session.cpp	\
	src/auction_session.cpp	\
	src/account_session.cpp	\
	src/payment_session.cpp	\
	src/account.cpp	\
	src/map_cell.cpp	\
	src/map_object.cpp	\
	src/castle.cpp	\
	src/item_box.cpp	\
	src/mail_box.cpp	\
	src/mail_data.cpp	\
	src/chat_box.cpp	\
	src/chat_message.cpp	\
	src/announcement.cpp	\
	src/overlay.cpp	\
	src/activation_code.cpp	\
	src/auction_center.cpp	\
	src/payment_transaction.cpp	\
	src/auction_transaction.cpp	\
	src/tax_box.cpp	\
	src/mmain.cpp
