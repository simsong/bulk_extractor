#!/usr/bin/env python3
"""
Reports where the features are located within an image.

This script relies on the XML output produced by `fiwalk' and the TXT files
produced by `bulk_extractor'.  By combining these two, the script will show
the file on the filesystem where the `bulk_extractor' match was found.

There are two ways to do this: a database of features, which is
examined for every byte run, or a database of byte runs, which is
examined for every feature.
"""

import platform
if platform.python_version_tuple() < ('3','2','0'):
    raise RuntimeError('This script now requires Python 3.2 or above')

try:
    import dfxml, fiwalk, bulk_extractor_reader
except ImportError:
    raise ImportError('This script requires the dfxml and fiwalk modules for Python.')

try:
    from argparse import ArgumentParser
except ImportError:
    raise ImportError("This script requires ArgumentParser which is in Python 2.7 or Python 3.0")

__version__='1.4.0'

import bisect,os

class byterundb:
    """The byte run database holds a set of byte runs, sorted by the
    start byte. It can be searched to find the name of a file that
    corresponds to a byte run."""
    def __init__(self):
        self.rary = []          # each element is (runstart,runend,fname,md5)
        self.sorted = True      # whether or not sorted
    
    def __iter__(self):
        return self.rary.__iter__()
        
    def dump(self):
        for e in self.rary:
            print(e)

    def add_extent(self,offset,length,fname,md5val):
        self.rary.append((offset,offset+length,fname,md5val))
        self.sorted = False

    def search(self,pos):
        """Return the touple associated with a offset"""
        if self.sorted==False:
            self.rary.sort()
            self.sorted=True
        
        p = bisect.bisect_left(self.rary,((pos,0,"")))

        # If the offset matches the first byte in the returned byte run,
        # we have found the matching exten
        try:
            if self.rary[p][0] == pos:
                return self.rary[p]
        except IndexError:
            pass

        # If the first element in the array was found, all elements are to the right
        # of the provided offset, so there is no byte extent that maches.
        
        if p==0:
            return None

        # Look at the byte extent whose origin is to the left
        # of pos. If the extent includes pos, return it, otherwise
        # return None
        if self.rary[p-1][0] <= pos < self.rary[p-1][1]:
            return self.rary[p-1]

        return None

    def process_fi(self,fi):
        """Read an XML file and add each byte run to this database"""
        for run in fi.byte_runs():
            self.add_extent(run.img_offset,run.len,fi.filename(),fi.md5())
            
           
class byterundb2:
    """Maintain two byte run databases, one for allocated files, one for unallocated files."""
    def __init__(self):
        self.allocated = byterundb()
        self.unallocated = byterundb()
        self.filecount   = 0

    def process(self,fi):
        if fi.allocated():
            self.allocated.process_fi(fi)
        else:
            self.unallocated.process_fi(fi)
        self.filecount += 1
        if self.filecount % 1000==0:
            print("Processed %d fileobjects in DFXML file" % self.filecount)

    def read_xmlfile(self,fname):
        if fname.endswith(".xml"):
            fiwalk.fiwalk_using_sax(xmlfile=open(fname,'rb'),callback=self.process)
        else:
            fiwalk.fiwalk_using_sax(imagefile=open(fname,'rb'),callback=self.process)
    
    def search(self,offset):
        """First search the allocated. If there is nothing, search unallocated"""
        r = self.allocated.search(offset)
        if not r:
            r = self.unallocated.search(offset)
        return r

    def dump(self):
        print("Allocated:")
        self.allocated.dump()
        print("Unallocated:")
        self.unallocated.dump()
            

xor_re = re.compile(r"^(\d+)\-XOR\-(\d+)")

def decode_path_offset(offset):
    """If the path has an XOR transformation, add the offset within
    the XOR to the initial offset. Otherwise don't. Return the integer
    value of the offset."""
    m = xor_re.search(offset)
    if m:
        return int(m.group(1)+m.group(2))
    return int(offset[0:offset.find("-")])
    
