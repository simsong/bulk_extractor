#!/usr/bin/env python3

import argparse
import numpy
import sys
from bulk_extractor_reader import BulkReport
from matplotlib import pyplot

def main():
    args = proc_args()

    x_var = args.x_var
    y_vars = list()
    if args.show_clocktime:
        y_vars += (AxisVar("clocktime"),)
    if args.show_memory:
        y_vars += (AxisVar("peak_memory"),)

    if len(y_vars) < 1:
        print("all y values suppressed", file=sys.stderr)
        return 1

    try:
        reports = tuple(sorted((BulkReport(x) for x in args.reports),
                key=lambda x: x_var.of(x)))
    except RuntimeError as e:
        print("failed to open reports: ", str(e), file=sys.stderr)
        return 1

    # check x uniqueness
    if len(set([x_var.of(report) for report in reports])) != len(reports):
        print("x value ({}) must be different for each report"
                .format(x_var.name), file=sys.stderr)
        return 1

    make_plot(reports, x_var, y_vars, args.output_filename)

def make_plot(reports, x_var, y_vars, filename):
    # TODO both time and mem
    y_var = y_vars[0]

    fig = pyplot.figure()
    ax = fig.add_axes([0.25, 0.1, 0.7, 0.8])

    numbins = len(reports)
    barHeight = 0.6

    pyplot.title("bulk_extractor Benchmark")

    for label in ax.yaxis.get_ticklabels():
        label.set_fontsize(6)
    for label in ax.xaxis.get_ticklabels():
        label.set_fontsize(7)

    ax.xaxis.get_label().set_fontsize(8.5)
    ax.yaxis.get_label().set_fontsize(8.5)
    
    pyplot.xlabel(x_var.label)
    pyplot.ylabel(y_var.label)

    x_values = tuple((x_var.of(report) for report in reports))
    y_values = tuple((y_var.of(report) for report in reports))

    pyplot.xticks(numpy.arange(numbins)+1.5*barHeight/2, x_values)
    rects = pyplot.bar(left=numpy.arange(numbins)+1.5*barHeight/2,
                     height=y_values,
                     width=barHeight,
                     align='center')

    pyplot.savefig(filename, format='pdf')

class AxisVar:
    def __init__(self, var_name):
        self.name = var_name
        if var_name == "version":
            self.of = lambda x: x.version()
            self.label = "bulk_extractor Version"
        elif var_name == "threads":
            self.of = lambda x: x.threads()
            self.label = "Number of Threads"
        elif var_name == "page_size":
            self.of = lambda x: x.page_size() / (1024 ** 2)
            self.label = "Page Size (MiB)"
        elif var_name == "margin_size":
            self.of = lambda x: x.margin_size() / (1024 ** 2)
            self.label = "Margin Size (MiB)"
        elif var_name == "clocktime":
            self.of = lambda x: x.clocktime()
            self.label = "Clock Time (seconds)"
        elif var_name == "peak_memory":
            self.of = lambda x: x.peak_memory() / 1024
            self.label = "Peak Memory Usage (MiB)"
        else:
            raise ValueError("unknown variable name " + str(var_name))

def proc_args():
    arg_parser = argparse.ArgumentParser(description=(
        "produce a performance data graph from various bulk_extractor outputs"))
    arg_parser.add_argument("reports", metavar="report", nargs="+",
            help="bulk_extractor report directory or zip file to graph")
    arg_parser.add_argument("-x", dest="x_var", type=AxisVar, default="version",
            help=("x axis variable (version, threads, page_size, or " +
                "margin_size)"))
    arg_parser.add_argument("-o", dest="output_filename",
            default="be_graph.pdf", help="(default \"be_graph.pdf\")")
    arg_parser.add_argument("--hide-clocktime", dest="show_clocktime",
            action="store_false", help="do not show clocktime on the y axis")
    arg_parser.add_argument("--hide-memory", dest="show_memory",
            action="store_false", help="do not show peak memory on the y axis")
    args = arg_parser.parse_args()
    return args

if __name__ == "__main__":
    main()
