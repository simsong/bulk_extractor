#!/usr/bin/env python3
"""
Reads multiple dfxml files and prepares a graph!
X and Y axis are independently specified.
Can specify multiple 'groups' which can be a single line, or multiple lines.
"""

import argparse
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import sys
import math
from bulk_extractor_reader import BulkReport

def make_plot(datasets, x_var, y_vars, group_var, filename):
    # TODO both time and mem
    y_var = y_vars[0]

    # use the longest dataset as the prototypical dataset
    exemplar = max(datasets, key=len)
    x_values = tuple((str(x_var.of(report)) for report in exemplar))
    # list of lists of y values
    all_y_values = tuple()

    minima = tuple()
    maxima = tuple()
    plot_labels = tuple()

    for dataset in datasets:
        all_y_values += (tuple((y_var.of(report) for report in dataset)),)
        minima += (min(all_y_values[-1]),)
        maxima += (max(all_y_values[-1]),)
        plot_labels += (group_var.name + " " + str(group_var.of(dataset[0])),)

    y_min = min(minima)
    y_max = max(maxima)
    y_range = y_max - y_min

    fig = plt.figure()
    ax = fig.add_axes((0.1, 0.2, 0.8, 0.7))

    # create open-looking plot by removing top and right spines and ticks
    ax.spines['right'].set_color('none')
    ax.spines['top'].set_color('none')
    ax.xaxis.set_ticks_position('bottom')
    ax.yaxis.set_ticks_position('left')

    # pad the plot a bit for aesthetics
    ax.set_xlim([-0.5, len(exemplar) - 0.5])
    ax.set_ylim([y_min - y_range * 0.1,
        y_max + y_range * (0.15 * len(plot_labels))])

    ax.set_xticks(np.arange(len(exemplar)))
    ax.set_xticklabels(x_values)

    for y_values, plot_label in zip(all_y_values, plot_labels):
        plt.plot(y_values, label=plot_label)

    plt.title("bulk_extractor Benchmark")
    plt.xlabel(x_var.label)
    plt.ylabel(y_var.label)
    if group_var.name != "none":
        plt.legend()

    plt.savefig(filename, format='pdf')

def plot_cpu(*, reports, filename):
    """Read reports and plot the CPU"""

    fig = plt.figure()
    ax = fig.add_axes((0.1, 0.2, 0.8, 0.7))
    ax.set_ylabel("% of CPU utilized")
    ax.set_xlabel("Seconds from start of run")
    ax.spines['right'].set_color('none')
    ax.spines['top'].set_color('none')
    ax.xaxis.set_ticks_position('bottom')
    ax.yaxis.set_ticks_position('left')
    ax.set_ylim(ymin=0,ymax=400)

    ymax = 100
    lines = []
    legends = []
    for report in reports:
        r = BulkReport(report)
        xyvals = r.cpu_track()

        # Extract timestamps and convert to seconds from beginning of run
        xvals = [xy[0] for xy in xyvals]
        xmin = min(xvals)
        xvals = [ (x-xmin).total_seconds() for x in xvals]

        # Extract CPU% and find max val
        yvals = [xy[1] for xy in xyvals]
        ymax  = max([ymax] + yvals)

        # Plot and retain handle
        line, = plt.plot( xvals, yvals, label='report')
        lines.append(line)

    ax.legend( handles=lines )        # draw legends
    ax.set_ylim(ymin=0, ymax=math.ceil(ymax/100)*100)   # set ymax
    plt.savefig(filename, format='pdf')

class AxisVar:
    """A class to return a function (x.of) and a label (x.label) for a given variable."""
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
        elif var_name == "image_size":
            self.of = lambda x: x.image_size() / (1000*1000)
            self.label = "Image Size (MB)"

        elif var_name == "none":
            self.of = lambda x: 0
            self.label = "Nothing"
        else:
            raise ValueError("unknown variable name " + str(var_name))
        print("self.of=",self.of)
        print("self.label=",self.label)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="produce a performance data graph from various bulk_extractor outputs")
    parser.add_argument("reports", metavar="report", nargs="+",
                            help="bulk_extractor report directory or DFXML file or zip file to graph")
    parser.add_argument("-x", "--x_var", default="version", type=AxisVar,
                            help="x axis variable (version, threads, page_size, image_size or margin_size)")
    parser.add_argument("-g", "--grouping", dest="group_var", type=AxisVar, help="variable to group results by")
    parser.add_argument("-o", "--output_filename", default="be_graph.pdf")
    parser.add_argument("--memory", "--show_memory", action="store_true",
                        help="plot bulk_extractor runs agains their peak memory usage")
    parser.add_argument("--cpu", action="store_true", help="plot bulk_extractor cpu vs. time")
    parser.add_argument("--xdups_ok", help="It is okay to have dups on the X axis", action='store_true')

    args = parser.parse_args()
    if args.group_var:
        group_var = AxisVar(args.group_var)
        print("group_var 1", group_var)
    else:
        group_var = AxisVar("none")
        print("group_var 2")
        print(group_var)

    if args.cpu:
        plot_cpu( reports=args.reports, filename=args.output_filename )
        exit(0)

    y_vars = []
    if args.show_memory:
        y_vars.append(AxisVar("peak_memory"))
    elif args.cpu:
        y_vars.append(AxisVar("cpu"))
    else:
        y_vars.append(AxisVar("clocktime"))

    try:
        reports = sorted((BulkReport(x) for x in args.reports), key=group_var.of)
    except RuntimeError as e:
        print("failed to open reports: ", str(e), file=sys.stderr)
        exit(1)

    # group related data into datasets
    datasets = []
    dataset  = []
    for report in reports:
        if len(dataset) < 1:
            dataset.append(report)
            continue
        if group_var.of(report) != group_var.of(dataset[0]):
            datasets += (sorted(dataset, key=args.x_var.of),)
            dataset.append(report)
            continue
        dataset.append(report)
    datasets.append(sorted(dataset, key=args.x_var.of))


    # check x uniqueness
    if args.xdups_ok==False:
        for dataset in datasets:
            if len(set([args.x_var.of(report) for report in dataset])) != len(dataset):
                print("x value ({}) must be unique per grouping ({})"
                      .format(args.x_var.name, group_var.name), file=sys.stderr)
                for d in dataset:
                    print(args.x_var.of(d), d)
                exit(1)

    make_plot(datasets, args.x_var, y_vars, group_var, args.output_filename)
