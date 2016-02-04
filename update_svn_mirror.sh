#!/bin/bash

prefix=../svn/paike_server/master

mkdir -p $prefix/

(find . -name '*.h' -or -name '*.hpp' -or -name '*.c' -or -name '*.cpp' -or -name '*.csv' -or -name '*.conf' -or -name '*.sh'	\
	-or -name '*.crt' -or -name '*.key' -or -name 'configure.ac' -or -name 'Makefile.am' -or -name 'placeholder' -or -name 'README*'	\
	-or -name 'LICENSE*' -or -name '.gitignore'	| \
	xargs -i cp -v {} $prefix/ --parent)

echo ------------------------ Committing... ------------------------

(cd $prefix/ &&	\
	svn add --force . &&	\
	svn ci -m "$(date +'Automated update from git on %Y-%m-%d %H:%M:%S')")
