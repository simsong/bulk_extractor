#!/usr/bin/env python3
# coding=UTF-8
#
# Cross Drive Analysis tool for bulk extractor.

__version__='2.0.0'
import os.path,sys

#if sys.version_info < (3,2):
#    raise RuntimeError("Requires Python 3.2 or above")

import os,sys,re,collections,sqlite3

# add paths in an attempt to find our modules
if os.getenv("DOMEX_HOME"):
    sys.path.append(os.getenv("DOMEX_HOME") + "/src/lib/") # add the library
sys.path.append("../lib/")      # add the library


# Replace this with an ORM?
schema = \
"""
PRAGMA cache_size = 200000;
CREATE TABLE IF NOT EXISTS drives (driveid INTEGER PRIMARY KEY,drivename TEXT NOT NULL UNIQUE,report_fn TEXT,ingested DATE);
CREATE INDEX IF NOT EXISTS drives_idx1 ON drives(drivename);
CREATE INDEX IF NOT EXISTS drives_idx2 ON drives(report_fn);

CREATE TABLE IF NOT EXISTS features (featureid INTEGER PRIMARY KEY,feature TEXT NOT NULL UNIQUE);
CREATE INDEX IF NOT EXISTS features_idx ON features (feature);

CREATE TABLE IF NOT EXISTS feature_drive_counts (driveid INTEGER NOT NULL,feature_type INTEGER NOT NULL,featureid INTEGER NOT NULL,count INTEGER NOT NULL) ;
CREATE INDEX IF NOT EXISTS feature_drive_counts_idx1 ON feature_drive_counts(featureid);
CREATE INDEX IF NOT EXISTS feature_drive_counts_idx2 ON feature_drive_counts(count);

CREATE TABLE IF NOT EXISTS feature_frequencies (id INTEGER PRIMARY KEY,feature_type INTEGER NOT NULL,featureid INTEGER NOT NULL,drivecount INTEGER,featurecount INTEGER);
CREATE INDEX IF NOT EXISTS feature_frequencies_idx ON feature_frequencies (featureid);

CREATE TABLE IF NOT EXISTS drive_correlations (driveid1 INTEGER NOT NULL,driveid2 INTEGER NOT NULL,corr REAL);
CREATE INDEX IF NOT EXISTS drive_correlations_idx1 ON drive_correlations (driveid1);
CREATE INDEX IF NOT EXISTS drive_correlations_idx2 ON drive_correlations (driveid2);
CREATE INDEX IF NOT EXISTS drive_correlations_idx3 ON drive_correlations (corr);

"""

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

def get_driveid(drivename,report_fn=None,create=True):
    c = conn.cursor()
    if create:
        c.execute("INSERT INTO drives (driveid,drivename,report_fn) VALUES (NULL,?,?)",(drivename,report_fn))
    c.execute("SELECT driveid from drives where drivename=?",(drivename,))
    r = c.fetchone()
    if r: return r[0]
    return None

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

