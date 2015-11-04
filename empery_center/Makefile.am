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
	lib/libposeidon-empery-center.la

lib_libposeidon_empery_center_la_SOURCES = \
	src/singletons/player_websocket_server.cpp	\
	src/singletons/cluster_cbpp_server.cpp	\
	src/singletons/data_http_server.cpp	\
	src/singletons/admin_http_server.cpp	\
	src/singletons/account_map.cpp	\
	src/singletons/player_session_map.cpp	\
	src/singletons/cluster_session_map.cpp	\
	src/singletons/map_object_map.cpp	\
	src/singletons/castle_map.cpp	\
	src/data/castle.cpp	\
	src/player/account.cpp	\
	src/player/map.cpp	\
	src/player/castle.cpp	\
	src/cluster/map.cpp	\
	src/player_session.cpp	\
	src/cluster_session.cpp	\
	src/data_session.cpp	\
	src/admin_session.cpp	\
	src/map_object.cpp	\
	src/castle.cpp	\
	src/utilities.cpp	\
	src/mmain.cpp
