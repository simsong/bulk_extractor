#!/bin/sh

if [ -z "$*" ]; then
  echo "Usage: $0 [[raw_avc_file] ...]";
fi

/bin/mkdir -p out

for f in $*; do
  echo $f
  name=`basename $f`
  ./test/test_parse_avcc.t $f > out/"$name".log 2>&1
done

