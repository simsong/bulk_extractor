#!/usr/bin/env python3
# coding=UTF-8
"""
Revised BE2.0 regression system:

Runs two versions of bulk_extractor and reports the differences, taking into account the need to sort feature files.

The analysis consists of:
1 - Sort the feature files if a ".sorted" file is not present.
2 - Create the ".sorted" file.
3 - Run "diff -r" on the two reports.
4 - Report the XML differences.

"""

from subprocess import Popen,call
import os
import glob
import xml.etree.ElementTree as ET
try:
    from xml.etree.cElementTree import XML
except ImportError:
    from xml.etree.ElementTree import XML

__version__ = "2.0.0"

def validate_report(report):
    if not os.path.isdir(report):
        raise FileNotFoundError(f"{report} is not a directory")
    xmlfile = os.path.join(report,"report.xml")
    if not os.path.isfile(xmlfile):
        raise FileNotFoundError(xmlfile)
    tree = XML(open(xmlfile,"r").read())

def sort_report(report):
    sorted_file = os.path.join(report,".sorted")
    if os.path.exists(sorted_file):
        return
    for fn in glob.glob(os.path.join(report,"*.txt")):
        lines = open(fn,"r").read().split("\n")
        open(fn,"w").write("\n".join(sorted(lines)))
    open(sorted_file,"w").close()

def generate_report(report1,report2):
    call(['diff','--recursive','--side-by-side',report1,report2])

if __name__=="__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Perform regression testing on bulk_extractor",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("report1",help="The first report to analyze")
    parser.add_argument("report2",help="The second report to analyze")
    args = parser.parse_args()
    for report in [args.report1, args.report2]:
        validate_report(report)
        sort_report(report)
    generate_report(args.report1, args.report2)
