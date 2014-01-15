#!/usr/bin/env python3

"""This program creates a BE report that is a statistical sample of an original BE report.
The user can then grade the report. A second pass then reports the accuracy of the BE report."""

from bulk_extractor_reader import BulkReport,is_comment_line
import random,re

def get_lines_array(f):
    """Returns an array of integers corresponding to each line in the feature file"""
    line_number = 0
    line_numbers = []
    if args.pattern:
        pattern = args.pattern.encode('utf-8')
    else:
        pattern = None
    if args.xpattern:
        xpattern = args.xpattern.encode('utf-8')
    else:
        xpattern = None
    for line in f:
        line_number += 1
        if pattern and not pattern in line.split(b"\t")[0]:
            continue
        if xpattern and xpattern in line.split(b"\t")[0]:
            continue
        if is_comment_line(line):
            continue
        line_numbers.append(line_number)
    return line_numbers

def sample(outdir,fn):
    line_numbers = get_lines_array(report.open(fn,"r"))
    count = min(args.count,len(line_numbers))
    print("{} has {} lines".format(fn,len(line_numbers)))
    lines_to_sample = sorted(random.sample(line_numbers,count))
    line_number = 0
    with open(os.path.join(outdir,fn),"w") as out:
        out.write("# -*- mode:text; truncate-lines:t -*-\n")
        out.write("# Sampled {} out of {}\n".format(count,len(line_numbers)))
        out.write("# Place '=' or 'y' in front of correct classifications and '-' or 'x' in front of incorrect ones\n")
        with report.open(fn) as f:
            for line in f:
                line_number += 1
                if is_comment_line(line):
                    out.write(line.decode('utf-8'))
                if line_number in lines_to_sample:
                    out.write("{}:\t".format(line_number))
                    out.write(line.decode('utf-8'))

def calc_stats(fn):
    wrong = 0
    right = 0
    for line in open(fn,"r"):
        if line[0]=='#':
            m = re.search("# Sampled (\d+) out of (\d+)",line)
            if m:
                sampled = int(m.group(1))
                total = int(m.group(2))
        elif line[0] in '-_xzn' :
            wrong += 1
        elif line[0] in '+=y' :
            right += 1
        else:
            if not args.quiet: print("No classification:",line,end='');
    return {"fn":os.path.basename(fn),
            "total":total,
            "sampled":sampled,
            "sampling_rate":sampled/total if total>0 else 0,
            "accuracy":(right/sampled if sampled>0 else 1),
            "error_rate":(wrong/sampled if sampled>0 else 0),
            "uncertainity":(sampled-(right+wrong))/sampled if sampled>0 else 0}
            

def calc_report(dirname):
    for (dirpath,dirnames,filenames) in os.walk(dirname):
        for filename in filenames:
            if filename.endswith("~"): continue
            fn = os.path.join(dirpath,filename)
            r = calc_stats(fn)
            if r: res.append(r)
    print("Report: {}".format(args.calc))
    print("{:20} {:8}     {:8}     {:8} {:8}".format("Feature","Total","Sampled","Accuracy","Err Rate"))
    for r in res:
        print("{:20} {:8} {:8} ({:4.0f}%)    {:4.0f}%   {:4.0f}%".format(
                r['fn'],r['total'],r['sampled'],r['sampling_rate']*100.0,r['accuracy']*100.0,r['error_rate']*100.0))
    
    
if __name__ == "__main__":
    import argparse,sys,os
    arg_parser = argparse.ArgumentParser(description=(
        "Create a bulk_extractor report that is sampled from an existing report. Number each feature file line; do not copy over the histograms. Currently does not handle carved objects"))
    arg_parser.add_argument("report", metavar="report", 
            help="bulk_extractor report directory or zip file to graph")
    arg_parser.add_argument("output", type=str, help="Output directory")
    arg_parser.add_argument("--count", type=int, default="100",
            help="Number of items to sample")
    arg_parser.add_argument("--pattern", type=str, help="Only sample lines that include this pattern in the forensic path")
    arg_parser.add_argument("--xpattern", type=str, help="Do not sample lines that include this pattern in the forensic path")
    arg_parser.add_argument("--calc", help="Compute the statistics",action="store_true")
    arg_parser.add_argument("--trials", type=int, default="5", help="Number of trials to divide into")
    arg_parser.add_argument("--quiet",action='store_true',help='do not alert on lines with no classification')
    args = arg_parser.parse_args()
    
    res = []
    if args.calc:
        for (dirpath,dirnames,filenames) in os.walk(args.output):
            for filename in filenames:
                if filename.endswith("~"): continue
                fn = os.path.join(dirpath,filename)
                r = calc_stats(fn)
                print(r)
                res.append(r)
        print("{:20} {:8} {:8} {:4} {:8} {:8}".format("Feature","Total","Sampled","%","Accuracy","Err Rate"))
        for r in res:
            print("{:20} {:8} {:8} {:4}% {:8} {:8}".format(
                    r['fn'],r['total'],r['sampled'],r['sampled']*100.0/r['total'],r['accuracy'],r['error_rate']))
        exit(0)

    if args.sample:
        (input,output) = args.sample
        if os.path.exists(output):
            raise RuntimeError(output+" exists")
    
        os.mkdir(output)
        report = BulkReport(input)
        for fn in report.feature_files():
            sample(output,fn)
