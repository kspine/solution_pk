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
	lib/libposeidon-empery-dungeon.la

lib_libposeidon_empery_dungeon_la_SOURCES = \
	src/singletons/dungeon_client.cpp	\
	src/singletons/dungeon_map.cpp \
	src/dungeon/dungeon.cpp \
	src/dungeon.cpp \
	src/dungeon_object.cpp \
	src/mmain.cpp
