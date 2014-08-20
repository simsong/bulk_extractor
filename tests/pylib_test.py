#!/usr/bin/env python

#
# This program tests the bulk_extractor shared library
# This test program rquires Python3 and should run on any OS for which the library is compiled.
# 
# The library's location is passed to the module in the environment variable
# BULK_EXTRACTOR_LIB_PATH

import os
import sys
sys.path.append("../python/module")
os.environ['BULK_EXTRACTOR_LIB_PATH'] = "../src/libbulkextractor.so"
import bulkextractor as b_e

scanners = [ "email", "accts", "exif", "zip", "gzip", "rar", "bulk", ]
b_e.init(scanners)

be = b_e.Session()
be.analyze_buffer("Test Dataa  demo@bulk_extractor.org Just a demo 617-555-1212 ok!")
be.finalize()
for featurefile, histogram in be.histograms().items():
    print("{}:".format(featurefile))
    for entry in histogram:
        print("    {}		{}".format(entry.count, entry.feature))

if len(sys.argv)==1:
    print("enter a device to analyze it")
else:
    be.analyze_device(sys.argv[1],1.0,65536)


