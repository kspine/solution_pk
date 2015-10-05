#!/bin/bash

prefix=../svn/paike_data

(cd $prefix && svn up)
find $prefix/ -name '*.csv' | xargs -i sh -c 'iconv -f GB18030 -t UTF-8 {} -o ./etc/poseidon/empery_data/$(basename {})'
