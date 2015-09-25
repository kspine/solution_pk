#!/bin/bash

bin='/usr/bin'
runpath=$(find `pwd` -type d -name '.libs' | tr '\n' \:)
etc=`pwd`'/etc'

if [ "$1" == "-d" ]; then
	LD_LIBRARY_PATH=$runpath ./poseidon/libtool --mode=execute gdb --args $bin/poseidon $etc/poseidon
elif [ "$1" == "-v" ]; then
	LD_LIBRARY_PATH=$runpath ./poseidon/libtool --mode=execute valgrind --leak-check=full --log-file='valgrind.log' $bin/poseidon $etc/poseidon
elif [ "$1" == "-vgdb" ]; then
	LD_LIBRARY_PATH=$runpath ./poseidon/libtool --mode=execute valgrind --vgdb=yes --vgdb-error=0 --leak-check=full --log-file='valgrind.log' $bin/poseidon $etc/poseidon
else
	LD_LIBRARY_PATH=$runpath $bin/poseidon $etc/poseidon
fi
