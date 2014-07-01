#!/usr/bin/env python3
# coding=UTF-8
# perform a diff of two bulk_extractor output directories.

# for you and I, the more relevant would be the times of the b_e
# reports, and ideally if they were e01 files the times the image was
# created. But the legal folks would probably like to know when the
# report was generated as well

b'This module needs Python 2.7 or later.'

__version__='1.3.0'
import os.path,sys

if sys.version_info < (2,7):
    raise "Requires Python 2.7 or above"

import ttable, bulk_extractor_reader

html_header = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
bulk_diff_version = '1.3'

all_emails = set()

def process(report):
    b1 = bulk_extractor_reader.BulkReport(report,do_validate=False)
    try:
        h = b1.read_histogram("email_histogram.txt")
    except KeyError:
        return
    for (a) in h:
        all_emails.add(a)
    print("Processed {}; now {} unique emails".format(report,len(all_emails)))
            

if __name__=="__main__":
    import argparse 
    global args
    import sys,time,zlib

    parser = argparse.ArgumentParser(description="Create a stop list from bulk_extractor reports",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("reports",nargs="+")
    args = parser.parse_args()

    for fn in args.reports:
        try:
            process(fn)
        except zlib.error:
            print("{} appears corrupt".format(fn))

