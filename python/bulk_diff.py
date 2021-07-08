#!/usr/bin/env python3
# coding=UTF-8
# perform a diff of two bulk_extractor output directories.

__version__='1.5.0'
import os
import path
import sys

import ttable, bulk_extractor_reader

html_header = '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">\n'
__version__ = '2.0.0-dev'

class BulkDiff:
    def __init__(self, dname1, dname2, *, out, both=False, mode='text'):
        self.b1 = bulk_extractor_reader.BulkReport(dname1)
        self.b2 = bulk_extractor_reader.BulkReport(dname2)
        self.out = out
        self.both = both
        self.mode = mode
        self.only_features = set()
        self.only_features.update(self.b1.feature_files())
        self.only_features.update(self.b2.feature_files())

    def only_feature(self, feature):
        self.only_features = set([feature])

    def getab(self, phase=1):
        if phase==1:
            return (self.b1, self.b2)
        return (self.b2, self.b1)

    def compare_files(self):
        out = self.out
        if self.both:
            t = ttable.ttable()
            t.append_data(['bulk_diff.py Version:',__version__])
            t.append_data(['PRE Image:',self.b1.image_filename()])
            t.append_data(['POST Image:',self.b2.image_filename()])
            out.write(t.typeset(mode=self.mode))

        for i in [1,2]:
            (a,b) = self.getab(i)
            r = a.files.difference(b.files)
            total_diff  = sum([a.count_lines(f) for f in r if ".txt" in f])
            total_other = sum(1 for f in r if ".txt" not in f)
            if total_diff>0 or total_other>0 or args.both:
                print("Files only in {}:".format(a.name), file=out)
                for f in r:
                    if ".txt" in f:
                        print("     %s (%d lines)" % (f,a.count_lines(f)), file=out)
                    else:
                        print("     %s" % (f), file=out)

    def compare_histograms(self):
        b1_histograms = set(self.b1.histogram_files())
        b2_histograms = set(self.b2.histogram_files())
        common_histograms = b1_histograms.intersection(b2_histograms)
        out = self.out

        if self.mode=='html':
            out.write("<ul>\n")
            for histogram_file in sorted(histogram_files):
                out.write("<li><a href='#%s'>%s</a></li>\n" % (histogram_file,histogram_file))
            out.write("</ul>\n<hr/>\n")

        for histogram_file in sorted(common_histograms):
            diffcount = 0
            if args.html:
                out.write('<h2><a name="%s">%s</a></h2>\n' % (histogram_file,histogram_file))
            t = ttable.ttable()
            t.set_col_alignment(0,t.RIGHT)
            t.set_col_alignment(1,t.RIGHT)
            t.set_col_alignment(2,t.RIGHT)
            t.set_col_alignment(3,t.LEFT)
            t.set_title(histogram_file)
            t.append_head(['# in PRE','# in POST','∆','Value'])

            (b1,b2) = self.getab()

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
                if v2>v1 or (v2==v1 and args.same) or (v2<v1 and args.smaller):
                    data.append((v1, v2, v2-v1, feature.decode('utf-8')))

            # Sort according the diff first, then v2 amount, then v1 amount, then alphabetically on value
            def mysortkey(a):
                return (-a[2],a[3],a[1],a[0])

            if data:
                for row in sorted(data,key=mysortkey):
                    t.append_data(row)
                out.write(t.typeset(mode=self.mode))
            if diffcount==0 and args.both:
                if args.html:
                    out.write("{}: No differences\n".format(histogram_file))
                else:
                    out.write("{}: No differences\n".format(histogram_file))

    def compare_features(self):
        out = self.out
        for feature_file in self.b1.feature_files():
            if feature_file not in self.only_features:
                continue
            if feature_file not in self.b2.feature_files():
                continue
            print("Compare features",feature_file)
            if self.both:
                (a,b) = self.getab()
                a_offsets = set([bulk_extractor_reader.parse_feature_line(line)[0] for line in a.open(feature_file) if line[0]!=35])
                b_offsets = set([bulk_extractor_reader.parse_feature_line(line)[0] for line in b.open(feature_file) if line[0]!=35])
                common = a_offsets.intersection(b_offsets)
                for line in a.open(feature_file):
                    r = bulk_extractor_reader.parse_feature_line(line)
                    if r and r[0] in common:
                        print("{} {} IN BOTH".format(r[0].decode('utf-8'),r[1].decode('utf-8')), file=out)
            # differences
            for p in [1,2]:
                (a,b) = self.getab(p)
                a_features = {}
                for line in a.open(feature_file):
                    r = bulk_extractor_reader.parse_feature_line(line)
                    if not r: continue
                    a_features[r[0]] = r[1]
                for line in b.open(feature_file):
                    r = bulk_extractor_reader.parse_feature_line(line)
                    if not r: continue
                    if r[0] not in a_features:
                        print("{} {} is only in {}".format(r[0].decode('utf-8'),r[1].decode('utf-8'),a.name), file=out)


if __name__=="__main__":
    from argparse import ArgumentParser
    import sys,time

    parser = ArgumentParser()
    parser.add_argument("--smaller",help="Show features that didn't change or got smaller", action="store_true")
    parser.add_argument("--same",help="Show features that didn't change",action="store_true")
    parser.add_argument("--tabdel",help="Specify a tab-delimited output file for easy import into Excel")
    parser.add_argument("--html",help="HTML output. Argument is file name base")
    parser.add_argument("--features",help="Compare feature files also",action='store_true')
    parser.add_argument("--both", help="Notify of feature files that are the same", action='store_true')
    parser.add_argument("--perf", help="Analyze clock-time performance differences", action='store_true')
    parser.add_argument("--feature", help="Only look at this feature")
    parser.add_argument("file1", help="First bulk_extractor report (directory or zip file)")
    parser.add_argument("file2", help="Second bulk_extractor report (directory or zip file)")

    args = parser.parse_args()
    if args.html:
        out = open(args.html,"w")
        out.write(html_header)
        out.write('<html><head>\n')
        out.write('<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>\n')
        out.write("</head><body>\n")

    bd = BulkDiff(args.file1, args.file2, out=sys.stdout, both=args.both, mode='html' if args.html else 'text')
    if args.feature:
        bd.only_feature(args.feature)
    bd.compare_files()
    bd.compare_histograms()
    bd.compare_features()
