#!/usr/bin/env python

import os
import sys
sys.path.append("../python/module")
os.environ['BULK_EXTRACTOR_LIB_PATH'] = "../src/bulk_extractor.so"
import bulkextractor as b_e

scanners = [ "email", "accts", "exif", "zip", "gzip", "rar", "bulk", ]
be = b_e.BulkExtractor(scanners)
be.memory_mode()
be.analyze_buffer("Test Dataa  demo@bulk_extractor.org Just a demo 617-555-1212 ok!")
be.finalize()
for featurefile, histogram in be.histograms().items():
    print("{}:".format(featurefile))
    for entry in histogram:
        print("    {}		{}".format(entry.count, entry.feature))
