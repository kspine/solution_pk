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
	lib/libposeidon-empery-center-log.la

lib_libposeidon_empery_center_log_la_SOURCES = \
	src/handlers/account_created.cpp	\
	src/handlers/account_logged_in.cpp	\
	src/handlers/account_logged_out.cpp	\
	src/handlers/item_changed.cpp	\
	src/handlers/resource_changed.cpp	\
	src/handlers/auction_item_changed.cpp	\
	src/mmain.cpp
