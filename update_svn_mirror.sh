#!/bin/bash

prefix=../svn/infinity_of_empery_server

mkdir -p $prefix/

git submodule foreach 'git checkout master && git pull'

(find . | grep -P '^.*(\.((h|c)(pp)?|csv|conf|sh|crt|key)|configure\.ac|Makefile\.am|placeholder|README.*|LICENSE.*|\.gitignore)$' |	\
	xargs -i cp -v {} $prefix/ --parent)

echo ------------------------ Committing... ------------------------

(cd $prefix/ &&	\
	svn up &&	\
	svn add --force . &&	\
	svn ci -m "$(date +'Automated update from git on %Y-%m-%d %H:%M:%S')")
