DIR=test2
/bin/rm -rf $DIR
FILE=/corp/drives/nps/nps-2009-casper-rw/ubnist1.casper-rw.gen3.raw
./bulk_extractor -r test_redlist.txt -w test_whitelist.txt -o $DIR $FILE