def process_featurefile2(rundb,infile,outfile):
    """Returns features from infile, determines the file for each, writes results to outfile"""
    # Stats
    unallocated_count = 0
    feature_count = 0
    features_encoded = 0
    located_count = 0

    if args.terse:
        outfile.write(b"# Position\tFeature\tFilename\n")
    else:
        outfile.write(b"# Position\tFeature\tContext\tFilename\tFile MD5\n")
    t0 = time.time()
    for line in infile:
        if bulk_extractor_reader.is_comment_line(line):
            outfile.write(line)
            continue
        (offset,feature,context) = line[:-1].split(b'\t')
        feature_count += 1
        if b"-" in offset:
            ioffset = decode_path_offset(offset)
            features_encoded += 1
        else:
            ioffset = int(offset)
        tpl = rundb.search(ioffset)
        if tpl:
            located_count += 1
            fname = tpl[2].encode('utf-8') # THIS MIGHT GENERATE A UNICODE ERROR
            if tpl[3]:
                md5val = tpl[3].encode('utf-8')
            else:
                md5val = b""
        else:
            unallocated_count += 1
            fname = b""
            md5val = b""
        outfile.write(offset)
        outfile.write(b'\t')
        outfile.write(feature)
        if not args.terse:
            outfile.write(b'\t')
            outfile.write(context)
        outfile.write(b'\t')
        outfile.write(fname)
        if not args.terse:
            outfile.write(b'\t')
            outfile.write(md5val)
        outfile.write(b'\n')
    t1 = time.time()
    for (title,value) in [["# Total features input: {}",feature_count],
                          ["# Total features located to files: {}",located_count],
                          ["# Total features in unallocated space: {}",unallocated_count],
                          ["# Total features in encoded regions: {}",features_encoded],
                          ["# Total processing time: {:.2} seconds",t1-t0]]:
        outfile.write((title+"\n").format(value).encode('utf-8'))



if __name__=="__main__":
    import sys, time, re

    try:
        if dfxml.__version__ < "1.0.0":
            raise RuntimeError("Requires dfxml.py Version 1.0.0 or above")
    except AttributeError:
        raise RuntimeError("Requires dfxml.py Version 1.0.0 or above")

    try:
        if bulk_extractor_reader.__version__ < "1.0.0":
            raise RuntimeError("Requires bulk_extractor_reader.py Version 1.0.0 or above")
    except AttributeError:
        raise RuntimeError("Requires bulk_extractor_reader.py Version 1.0.0 or above")

    parser = ArgumentParser(prog='identify_filenames.py', description='Identify filenames from "bulk_extractor" output')
    parser.add_argument('bulk_extractor_output', action='store',
                        help='Directory or ZIP file of bulk_extractor output')
    parser.add_argument('outdir',action='store',
                        help='Output directory; must not exist')
    parser.add_argument('--all',action='store_true',
                        help='Process all feature files')
    parser.add_argument('--featurefiles', action='store',
                        help='Specific feature file to process; separate with commas')
    parser.add_argument('--imagefile', action='store',
                        help='Overwrite location of image file from bulk_extractor output')
    parser.add_argument('--xmlfile', action='store',
                        help="Don't run fiwalk; use the provided XML file instead")
    parser.add_argument('--list', action='store_true',
                        help='List feature files in bulk_extractor_output and exit')
    parser.add_argument('-t', dest='terse', action='store_true',
                        help='Terse output')
    parser.add_argument('-v', action='version', version='%(prog)s version '+__version__,
                        help='Print Version and exit')
    parser.add_argument("--verbose",action="store_true",
                        help='Verbose mode')
    parser.add_argument('--debug', action='store_true',
                        help='Debug mode')
    args = parser.parse_args()

    # Start the timer used to calculate the total run time
    t0 = time.time()

    # Open the report
    report = bulk_extractor_reader.BulkReport(args.bulk_extractor_output)
    
    if args.list:
        print("Feature files in {}:".format(args.bulk_extractor_output))
        for fn in report.feature_files():
            print(fn)
        exit(1)

    if not os.path.exists(args.outdir):
        os.mkdir(args.outdir)

    if not os.path.isdir(args.outdir):
        raise RuntimeError(args.outdir+" must be a directory")

    if not args.featurefiles and not args.all:
        raise RuntimeError("Please request a specific feature file or --all feature files")

    rundb = byterundb2()
    rundb.read_xmlfile(args.xmlfile)

    feature_file_list = None
    if args.featurefiles:
        feature_file_list = args.featurefiles.split(",")
    if args.all:
        feature_file_list = report.feature_files()

    for feature_file in feature_file_list:
        output_fn = os.path.join(args.outdir,("annotated_" + feature_file ))
        if os.path.exists(output_fn):
            raise RuntimeError(output_fn+" exists")
        print("feature_file:",feature_file)
        process_featurefile2(rundb,report.open(feature_file,mode='rb'),open(output_fn,"wb"))


