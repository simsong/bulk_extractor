#!/bin/bash -ex

TDIR=lgtest
GITHASH=`git rev-parse --short HEAD`
ODIR=$TDIR/out

mkdir -p $TDIR

export LD_LIBRARY_PATH=/usr/local/lib

src/bulk_extractor -x accts_lg -x base16_lg -x email_lg -x gps_lg -x lightgrep -o $ODIR $1
mv $ODIR $TDIR/${GITHASH}

src/bulk_extractor -x accts -x base16 -x email -x gps -o $ODIR $1
mv $ODIR $TDIR/${GITHASH}_lg

CMPDIR=$TDIR/${GITHASH}_cmp
mkdir $CMPDIR 

for i in `find $TDIR/$GITHASH -name '*.txt' ! -name '*_histogram.txt' ! -name 'url_services.txt' | xargs basename -a`; do
  A=`mktemp`
  B=`mktemp` 
  sort $TDIR/$GITHASH/$i >$A
  sort $TDIR/${GITHASH}_lg/$i >$B
  comm -3 $A $B >$CMPDIR/$i
  rm $A $B
  [ -s $CMPDIR/$i ] || rm $CMPDIR/$i
done
