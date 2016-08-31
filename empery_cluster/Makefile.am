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
	lib/libposeidon-empery-cluster.la

lib_libposeidon_empery_cluster_la_SOURCES = \
	src/singletons/world_map.cpp	\
	src/cluster/map.cpp	\
	src/data/common.cpp	\
	src/data/global.cpp	\
	src/data/map.cpp	\
	src/data/map_object.cpp \
	src/data/resource_crate.cpp \
	src/map_utilities.cpp	\
	src/cluster_client.cpp	\
	src/map_cell.cpp	\
	src/map_object.cpp	\
	src/resource_crate.cpp	\
	src/ai_control.cpp \
	src/mmain.cpp
