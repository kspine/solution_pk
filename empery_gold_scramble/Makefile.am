ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign subdir-objects
AM_CPPFLAGS = -Wall -Wextra -Werror -Wsign-conversion -Wno-error=unused-parameter -Wno-missing-field-initializers -pipe -fPIC -DPIC
AM_CXXFLAGS =

%.hpp.gch: %.hpp
	$(CXX) -x c++-header $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) $< -o $@

BUILT_SOURCES = \
	src/precompiled.hpp.gch

CLEANFILES = \
	src/precompiled.hpp.gch

lib_LTLIBRARIES = \
	lib/libposeidon-empery-gold-scramble.la

lib_libposeidon_empery_gold_scramble_la_SOURCES = \
	src/singletons/player_session_map.cpp	\
	src/singletons/player_websocket_server.cpp	\
	src/singletons/http_client_daemon.cpp	\
	src/singletons/global_status.cpp	\
	src/singletons/bid_record_map.cpp	\
	src/singletons/game_record_map.cpp	\
	src/player/account.cpp	\
	src/player_session.cpp	\
	src/utilities.cpp	\
	src/mmain.cpp