def ingest(report_fn):
    import time
    c = conn.cursor()
    c.execute("select count(*) from drives where report_fn=?",(report_fn,))
    if c.fetchone()[0]>0 and args.reimport==False:
        print("{} already imported".format(report_fn))
        return

    try:
        br = bulk_extractor_reader.BulkReport(report_fn)
        image_filename = br.image_filename()
    except IndexError:
        print("No image filename in bulk_extractor report for {}; will not ingest".format(report_fn))
        return
    except OSError:
        print("Cannot open {}; will not ingest".format(report_fn))
        return
    except KeyError:
        print("Cannot open {}; will not ingest".format(report_fn))
        return

    if args.reimport==False:
        driveid = get_driveid(image_filename,create=False)
        if driveid:
            print("{} already imported".format(image_filename))
            return

    driveid = get_driveid(image_filename,report_fn,create=True)
    print("Ingesting {} as driveid {}".format(br.image_filename(),driveid))
    t0 = time.time()
    
    if args.reimport:
        # Make sure that this driveid is not in the feature tables
        c.execute("DELETE FROM feature_drive_counts where driveid=?",(driveid,))

    # initial version we are ingesting search terms, winpe executables, and email addresses
    for (search,count) in br.read_histogram_entries("url_searches.txt"):
        if search.startswith(b"cache:"): continue  
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
    for r in br.read_features("winpe.txt"):
        try:
            (pos,feature,context) = r
            featureid = get_featureid(feature)
            pe_header_counts[featureid] += 1
        except ValueError:
            print("got {} values".format(len(r)))
    for (featureid,count) in pe_header_counts.items():
        c.execute("INSERT INTO feature_drive_counts (driveid,feature_type,featureid,count) values (?,?,?,?);",
                  (driveid,WINPE_TYPE,featureid,count))
    conn.commit()
    t1 = time.time()
    print("Driveid {} imported in {} seconds\n".format(driveid,t1-t0))


def correlate_for_type(driveid,feature_type,verbose=True,larger=False):
    """Performs the correlation for a specific drive, returns a dictionary of the drives that the
    drive correlates with and the correlation coefficients."""

    def feature_fmt(feature):
        import urllib.parse
        if type(feature)==bytes:
            feature = feature.decode('utf8',errors='ignore')
        if (feature_type==SEARCH_TYPE) and ("%" in feature):
            try:
                feature = "{} ({})".format(feature,urllib.parse.unquote(feature,errors='strict'))
            except UnicodeDecodeError:
                pass            # not valid UTF-8, so don't decode it
        return feature

    c = conn.cursor()
    res = []
    c.execute("SELECT R.drivecount,C.featureid,feature FROM feature_drive_counts as C JOIN features as F ON C.featureid = F.rowid "+
              "JOIN feature_frequencies as R ON C.featureid = R.featureid where C.driveid=? and C.feature_type=?",
              (driveid,feature_type))
    res = sorted(c.fetchall())
    # Strangely, when we add an ' order by R.drivecount where R.drivecount>1' above it kills performance
    # So we just do those two operations manually
    # res = filter(lambda r:r[0]>1,sorted(res))
    if verbose:
        print("Unique terms for this drive:")
        for (drivecount,featureid,feature) in filter(lambda a:a[0]==1,res):
            print("  {}".format(feature_fmt(feature)))
        print("")

    # Calculate the correlation coefficient
    coefs = {}                  # the coefficients, by driveid
    contribs = {}               # the contributing drives, by driveid
    for line in res:
        if args.debug: print(line)
        (drivecount,featureid,feature) = line
        if drivecount==1: pass
        if larger:
            # Only correlate with larger DriveIDs
            cmd = "select driveid from feature_drive_counts where featureid=? and driveid>?"
        else:
            cmd = "select driveid from feature_drive_counts where featureid=? and driveid!=?"
        arg = (featureid,driveid)
        for (driveid_,) in c.execute(cmd, arg):
            if args.debug: print("  also on drive {}".format(driveid_))
            if driveid_ not in coefs: coefs[driveid_] = 0; contribs[driveid_] = []
            coefs[driveid_] += 1.0/drivecount
            contribs[driveid_].append([1.0/drivecount,featureid,feature])
        if drivecount > args.drive_threshold:
            break

    if verbose:
        for (driveid_,coef) in sorted(coefs.items(),key=lambda a:a[1],reverse=True):
            print("Drive {} {}".format(driveid_,get_drivename(driveid_)))
            print("Correlation: {:.6}".format(coef))
            for (weight,featureid,feature) in sorted(contribs[driveid_],reverse=True):
                print("   {:.2}   {}".format(weight,feature_fmt(feature)))
            print("")
    return coefs

