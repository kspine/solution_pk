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
	lib/libposeidon-texas-gate-westwalk.la

lib_libposeidon_texas_gate_westwalk_la_SOURCES = \
	src/singletons/account_http_server.cpp	\
	src/singletons/account_map.cpp	\
	src/account/check.cpp	\
	src/account/create.cpp	\
	src/account/login.cpp	\
	src/account/renewal.cpp	\
	src/account/regain.cpp	\
	src/account_http_session.cpp	\
	src/sms_http_client.cpp	\
	src/mmain.cpp
