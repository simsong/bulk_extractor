#!/bin/sh
#
echo make_bench.sh will make the benchmark files 
echo This should always be done on the same machine.
echo All options should be enabled '(-e all)'
for drive in nps-2009-domexusers nps-2009-ubnist1.gen3 nps-2010-emails nps-2009-casper-rw ; do
  #/bin/rm -rf $drive
  fn=/corp/nps/drives/`basename $drive .gen3`/$drive.E01
  if [ ! -r $fn ] ; then echo $fn not found ; exit 1 ; fi
  src/bulk_extractor -e all -o $drive $fn
done
