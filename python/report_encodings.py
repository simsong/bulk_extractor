#!/usr/bin/env python3.2
#
# Report on how things are encoded in one or more bulk_extractor reports
#


import sys,os
#sys.path.append(os.getenv("DOMEX_HOME") + "/src/lib/") # 
#sys.path.append("../lib/")                             # 
#sys.path.append(os.getenv("HOME") + "/lib")            # 
#sys.path.append(os.getenv("HOME") + "/gits/bulk_extractor/python") # for bulk_extractor_reader

from collections import Counter,defaultdict
from statbag import statbag
import bulk_extractor_reader
import ttable
from ttable import tvar
import glob

ignored_features = ['bulk_tags.txt','wordlist.txt']


# configuration
MINRECOVERABLEFILES = 100
import re

PF  = "1) Plain in File"
EF  = "2) Encoded in File"
PNF = "3) Plain Not in File"
ENF = "4) Encoded Not in File"

space="\\hspace{2pc}"

def drive_name(path):
    if path[-1]=='/':
        path = path[0:-1]
    return path.split("/")[-1]    

def process_line(line):
    fields = line.split(b'\t')
    if len(fields)<3:
        return (None,None,None,None)
    nofilename = len(fields)<5
    # Remove digits from the path
    path = fields[0].decode('utf-8')
    encoding    = "".join(filter(lambda s:not s.isdigit(),path))
    encoding = encoding.replace("BASE","BASE64") # put back BASE64
    encoding = encoding.replace("--","-")        # 
    if encoding[0:1]=='-': encoding=encoding[1:]
    if encoding[-1:]=='-': encoding=encoding[:-1]
    feature = bulk_extractor_reader.decode_feature(fields[1])
    return (path,encoding,feature,nofilename)

def get_line_context(line):
    return line.split(b'\t')[2]

class Drive:
    """Reads a bulk_extractor report for a drive.  Determine the total number of encodings used for each feature file type.:
    """

    def __init__(self,fn):
        """fn is the filename of the report directory or zip file"""
        self.fn = fn                       # filename of this drive
        self.f_encoding_counts = defaultdict(Counter) # for each feature type, track the encodings
        self.uderror = 0        # unicode decode
        
    def process(self):
        ber = bulk_extractor_reader.BulkReport(self.fn,do_validate=False)
        for ff in ber.feature_files():
            if ff in ignored_features: continue
            print("Processing {} in {}".format(ff,self.fn))
            self.process_feature_file(ber,ff)

    def process_feature_file(self,ber,ff):
        for line in ber.open(ff,'rb'):
            # Remove lines that aren't features
            if line[0]<ord('0') or line[0]>ord('9'):
                continue
            try:
                (path,encoding,feature,nofilename) = process_line(line)
                self.f_encoding_counts[ff][encoding] += 1
            except UnicodeDecodeError:
                self.uderror += 1
                pass
            except UnicodeEncodeError:
                self.uderror += 1
                pass
            except SystemError:
                self.uderror += 1
                pass



if __name__=="__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Experiment")
    parser.add_argument("bereports",help="bulk_extractor reports to process",type=str,nargs="+")
    parser.add_argument("--verbose",help="Print all email addresses",action='store_true')
    parser.add_argument("--latex",help="output file for LaTeX",type=str)
    args = parser.parse_args()
    
    if not args.bereports:
        parser.print_help()

    l = None
    if args.latex:
        l = open(args.latex,"wt")
        tvar.out = l
        tvar("MINRECOVERABLEFILES",MINRECOVERABLEFILES,"Minimum Recoverable Files")
        tvar("YEARSTART",2005,"First Year") # until I can get a better value
        tvar("YEAREND",2013,"Last Year")    # until I can get a better value

    # Perform glob expansion for Windows
    files = []
    for fn in args.bereports:
        if "*" in fn:
            files += glob.glob(fn)
        else:
            files += [fn]

    drive_encoding_counts = {}
    for fn in files:
        print("")
        d = Drive(fn)
        d.process()
        for ff in d.f_encoding_counts:
            if ff not in drive_encoding_counts: drive_encoding_counts[ff] = defaultdict(statbag)
            for encoding in d.f_encoding_counts[ff]:
                drive_encoding_counts[ff][encoding].addx(d.f_encoding_counts[ff][encoding])
                
    # Now that the data have been collected, typeset the big table
    t = ttable.ttable()
    t.latex_colspec = "lrrrrr"
    t.append_head(('',               'Drives with','Feature', 'avg',      'max',      ''))
    t.append_head(('Feature / Coding','coding' ,'Count','per drive','per drive','$\\sigma$'))
    t.set_col_alignment(1,t.LEFT)
    t.set_col_alignment(2,t.RIGHT)
    t.set_col_alignment(3,t.RIGHT)
    t.set_col_alignment(4,t.RIGHT)
    t.set_col_alignment(5,t.RIGHT)
    
    rep = []                    # report will be sorted by second column

    print("\n"*4)
    for ff in sorted(drive_encoding_counts.keys()):
        for enc in sorted(drive_encoding_counts[ff].keys()):
            k = ff + " / " + str(enc)
            sb = drive_encoding_counts[ff][enc]
            row = (k,sb.count(),sb.sumx(),sb.average(),sb.maxx(),sb.stddev())
            t.append_data(row)
        t.append_data(())
        
    print(t.typeset(mode='text'))
    if l:
        s = t.typeset(mode='latex')
        l.write("\\newcommand{\\studyStats}{"+s+"}\n")



