#!/usr/bin/env python3

import argparse
import matplotlib.pyplot as plt
import numpy as np
import sys
from bulk_extractor_reader import BulkReport

def main():
    args = proc_args()

    group_var = args.group_var
    x_var = args.x_var
    y_vars = tuple()
    if args.show_memory:
        y_vars += (AxisVar("peak_memory"),)
    else:
        y_vars += (AxisVar("clocktime"),)

    if len(y_vars) < 1:
        print("all y values suppressed", file=sys.stderr)
        return 1

    try:
        reports = tuple(sorted((BulkReport(x) for x in args.reports),
                key=group_var.of))
    except RuntimeError as e:
        print("failed to open reports: ", str(e), file=sys.stderr)
        return 1

    # group related data into datasets
    datasets = tuple()
    dataset = tuple()
    for report in reports:
        if len(dataset) < 1:
            dataset += (report,)
            continue
        if group_var.of(report) != group_var.of(dataset[0]):
            datasets += (sorted(dataset, key=x_var.of),)
            dataset = (report,)
            continue
        dataset += (report,)
    datasets += (sorted(dataset, key=x_var.of),)
    del(dataset)

    # check x uniqueness
    for dataset in datasets:
        if len(set([x_var.of(report) for report in dataset])) != len(dataset):
            print("x value ({}) must be unique per grouping ({})"
                    .format(x_var.name, group_var.name), file=sys.stderr)
            return 1

    make_plot(datasets, x_var, y_vars, group_var, args.output_filename)

def make_plot(datasets, x_var, y_vars, group_var, filename):
    # TODO both time and mem
    y_var = y_vars[0]

    # use the longest dataset as the prototypical dataset
    exemplar = max(datasets, key=len)
    x_values = tuple((str(x_var.of(report)) for report in exemplar))
    # list of lists of y values
    y_valueses = tuple()

    minima = tuple()
    maxima = tuple()
    plot_labels = tuple()

    for dataset in datasets:
        y_valueses += (tuple((y_var.of(report) for report in dataset)),)
        minima += (min(y_valueses[-1]),)
        maxima += (max(y_valueses[-1]),)
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

    for y_values, plot_label in zip(y_valueses, plot_labels):
        plt.plot(y_values, label=plot_label)

    plt.title("bulk_extractor Benchmark")
    plt.xlabel(x_var.label)
    plt.ylabel(y_var.label)
    if group_var.name != "none":
        plt.legend()

    plt.savefig(filename, format='pdf')

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
        elif var_name == "none":
            self.of = lambda x: 0
            self.label = "Nothing"
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
    arg_parser.add_argument("-g", "--grouping", dest="group_var", type=AxisVar,
            default=AxisVar("none"), help="variable to group results by")
    arg_parser.add_argument("-o", dest="output_filename",
            default="be_graph.pdf", help="(default \"be_graph.pdf\")")
    arg_parser.add_argument("--memory", dest="show_memory",
            action="store_true", help=("plot bulk_extractor runs agains " +
                "their peak memory usage"))
    args = arg_parser.parse_args()
    return args

if __name__ == "__main__":
    sys.exit(main())
