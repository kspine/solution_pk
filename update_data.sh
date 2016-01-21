#!/bin/bash

set -e

if [ -z "$1" ];
then
	echo Please specify a version.
	exit 1
fi

prefix="../svn/paike_Numerical/$1"
destination="./etc/poseidon/empery_center_data"

(cd "$prefix" && svn up)
find "$prefix" -name '*.csv' | xargs -i sh -c 'echo {} && iconv -f GB18030 -t UTF-8 {} -o '"$destination"'/$(basename {})'
sed -i 's/\r$//' $(find "$destination" -name '*.csv')
