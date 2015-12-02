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
	lib/libposeidon-empery-cluster.la

lib_libposeidon_empery_cluster_la_SOURCES = \
	src/singletons/world_map.cpp	\
	src/cluster/map.cpp	\
	src/abstract_data_object.cpp	\
	src/utilities.cpp	\
	src/cluster_client.cpp	\
	src/map_cell.cpp	\
	src/map_object.cpp	\
	src/mmain.cpp
