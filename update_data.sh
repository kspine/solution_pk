#!/bin/bash

prefix="../../svn/paike_Numerical"
destination="./etc/poseidon/empery_center_data"

(cd "$prefix" && svn up)
find "$prefix/configs" -name '*.csv' | xargs -i sh -c 'echo {} && iconv -f UTF-8 -t UTF-8 {} -o '"$destination"'/$(basename {})'
sed -i 's/\r$//' $(find "$destination" -name '*.csv')
