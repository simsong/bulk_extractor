#!/usr/bin/env python3
# coding=UTF-8
# perform a diff of two bulk_extractor output directories.

# for you and I, the more relevant would be the times of the b_e
# reports, and ideally if they were e01 files the times the image was
# created. But the legal folks would probably like to know when the
# report was generated as well

b'This module needs Python 2.7 or later.'

__version__='1.5.0'
import os.path,sys

if sys.version_info < (2,7):
    raise "Requires Python 2.7 or above"

import ttable, bulk_extractor_reader

html_header = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
bulk_diff_version = '1.3'

def process(out,dname1,dname2):
    mode = 'text'
    if options.html: mode='html'

    b1 = bulk_extractor_reader.BulkReport(dname1)
    b2 = bulk_extractor_reader.BulkReport(dname2)

    t = ttable.ttable()
    t.append_data(['bulk_diff.py Version:',bulk_diff_version])
    t.append_data(['PRE Image:',b1.image_filename()])
    t.append_data(['POST Image:',b2.image_filename()])
    out.write(t.typeset(mode=mode))

    for i in [1,2]:
        if i==1:
            a=b1;b=b2
        else:
            b=b1;a=b2;
        r = a.files.difference(b.files)
        if r:
            print("Files only in {}:".format(a.name))
            for f in r:
                if ".txt" in f:
                    print("     %s (%d lines)" % (f,a.count_lines(f)))
                else:
                    print("     %s" % (f))

    # Report interesting differences based on the historgrams.
    # Output Example:
        """
# in PRE     # in POST      ∆      Feature
10           20            10      steve@hotmail.com
 8           17             9      steve@mac.com
11           16             5      bobsmith@hotmail.com
"""
    b1_histograms = set(b1.histogram_files())
    b2_histograms = set(b2.histogram_files())
    common_histograms = b1_histograms.intersection(b2_histograms)
    
    if options.html:
        out.write("<ul>\n")
        for histogram_file in sorted(histogram_files):
            out.write("<li><a href='#%s'>%s</a></li>\n" % (histogram_file,histogram_file))
        out.write("</ul>\n<hr/>\n")

    for histogram_file in sorted(common_histograms):
        diffcount = 0
        if options.html:
            out.write('<h2><a name="%s">%s</a></h2>\n' % (histogram_file,histogram_file))
        else:
            out.write('\n')
        t = ttable.ttable()
        t.set_col_alignment(0,t.RIGHT)
        t.set_col_alignment(1,t.RIGHT)
        t.set_col_alignment(2,t.RIGHT)
        t.set_col_alignment(3,t.LEFT)
        t.set_title(histogram_file)
        t.append_head(['# in PRE','# in POST','∆','Value'])

        b1.hist = b1.read_histogram(histogram_file)
        b2.hist = b2.read_histogram(histogram_file)
        b1.keys = set(b1.hist.keys())
        b2.keys = set(b2.hist.keys())

        # Create the output, then we will sort on col 1, 2 and 4
        data = []
        for feature in b1.keys.union(b2.keys):
            v1 = b1.hist.get(feature,0)
            v2 = b2.hist.get(feature,0)
            if v1!=v2: diffcount += 1
            if v2>v1 or (v2==v1 and options.same) or (v2<v1 and options.smaller):
                data.append((v1, v2, v2-v1, feature.decode('utf-8')))

        # Sort according the diff first, then v2 amount, then v1 amount, then alphabetically on value
        def mysortkey(a):
            return (-a[2],a[3],a[1],a[0])

        if data:
            for row in sorted(data,key=mysortkey):
                t.append_data(row)
            out.write(t.typeset(mode=mode))
        if diffcount==0:
            if options.html:
                out.write("{}: No differences\n".format(histogram_file))
            else:
                out.write("{}: No differences\n".format(histogram_file))

            
    if options.features:
        for feature_file in b1.feature_files():
            if feature_file not in b2.feature_files():
                continue
            print("Compare features",feature_file)
            for p in [1,2]:
                if p==1:
                    a = b1; b = b2
                else:
                    a = b2; b = a
                a_features = {}
                for line in a.open(feature_file):
                    r = bulk_extractor_reader.parse_feature_line(line)
                    if not r: continue
                    a_features[r[0]] = r[1]
                for line in b.open(feature_file):
                    r = bulk_extractor_reader.parse_feature_line(line)
                    if not r: continue
                    if r[0] not in a_features:
                        print("{} {} is only in {}".format(r[0],r[1],a.name))




if __name__=="__main__":
    from optparse import OptionParser
    global options
    import sys,time

    parser = OptionParser()
    parser.usage = """usage: %prog [options] <pre> <post>
<pre> and <post> may be a bulk_extractor output directory or a zip file of a report.
"""
    parser.add_option("--smaller",help="Also show values that didn't change or got smaller",
                      action="store_true")
    parser.add_option("--same",help="Also show values that didn't change",action="store_true")
    parser.add_option("--tabdel",help="Specify a tab-delimited output file for easy import into Excel")
    parser.add_option("--html",help="HTML output. Argument is file name base")
    parser.add_option("--features",help="Compare feature files also",action='store_true')

    (options,args) = parser.parse_args()
    if len(args)!=2:
        parser.print_help()
        exit(1)
        
    out = sys.stdout
    if options.html:
        out = open(options.html,"w")
        out.write(html_header)
        out.write('<html><head>\n')
        out.write('<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>\n')
        out.write("</head><body>\n")

    process(out,args[0],args[1])
