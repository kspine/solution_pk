#!/bin/bash

prefix=../svn/paike_Numerical/configs/csvs
destination=./etc/poseidon/empery_center_data

(cd "$prefix" && svn up)
find "$prefix" -name '*.csv' | xargs -i sh -c 'echo {} && iconv -f GB18030 -t UTF-8 {} -o '"$destination"'/$(basename {})'
sed -i 's/\r$//' $(find "$destination" -name '*.csv')
