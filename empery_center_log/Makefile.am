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
	lib/libposeidon-empery-center-log.la

lib_libposeidon_empery_center_log_la_SOURCES = \
	src/singletons/log_cbpp_server.cpp	\
	src/log/account.cpp	\
	src/log/item.cpp	\
	src/log/castle.cpp	\
	src/log/auction.cpp	\
	src/log/map.cpp	\
	src/log/legion.cpp	\
	src/log/novice_guide.cpp \
	src/log/dungeon.cpp	\
	src/log_session.cpp	\
	src/mmain.cpp
