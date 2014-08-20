#!/usr/bin/env python3
#
# NIST makes available at http://www.nsrl.nist.gov/morealgs.htm 
# A text file is available which relates the MD5 of a complete file to the MD5 of the first 4096 bytes in the file (provided the file is that large).
# A 426 MB Zip file can be downloaded which contains a 823 MB text file with 13,112,687 rows. 
# Each row has an MD5 hash of the entire file, a tab character, and an MD5 hash of the first 4k.
#
# This program scans the file and scans a bulk_extractor winpe.txt feature file
# to determine if the Windows executables are in NSRL

import sys

md5s = {}
for line in open("md5b4096.txt"):
    try:
        (full,first) = line[0:-1].split("\t")
        md5s[first] = full
    except ValueError:
        pass
    
for fn in sys.argv[1:]:
    print("reading {}".format(fn))
    for line in open(fn):
        if line[0]=='#': continue
        (offset,md5,xml) = line.split("\t")[0:3]
        md5 = md5.upper()
        try:
            print(offset,md5,md5s[md5],xml[0:50])
        except KeyError:
            print("NOT IN NSRL: ",line)
            pass
