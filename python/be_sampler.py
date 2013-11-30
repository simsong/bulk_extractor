#!/usr/bin/env python3

from bulk_extractor_reader import BulkReport,is_comment_line
import random

def sample(outdir,fn):
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
    with report.open(fn) as f:
        for line in f:
            line_number += 1
            if pattern and not pattern in line:
                continue
            if xpattern and xpattern in line:
                continue
            if is_comment_line(line):
                continue
            line_numbers.append(line_number)
    count = min(args.count,len(line_numbers))
    print("{} has {} lines".format(fn,len(line_numbers)))
    lines_to_sample = sorted(random.sample(line_numbers,count))
    line_number = 0
    with open(os.path.join(outdir,fn),"wb") as out:
        out.write("# Sampled {} out of {}\n".format(count,len(line_numbers)).encode('utf-8'))
        with report.open(fn) as f:
            for line in f:
                line_number += 1
                if is_comment_line(line):
                    out.write(line)
                if line_number in lines_to_sample:
                    out.write("{}:\t".format(line_number).encode('utf-8'))
                    out.write(line)


    
        

if __name__ == "__main__":
    import argparse,sys,os
    arg_parser = argparse.ArgumentParser(description=(
        "Create a bulk_extractor report that is sampled from an existing report. Number each feature file line; do not copy over the histograms. Currently does not handle carved objects"))
    arg_parser.add_argument("report", metavar="report", 
            help="bulk_extractor report directory or zip file to graph")
    arg_parser.add_argument("output", type=str, help="Output directory")
    arg_parser.add_argument("--count", type=int, default="100",
            help="Number of items to sample")
    arg_parser.add_argument("--pattern", type=str, help="Only sample lines that include this pattern")
    arg_parser.add_argument("--xpattern", type=str, help="Do not sample lines that include this pattern")
    args = arg_parser.parse_args()
    
    if os.path.exists(args.output):
        raise RuntimeError(args.output+" exists")
    
    os.mkdir(args.output)
    report = BulkReport(args.report)
    for fn in report.feature_files():
        sample(args.output,fn)
