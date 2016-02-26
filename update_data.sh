#!/bin/bash

set -e

prefix="../svn/paike_Numerical"
destination="./etc/poseidon/empery_center_data"

(cd "$prefix" && svn up)
find "$prefix/configs_1.0" -name '*.csv' | xargs -i sh -c 'echo {} && iconv -f GB18030 -t UTF-8 {} -o '"$destination"'/$(basename {})'
sed -i 's/^M$//' $(find "$destination" -name '*.csv')
