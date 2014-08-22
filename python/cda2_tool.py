#!/usr/bin/env python3
# coding=UTF-8
#
# Cross Drive Analysis tool for bulk extractor.
# V4
# Features of this program:
# --netmap  -- makes a map of which computers exchanged packets (ethernet maps)
# --makestop  -- Creates a stoplist of features that are on more than a fraction of the drives
# --threshold -- sets the fraction of drives necessary for a feature to be ignored
# --idfeatures  -- spcifies which feature files are used for identity operations
#
# reads multiple bulk_extractor histogram files and outputs:
# stoplist.txt - list of email addresses on more than 1/3 of the disks
# targets.txt  - list of email addresses not on stoplist and the # of drives on which they appear.
#
# Version 1.3 - Complete rewrite; elimiantes driveids and featureids, since strings
#               in Python are hashable (and thus integers). Also uses bulk_extractor_reader

b'This module needs Python 3.2 or above.'

__version__='1.3.1'
import os.path,sys

if sys.version_info < (3,2):
    raise RuntimeError("Requires Python 3.2 or above")

import os,sys,re,collections,sqlite3

# add paths in an attempt to find our modules
if os.getenv("DOMEX_HOME"):
    sys.path.append(os.getenv("DOMEX_HOME") + "/src/lib/") # add the library
sys.path.append("../lib/")      # add the library

# Replace this with an ORM?
schema = \
"""CREATE TABLE IF NOT EXISTS drives (driveid INTEGER PRIMARY KEY,drivename TEXT NOT NULL UNIQUE,ingested DATE);
CREATE TABLE IF NOT EXISTS features (featureid INTEGER PRIMARY KEY,feature TEXT NOT NULL UNIQUE);
CREATE TABLE IF NOT EXISTS feature_drive_counts (driveid INTEGER NOT NULL,feature_type INTEGER NOT NULL,featureid INTEGER NOT NULL,count INTEGER NOT NULL) ;
CREATE TABLE IF NOT EXISTS feature_frequencies (id INTEGER PRIMARY KEY,feature_type INTEGER NOT NULL,featureid INTEGER NOT NULL,drivecount INTEGER,featurecount INTEGER);"""

"""Explaination of tables:
drives         - list of drives that have been ingested.
features       - table of all features
feature_drive_counts - count of all features per drive
feature_frequencies    - for each feature type, a count of the number of drives on which it appears, and the total number of times in the collection
"""


import ttable, bulk_extractor_reader

SEARCH_TYPE = 1
EMAIL_TYPE = 2
WINPE_TYPE = 3

def create_schema():
    # If the schema doesn't exist, create it
    c = conn.cursor()
    for line in schema.split(";"):
        print(line)
        c.execute(line)

def get_driveid(drivename):
    c = conn.cursor()
    c.execute("INSERT INTO drives (driveid,drivename) VALUES (NULL,?)",(drivename,))
    return c.lastrowid

def get_drivename(driveid):
    c = conn.cursor()
    c.execute("SELECT drivename from drives where driveid=?",(driveid,))
    return c.fetchone()[0]

def get_featureid(feature):
    c = conn.cursor()
    c.execute("INSERT OR IGNORE INTO features (featureid,feature) VALUES (NULL,?)",(feature,))
    c.execute("SELECT featureid from features where feature=?",(feature,))
    return c.fetchone()[0]

def list_drives():
    c = conn.cursor()
    for (rowid,drivename) in c.execute("SELECT rowid,drivename FROM drives"):
        print("{:3} {}".format(rowid,drivename))

def feature_drive_count(featureid):
    assert(type(featureid)==int)
    c = conn.cursor()
    c.execute("SELECT count(*) from feature_drive_counts where featureid=? ",(featureid,))
    return c.fetchone()[0]


def ingest(report):
    c = conn.cursor()
    br = bulk_extractor_reader.BulkReport(report)
    driveid = get_driveid(br.image_filename())
    print("Ingesting {} as driveid {}".format(br.image_filename(),driveid))

    # initial version we are ingesting search terms, winpe executables, and email addresses
    # make sure that our table exists
    for (search,count) in br.read_histogram_entries("url_searches.txt"):
        if search.startswith(b"cache:"): continue  
        #print("Add search {}".format(search))
        featureid = get_featureid(search);
        c.execute("INSERT INTO feature_drive_counts (driveid,feature_type,featureid,count) values (?,?,?,?);",
                  (driveid,SEARCH_TYPE,featureid,count))
        
    # Add counts for email addresses
    for (email,count) in br.read_histogram_entries("email_histogram.txt"):
        #print("Add email {} = {}".format(email,count))
        featureid = get_featureid(email);
        c.execute("INSERT INTO feature_drive_counts (driveid,feature_type,featureid,count) values (?,?,?,?);",
                  (driveid,EMAIL_TYPE,featureid,count))

    # Add hashes for Windows executables
    import collections
    pe_header_counts = collections.Counter()
    for (pos,feature,context) in br.read_features("winpe.txt"):
        featureid = get_featureid(feature)
        pe_header_counts[featureid] += 1
    for (featureid,count) in pe_header_counts.items():
        c.execute("INSERT INTO feature_drive_counts (driveid,feature_type,featureid,count) values (?,?,?,?);",
                  (driveid,WINPE_TYPE,featureid,count))
    conn.commit()


def make_report(driveid):
    print("Report for drive: {} {}".format(driveid,get_drivename(driveid)))
    print("Email address   count      count all drives")
    c = conn.cursor()
    for (featureid,feature,count) in c.execute("SELECT C.featureid,feature,count FROM feature_drive_counts as C JOIN features as F ON C.featureid = F.rowid where C.driveid=? and C.feature_type=? ",(driveid,EMAIL_TYPE)):
        print(feature,count,feature_drive_count(featureid))

def test():
    a = get_featureid("A")
    b = get_featureid("B")
    assert(a!=b)
    assert(get_featureid("A")==a)
    assert(get_featureid("B")==b)
    conn.commit()


if(__name__=="__main__"):
    import argparse,xml.parsers.expat

    parser = argparse.ArgumentParser(description='Cross Drive Analysis with bulk_extractor output')
    parser.add_argument("--ingest",help="Ingest a new BE report",action="store_true")
    parser.add_argument("--list",help="List the reports in the database",action='store_true')
    parser.add_argument("--recalc",help="Recalculate all of the feature counts in database",action='store_true')
    parser.add_argument("--test",help="Test the script",action="store_true")
    parser.add_argument("--report",help="Generate a report for a specific driveid",type=int)
    parser.add_argument('reports', type=str, nargs='*', help='bulk_extractor report directories or ZIP files')
    args = parser.parse_args()

    if args.test:
        try:
            os.unlink('cda2_tool.test.db')
        except FileNotFoundError:
            pass
        conn = sqlite3.connect('cda2_tool.test.db')
        create_schema()
        test()

    conn = sqlite3.connect('cda2_tool.db')

    if args.ingest:
        create_schema()
        for fn in args.reports:
            ingest(fn)


    if args.list:
        list_drives()

    if args.report:
        make_report(args.report)
