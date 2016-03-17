#!/bin/bash

mkdir -p ./log/

while [ 1 ]; do
	echo $(date +'[%Y-%m-%d %H:%M:%S']) Starting server...
	poseidon >>daemon.log 2>&1
	exit_code=$?
	echo $(date +'[%Y-%m-%d %H:%M:%S']) Exited, exit code was ${exit_code}.
	sleep 1
done
