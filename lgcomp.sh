#!/bin/bash -ex

TDIR=lgtest
GITHASH=`git rev-parse --short HEAD`
ODIR=$TDIR/out

mkdir -p $TDIR

export LD_LIBRARY_PATH=/usr/local/lib

#OPTS="-j 1"

src/bulk_extractor -x accts_lg -x base16_lg -x email_lg -x gps_lg -x lightgrep -o $ODIR $OPTS $1
mv $ODIR $TDIR/${GITHASH}

src/bulk_extractor -x accts -x base16 -x email -x gps -o $ODIR $OPTS $1
mv $ODIR $TDIR/${GITHASH}_lg

CMPDIR=$TDIR/${GITHASH}_cmp
mkdir $CMPDIR
mkdir $CMPDIR/plus
mkdir $CMPDIR/minus

for i in `find $TDIR/$GITHASH -name '*.txt' ! -name '*_histogram.txt' ! -name 'url_services.txt' -exec basename \{\} \;`; do
  A=`mktemp`
  B=`mktemp`
  sort $TDIR/$GITHASH/$i >$A
  sort $TDIR/${GITHASH}_lg/$i >$B
  comm -1 -3 $A $B >$CMPDIR/plus/$i
  comm -2 -3 $A $B >$CMPDIR/minus/$i
  rm $A $B
  [ -s $CMPDIR/plus/$i ] || rm $CMPDIR/plus/$i
  [ -s $CMPDIR/minus/$i ] || rm $CMPDIR/minus/$i
done
