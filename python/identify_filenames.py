#!/usr/bin/python
"""
Reports where the features are located within an image.

This script relies on the XML output produced by `fiwalk' and the TXT files
produced by `bulk_extractor'.  By combining these two, the script will show
the file on the filesystem where the `bulk_extractor' match was found.
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

__version__='1.3.0'

import bisect,os

def make_utf8(s):
    """Takes a string, unicode, or something else and make it UTF-8"""
    # Try the most basic conversion first. If they work, we return
    try:
        if type(s)==unicode:
            return s.encode('utf-8')
        elif type(s)==str:
            return s
        else:
            return unicode(str(s),'utf-8').replace("\\","\\\\")
    except UnicodeDecodeError:
        pass
    except UnicodeEncodeError:
        pass
    # It didn't work; convert character-by-character.
    ret = ""
    for ch in s:
        if(ord(ch)<ord(' ')):
            ret += "\\%03o" % ord(ch)
            continue
        try:
            ret += ch.encode('utf-8').replace("\\","\\\\")
            continue
        except UnicodeDecodeError:
            pass
        except UnicodeEncodeError:
            pass
        ret += "\\%03o" % ord(ch)
    return ret


class featuredb:
    """An object that holds all of the features in file order and provides
    rapid search into it.  This is implemented as an array of tuples where
    the tuple is (int offset,array of elements).  Currently this is a sorted
    array of feature names."""
    def __init__(self):
        self.fary = []          # each element is (offset,fields)
        self.sorted = True

    def __iter__(self):
        return self.fary.__iter__()

    def print_debug(self):
        print("feature array size: {}".format(len(self.fary)))
        if not self.fary: return
        print("First entry: {}".format(self.fary[0]))
        print("Last entry: {}".format(self.fary[-1]))

    def dump(self):
        for e in self.fary:
            print(e)


    def count(self):
        """Return the number of features in the DB"""
        return len(self.fary)

    def add_featurefile_line(self,line):
        """Adds a feature found at a given position to the data base.
        Returns the element added. The element consists of an offset and the fields
        from the feature file"""
        fields = line.split(b'\t')
        pos = fields[0]

        # Check the first column to ensure we can use it
        if pos.isdigit():
            offset = int(pos)
        elif b'-' in pos:
            offset = int(pos[0:pos.find(b"-")])
        else:
            print("First field does not appear to be valid for this script, saw: %s" % fields[0])
            exit(1)

        self.fary.append((offset,fields))
        self.sorted = False
        return fields

    def search(self,run):
        """ Returns a list of all the features in self.fary that fall within the byte run.
        self.fary is sorted by offset. No two features can have the same offset.
        We take the run starting point and find the first feature that fits in the run
        using the bisection algorithm. Then we scan through the array until we find the first
        feature that starts outside the run.
        """
        ret = []
        if run.img_offset==None:    # no offset
            return []
        if self.sorted==False:
            self.fary.sort()
            self.sorted=True
        
        # See if the run appears within the array
        try:
            p = bisect.bisect_left(self.fary,(run.img_offset,[b'',b'',b'']))
        except AttributeError:
            return []           # no known image offset in the run
        except TypeError as te:
            print(te)
            for e in self.fary:
                print(e)
            print("run.img_offset=",run.img_offset)
            raise te
        # Now we have a starting point ; look until the end
        for i in range(p,len(self.fary)):
            f = self.fary[i]   # the file 
            if not run.img_offset or not run.len: # no image offset
                continue
            if run.img_offset <= f[0] < run.img_offset+run.len:
                ret.append(f)
            if f[0] > run.img_offset:
                break       # all of the array entries now bigger
        return ret

    def print_unused(self):
        print("The following features were not found in any file:")
        for p in self.fary:
            if p[3]==False:
                print("\t".join(p[1:3]))

def findex(f):
    return f[0]+f[1]

def process_featurefile(args,report,featurefile):
    # Counters for the summary report
    global file_count
    features = featuredb()
    unallocated_count = 0
    feature_count = 0
    features_compressed = 0
    located_count = 0
    unicode_encode_errors = 0
    unicode_decode_errors = 0
    file_count = 0

    ofn = os.path.join(args.outdir,("annotated_" + featurefile ))
    if os.path.exists(ofn):
        raise RuntimeError(ofn+" exists")
    of = open(ofn,"wb")

    # First read the feature files
    print("Adding features from "+featurefile)
    try:
        linenumber = 0
        for line in report.open(featurefile,mode='rb'):
            # Read the file in binary and convert to unicode if possible
            linenumber += 1
            if bulk_extractor_reader.is_comment_line(line):
                continue
            try:
                fset = features.add_featurefile_line(line[0:-1])
                feature_count += 1
                if (b"ZIP" in fset[0]) or (b"HIBER" in fset[0]):
                    features_compressed += 1
                del fset
            except ValueError:
                raise RuntimeError("Line {} in feature file {} is invalid: {}".format(linenumber,featurefile,line))
    except IOError:
         print("Error: Failed to open feature file '%s'" % fn)
         exit(1)
    
    if args.debug:
        print('')
        features.print_debug()

    # feature2fi is a map each feature to the file in which it was found
    feature2fi = {}

    ################################################################
    # If we got features in the featuredb, find out the file that each one came from
    # by scanning all of the files and, for each byte run, indicating the features
    # that are within the byte run
    if features.count()>0:
        global filecount
        def process(fi):
            global file_count
            file_count += 1
            if args.verbose or args.debug:
                print("%d %s (%d fragments)" % (file_count,fi.filename(),fi.fragments()))
            for run in fi.byte_runs():
                for (offset,fset) in features.search(run):
                    if args.debug:
                        print("  run={} offset={} fset={} ".format(run,offset,fset))
                    feature2fi[findex(fset)] = fi    # for each of those features, not that it is in this file
            if file_count%1000==0:
                print("Processed %d fileobjects in DFXML file" % file_count)

        xmlfile = None
        if args.xmlfile:
            xmlfile = args.xmlfile
        else:
            if args.imagefile:
                imagefile = args.imagefile
            else:
                imagefile = report.imagefile()
            # See if there is an xmlfile
            (root,ext) = os.path.splitext(imagefile)
            possible_xmlfile = root+".xml"
            if os.path.exists(possible_xmlfile):
                xmlfile = possible_xmlfile
        if xmlfile:
            print("Using XML file "+xmlfile)
            fiwalk.fiwalk_using_sax(xmlfile=open(xmlfile,'rb'),callback=process)
        else:
            print("Running fiwalk on " + imagefile)
            fiwalk.fiwalk_using_sax(imagefile=open(imagefile,'rb'),callback=process)
    else:
        print("No features found; copying feature file")
    ################################################################

    print("Generating output...")

    # Now print all of the features
    if args.terse:
        of.write(b"# Position\tFeature\tFilename\n")
    else:
        of.write(b"# Position\tFeature\tContext\tFilename\tFile MD5\n")
    for (offset,fset) in features:
        try:
            of.write(fset[0]) # pos
            of.write(b"\t")
            of.write(fset[1]) # feature
            of.write(b"\t")
            try:
                if not args.terse:
                    of.write(fset[2]) # context
            except IndexError:
                pass            # no context
            try:
                fi = feature2fi[findex(fset)]
                of.write(b"\t")
                if fi.filename(): of.write(fi.filename().encode('utf-8'))
                if args.debug:
                    print("pos=",offset,"feature=",fset[1],"fi=",fi,"fi.filename=",fi.filename())
                if not args.terse:
                    of.write(b"\t")
                    if fi.md5(): of.write(fi.md5().encode('utf-8'))
                located_count += 1
            except KeyError:
                unallocated_count += 1
                pass            # cannot locate
            of.write(b"\n")
        except UnicodeEncodeError:
            unicode_encode_errors += 1
            of.write(b"\n")
        except UnicodeDecodeError:
            unicode_decode_errors += 1
            of.write(b"\n")

    # stop the timer used to calculate the total run time
    t1 = time.time()

    # Summary report
    for (title,value) in [["Total features input: {}",feature_count],
                          ["Total features located to files: {}",located_count],
                          ["Total features in unallocated space: {}",unallocated_count],
                          ["Total features in compressed regions: {}",features_compressed],
                          ["Unicode Encode Errors: {}",unicode_encode_errors],
                          ["Unicode Decode Errors: {}",unicode_decode_errors],
                          ["Total processing time: {} seconds",t1-t0]]:
        of.write((title+"\n").format(value).encode('utf-8'))


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
    parser.add_argument('bulk_extractor_output', action='store', help='Directory or ZIP file of bulk_extractor output')
    parser.add_argument('outdir',action='store',help='Output directory; must not exist')
    parser.add_argument('--all',action='store_true',help='Process all feature files')
    parser.add_argument('--featurefiles', action='store', help='Specific feature file to process; separate with commas')
    parser.add_argument('--imagefile', action='store', help='Overwrite location of image file from bulk_extractor output')
    parser.add_argument('--xmlfile', action='store', help="Don't run fiwalk; use the provided XML file instead")
    parser.add_argument('--list', action='store_true', help='List feature files in bulk_extractor_output and exit')
    parser.add_argument('-t', dest='terse', action='store_true', help='Terse output')
    parser.add_argument('-v', action='version', version='%(prog)s version '+__version__, help='Print Version and exit')
    parser.add_argument("--verbose",action="store_true",help='Verbose mode')
    parser.add_argument('--debug', action='store_true',help='Debug mode')
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

    if args.featurefiles:
        for fn in args.featurefiles.split(","):
            process_featurefile(args,report,fn)

    if args.all:
        for fn in report.feature_files():
            if not report.is_histogram_file(fn):
                process_featurefile(args,report,fn)
        
