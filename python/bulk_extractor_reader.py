#!/usr/bin/python
# coding=UTF-8
# 
# bulk_extractor_reader.py:
# A module for working with bulk_extractor

__version__ = "1.3.0"

b'This module needs Python 2.7 or later.'
import zipfile,os,os.path,glob,codecs,re

property_re = re.compile("# ([a-z0-9\-_]+):(.*)",re.I)

def is_comment_line(line):
    if len(line)==0: return False
    if line[0:4]==b'\xef\xbb\xbf#': return True
    if line[0:1]=='\ufeff':
        line=line[1:]       # ignore unicode BOM
    try:
        if ord(line[0])==65279:
            line=line[1:]       # ignore unicode BOM
    except TypeError:
        pass
    if line[0:1]==b'#' or line[0:1]=='#':
        return True
    return False

def is_histogram_line(line):
    return line[0:2]==b"n="

def get_property_line(line):
    """If it is a property line return the (property, value)"""
    if line[0:1]=='#':
        m = property_re.search(line)
        if m:
            return (m.group(1),m.group(2))
    return None

def is_feature_line(line):
    return len(line.split(b"\t")) in [2,3]

def is_histogram_filename(fname):
    """Returns true if this is a histogram file"""
    if "_histogram" in fname: return True
    if "url_" in fname: return True # I know this is a histogram file
    if fname=="ccn_track2.txt": return False # I know this is not
    return None                              # don't know

def is_feature_filename(fname):
    """Returns true if this is a feature file"""
    if "_histogram" in fname: return False
    if "_stopped" in fname: return False
    return None                 # don't know


class BulkReport:
    """Creates an object from a bulk_extractor report. The report can be a directory or a ZIP of a directory.
    Methods that you may find useful:
    f = b.open(fname,mode) - opens the file f and returns a file handle. mode defaults to 'rb'
    b.is_histogram_file(fname) - returns if fname is a histogram file or not
    b.image_file() - Returns the name of the image file
    b.read_histogram(fn) - Reads a histogram and returns the histogram
    b.histograms      - Set of histogram names
    b.files   - Set of feature file names
"""    

    def __init__(self,fn):
        def validate():
            """Validates the XML and finds the histograms and feature files"""
            import xml.dom.minidom
            import xml.parsers.expat
            try:
                self.xmldoc = xml.dom.minidom.parse(self.open("report.xml"))
            except xml.parsers.expat.ExpatError as e:
                raise IOError("Invalid report.xml file")
            return True

        import os.path,glob
        self.name  = fn
        if os.path.isdir(fn):
            self.zipfile = None
            self.dname = fn
            self.all_files = set([os.path.basename(x) for x in glob.glob(os.path.join(fn,"*"))])
            self.files = set([os.path.basename(x) for x in glob.glob(os.path.join(fn,"*.txt"))])
            validate()
            return
        if fn.endswith(".zip") and os.path.isfile(fn):
            self.zipfile = zipfile.ZipFile(fn)
            # extract the filenames and make a map
            self.files = set()
            self.map   = dict()
            for fn in self.zipfile.namelist():
                self.files.add(os.path.basename(fn))
                self.map[os.path.basename(fn)] = fn
            validate()
            return
        raise RuntimeError("Cannot process " + fn)

    def imagefile(self):
        """Returns the file name of the disk image that was used."""
        return self.xmldoc.getElementsByTagName("image_filename")[0].firstChild.wholeText

    def version(self):
        """Returns the version of bulk_extractor that made the file."""
        return self.xmldoc.getElementsByTagName("version")[0].firstChild.wholeText

    def open(self,fname,mode='r'):
        """Opens a named file in the bulk report. Default is text mode.
        Returns .bulk_extractor_reader as a pointer to self.
        """
        if self.zipfile:
            mode=mode.replace('b','') # remove the b if present; zipfile doesn't support
            f = self.zipfile.open(self.map[fname],mode=mode)
        else:
            fn = os.path.join(self.dname,fname)
            f = open(fn,mode=mode)
        f.bulk_extractor_reader = self
        return f

    def is_histogram_file(self,fn):
        if is_histogram_filename(fn)==True: return True
        for line in self.open(fn,'rb'):
            if is_comment_line(line): continue
            return is_histogram_line(line)
        return False

    def is_feature_file(self,fn):
        if is_feature_filename(fn)==False: return False
        for line in self.open(fn,'rb'):
            if is_comment_line(line): continue
            return is_feature_line(line)
        return False
        
    def histograms(self):
        """Returns a list of the histograms, by name"""
        return filter(lambda fn:self.is_histogram_file(fn),self.files)

    def feature_files(self):
        """Returns a list of the feature_files, by name"""
        return sorted(filter(lambda fn:self.is_feature_file(fn),self.files))

    def read_histogram(self,fn):
        """Read a histogram file and return a dictonary of the histogram """
        import re
        ret = {}
        r = re.compile("^n=(\d+)\t(.*)$")
        for line in self.open(fn,'rb'):
            line = line.decode('utf-8')
            m = r.search(line)
            if m:
                ret[m.group(2)] = int(m.group(1))
        return ret
        
if(__name__=='__main__'):
    from optparse import OptionParser
    global options
    import os

    parser = OptionParser()
    parser.add_option("--wordlist",help="split a wordlist.txt file")
    (options,args) = parser.parse_args()

    if options.wordlist:
        global counter
        fn = options.wordlist
        if not os.path.exists(fn):
            print("%s does not exist" % fn)
            exit(1)
        print("Splitting wordlist file %s" % fn)
        fntemp = fn.replace(".txt","_%03d.txt")
        counter = 0
        seen = set()

        def next_file():
            global counter
            nfn = fntemp % counter
            counter += 1
            print("Switching to file %s" % nfn) 
            return open(nfn,"w")

        f = next_file()
        size = 0
        max_size = 1000*1000*10
        for line in open(fn,"r"):
            word = line.split("\t")[1]
            if word not in seen:
                f.write(word)
                if word[-1]!='\n': f.write("\n")
                seen.add(word)
                size += len(word)+1
                if size>max_size:
                    f.close()
                    f = next_file()
                    size = 0
