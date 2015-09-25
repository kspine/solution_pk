#!/bin/bash

mkdir -p ./log/

while [ 1 ]; do
	prefix="./log/daemon_$(date +'%Y-%m-%d_%H-%M-%S')"
	echo Starting server: log prefix = $prefix
	./run_server.sh >"$prefix.out" 2>"$prefix.err"
	echo Exited, error code was $?
	sleep 5
done