def make_report(driveid,verbose=True):
    c = conn.cursor()
    c.execute("select count(*) from feature_frequencies")
    if c.fetchone()[0]==0:
        build_feature_frequencies()
    if not (args.email or args.search or args.winpe):
        print("Specify --email for correlating on email addresses")
        print("Specify --search for correlating on search terms addresses")
        print("Specify --winpe for correlating on Windows executable headers")
        exit(1)
    print("Report for drive: {} {}".format(driveid,get_drivename(driveid)))
    if args.email:
        print("Email correlation report:")
        correlate_for_type(driveid,EMAIL_TYPE,verbose=verbose)
    if args.search:
        print("Search correlation report:")
        correlate_for_type(driveid,SEARCH_TYPE,verbose=verbose)
    if args.winpe:
        print("WINPE correlation report:")
        correlate_for_type(driveid,WINPE_TYPE,verbose=verbose)
    
def test():
    a = get_featureid("A")
    b = get_featureid("B")
    assert(a!=b)
    assert(get_featureid("A")==a)
    assert(get_featureid("B")==b)
    conn.commit()

def build_feature_frequencies():
    print("Building feature frequencies...")
    c = conn.cursor()
    c.execute("delete from feature_frequencies")
    c.execute("insert into feature_frequencies (featureid,feature_type,drivecount,featurecount) select featureid,feature_type,count(*),sum(count) from feature_drive_counts group by featureid,feature_type")
    conn.commit()
    print("Feature frequences built.")
    

if(__name__=="__main__"):
    import argparse,xml.parsers.expat
    parser = argparse.ArgumentParser(description='Cross Drive Analysis with bulk_extractor output')
    parser.add_argument('reports', type=str, nargs='*', help='bulk_extractor report directories or ZIP files')
    parser.add_argument("--debug",action="store_true")
    parser.add_argument("--ingest",help="Ingest a new BE report",action="store_true")
    parser.add_argument("--list",help="List the reports in the database",action='store_true')
    parser.add_argument("--recalc",help="Recalculate all of the feature counts in database",action='store_true')
    parser.add_argument("--test",help="Test the script",action="store_true")
    parser.add_argument("--report",help="Generate a report for a specific driveid",type=int)
    parser.add_argument("--build",help="build feature_frequences",action='store_true')
    parser.add_argument("--drive_threshold",type=int,help="don't show features on more than this number of drives",default=10)
    parser.add_argument("--correlation_cutoff",type=float,help="don't show correlation drives for coefficient less than this",default=0.5)
    parser.add_argument("--reimport",help="reimport drives, rather than ignoring drives already specified",action='store_true')
    parser.add_argument("--all",help="calculate the correlation coefficient for all",action='store_true')
    parser.add_argument("--maxdriveid",help="Limit correlation to a small area")
    parser.add_argument("--email",help="Correlate on email",action='store_true')
    parser.add_argument("--search",help="Correlate on search terms",action='store_true')
    parser.add_argument("--winpe",help="Correlate on windows executables",action='store_true')
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
        # If running on windows, check for * and glob if necessary
        if "*" in args.reports[0]: 
            import glob
            args.reports = glob.glob(args.reports[0])
        for fn in args.reports:
            print(fn)
            ingest(fn)

    if args.build:
        build_feature_frequencies()

    if args.list:
        list_drives()

    if args.report:
        make_report(args.report)
    
    if args.all:
        create_schema()
        c = conn.cursor()
        c.execute("select max(driveid1) from drive_correlations")
        start = 1
        r = c.fetchone()
        if r: start=r[0]
        for drive1 in range(start,2000):
            corr = correlate_for_type(drive1,SEARCH_TYPE,verbose=False,larger=True)
            print(corr)
            c.execute("delete from drive_correlations where (driveid1=?)",(drive1,))
            for (drive2,corr) in corr.items():
                c.execute("insert into drive_correlations (driveid1,driveid2,corr) values (?,?,?)",(drive1,drive2,corr))
                print(drive2,corr)
            conn.commit()
