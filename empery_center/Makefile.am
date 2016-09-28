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
	src/singletons/controller_client.cpp	\
	src/singletons/log_client.cpp	\
	src/singletons/player_websocket_server.cpp	\
	src/singletons/cluster_cbpp_server.cpp	\
	src/singletons/dungeon_cbpp_server.cpp	\
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
	src/singletons/simple_http_client_daemon.cpp	\
	src/singletons/auction_center_map.cpp	\
	src/singletons/payment_transaction_map.cpp	\
	src/singletons/auction_transaction_map.cpp	\
	src/singletons/global_status.cpp	\
	src/singletons/tax_record_box_map.cpp	\
	src/singletons/task_box_map.cpp	\
	src/singletons/battle_record_box_map.cpp	\
	src/singletons/war_status_map.cpp	\
	src/singletons/dungeon_box_map.cpp	\
	src/singletons/dungeon_map.cpp	\
	src/singletons/friend_box_map.cpp	\
	src/singletons/legion_map.cpp	\
	src/singletons/legion_member_map.cpp	\
	src/singletons/legion_applyjoin_map.cpp	\
	src/singletons/legion_invitejoin_map.cpp	\
	src/singletons/legion_package_share_map.cpp \
	src/singletons/legion_building_map.cpp \
	src/singletons/legion_package_pick_share_map.cpp \
	src/singletons/league_client.cpp \
	src/singletons/legion_task_box_map.cpp \
	src/data/common.cpp	\
	src/data/global.cpp	\
	src/data/castle.cpp	\
	src/data/item.cpp	\
	src/data/signing_in.cpp	\
	src/data/buff.cpp	\
	src/data/map.cpp	\
	src/data/map_defense.cpp	\
	src/data/map_object_type.cpp	\
	src/data/vip.cpp	\
	src/data/task.cpp	\
	src/data/map_event.cpp	\
	src/data/dungeon.cpp	\
	src/data/legion_corps_level.cpp  \
	src/data/legion_corps_power.cpp  \
	src/data/legion_package_corps_box.cpp  \
	src/data/legion_store_config.cpp  \
	src/data/legion_building_config.cpp  \
	src/data/league_power.cpp  \
	src/player/account.cpp	\
	src/player/map.cpp	\
	src/player/castle.cpp	\
	src/player/item.cpp	\
	src/player/mail.cpp	\
	src/player/chat.cpp	\
	src/player/auction.cpp	\
	src/player/announcement.cpp	\
	src/player/tax_record.cpp	\
	src/player/task.cpp	\
	src/player/battle_record.cpp	\
	src/player/dungeon.cpp	\
	src/player/friend.cpp	\
	src/player/legion.cpp	\
	src/player/legion_package.cpp \
	src/player/legion_building.cpp \
	src/player/league.cpp \
	src/admin/account.cpp	\
	src/admin/item.cpp	\
	src/admin/castle.cpp	\
	src/admin/castle_building_base.cpp	\
	src/admin/castle_tech.cpp	\
	src/admin/castle_resource.cpp	\
	src/admin/castle_soldier.cpp	\
	src/admin/announcement.cpp	\
	src/admin/payment.cpp	\
	src/admin/auction.cpp	\
	src/admin/dungeon.cpp	\
	src/admin/friend.cpp	\
	src/auction/query.cpp	\
	src/auction/transfer.cpp	\
	src/auction/create_bill.cpp	\
	src/auction/commit_bill.cpp	\
	src/account/promotion.cpp	\
	src/payment/create_bill.cpp	\
	src/payment/promotion_callback.cpp	\
	src/cluster/map.cpp	\
	src/controller/account.cpp	\
	src/controller/map.cpp	\
	src/controller/friend.cpp	\
	src/controller/chat.cpp	\
	src/event_listeners/account.cpp	\
	src/event_listeners/item.cpp	\
	src/event_listeners/castle.cpp	\
	src/event_listeners/auction.cpp	\
	src/event_listeners/map.cpp	\
	src/event_listeners/dungeon.cpp	\
	src/event_listeners/legion.cpp	\
	src/dungeon/dungeon.cpp	\
	src/league/league.cpp	\
	src/string_utilities.cpp	\
	src/map_utilities.cpp	\
	src/account_utilities.cpp	\
	src/map_utilities_center.cpp	\
	src/resource_utilities.cpp	\
	src/player_session.cpp	\
	src/cluster_session.cpp	\
	src/dungeon_session.cpp	\
	src/data_session.cpp	\
	src/admin_session.cpp	\
	src/auction_session.cpp	\
	src/account_session.cpp	\
	src/payment_session.cpp	\
	src/account.cpp	\
	src/map_cell.cpp	\
	src/map_object.cpp	\
	src/castle.cpp	\
	src/defense_building.cpp	\
	src/item_box.cpp	\
	src/mail_box.cpp	\
	src/mail_data.cpp	\
	src/chat_box.cpp	\
	src/chat_message.cpp	\
	src/horn_message.cpp	\
	src/announcement.cpp	\
	src/auction_center.cpp	\
	src/payment_transaction.cpp	\
	src/auction_transaction.cpp	\
	src/tax_record_box.cpp	\
	src/task_box.cpp	\
	src/strategic_resource.cpp	\
	src/map_event_block.cpp	\
	src/battle_record_box.cpp	\
	src/crate_record_box.cpp	\
	src/resource_crate.cpp	\
	src/dungeon_box.cpp	\
	src/dungeon.cpp	\
	src/dungeon_object.cpp	\
	src/friend_box.cpp	\
	src/legion.cpp	\
	src/legion_member.cpp	\
	src/legion_building.cpp	\
	src/warehouse_building.cpp	\
	src/legion_task_box.cpp \
	src/legion_task_reward_box.cpp \
	src/singletons/legion_task_reward_box_map.cpp \
	src/legion_task_contribution_box.cpp \
	src/singletons/legion_task_contribution_box_map.cpp \
	src/legion_log.cpp \
	src/mmain.cpp
