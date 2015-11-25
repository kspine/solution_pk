#!/bin/bash

tar -czvf empery_server.tar.gz `find .	\
	-name 'placeholder' -or	-name 'npc_server' -or \
	-name '*.h' -or -name '*.c' -or -name '*.hpp' -or -name '*.cpp' -or -name '*.hh' -or -name '*.cc' -or	\
	-name '*.csv' -or -name '*.xml' -or -name '*.conf' -or	\
	-name 'configure.ac' -or -name 'Makefile.am' -or	\
	-name '*.sh' -or -name '*.crt' -or -name '*.key'`
