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
	lib/libposeidon-empery-controller.la

lib_libposeidon_empery_controller_la_SOURCES = \
	src/singletons/controller_cbpp_server.cpp	\
	src/singletons/world_http_server.cpp	\
	src/singletons/account_map.cpp	\
	src/singletons/world_map.cpp	\
	src/controller/account.cpp	\
	src/controller/map.cpp	\
	src/controller/friend.cpp	\
	src/world/query_castles.cpp	\
	src/controller_session.cpp	\
	src/world_session.cpp	\
	src/account.cpp	\
	src/castle.cpp	\
	src/string_utilities.cpp	\
	src/mmain.cpp
