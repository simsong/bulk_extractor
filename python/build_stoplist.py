#!/usr/bin/env python3
# coding=UTF-8
# perform a diff of two bulk_extractor output directories.

b'This module needs Python 2.7 or later.'

__version__='1.3.0'
import os.path,sys

if sys.version_info < (2,7):
    raise "Requires Python 2.7 or above"

import ttable, bulk_extractor_reader

build_stoplist_version = '1.3'

all_emails = set()

def process(report,fsc):
    b1 = bulk_extractor_reader.BulkReport(report,do_validate=False)
    print("Reading email.txt")
    try:
        for line in b1.open("email.txt"):
            fsc.write(line)
    except KeyError:
        pass

    try:
        h = b1.read_histogram("email_histogram.txt")
        for (a) in h:
            all_emails.add(a)
    except KeyError:
        pass
    print("Processed {}; now {} unique emails".format(report,len(all_emails)))

            

if __name__=="__main__":
    import argparse 
    global args
    import sys,time,zlib,zipfile

    parser = argparse.ArgumentParser(description="Create a stop list from bulk_extractor reports",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--stoplist",default="stop-list.txt")
    parser.add_argument("--stopcontext",default="stop-context.txt")
    parser.add_argument("reports",nargs="+",help="BE reports or ZIPfiles with email.txt files to ignore")
    args = parser.parse_args()

    fsc = open(args.stopcontext,"wb")

    for fn in args.reports:
        try:
            process(fn,fsc)
        except zlib.error:
            print("{} appears corrupt".format(fn))
        except zipfile.BadZipFile:
            print("{} has a bad zip file".format(fn))

    with open(args.stoplist,"wb") as f:
        f.write(b"\n".join(sorted(all_emails)))
