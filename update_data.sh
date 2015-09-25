#!/bin/bash

prefix=../svn/infinity_of_empery_data

(cd $prefix && svn up)
find $prefix/ -name '*.csv' | xargs -i sh -c 'iconv -f GB18030 -t UTF-8 {} -o ./etc/poseidon/empery_data/$(basename {})'
