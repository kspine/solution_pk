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
	lib/libposeidon-empery-league.la

lib_libposeidon_empery_league_la_SOURCES = \
	src/singletons/league_map.cpp	\
	src/singletons/league_member_map.cpp	\
	src/singletons/league_cbpp_server.cpp	\
	src/singletons/league_applyjoin_map.cpp	\
	src/singletons/league_invitejoin_map.cpp	\
	src/league/league.cpp	\
	src/league_session.cpp	\
	src/string_utilities.cpp	\
	src/league.cpp	\
	src/league_member.cpp	\
	src/mmain.cpp
