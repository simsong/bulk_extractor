#!/usr/bin/env python3
#
# postprocess the EXIF output file into a CSV
#

__version__='1.3.0'

import xml.parsers.expat,os,os.path,sys,codecs

class ExifParser:
    def __init__(self,data):
        self.data = {}
        p = xml.parsers.expat.ParserCreate()
        p.StartElementHandler = self.start_element
        p.CharacterDataHandler = self.char_data
        p.Parse(data)
    def start_element(self,name,attrs):
        self.element = name
        if self.element not in self.data:
            self.data[self.element] = ""
    def char_data(self,data):
        self.data[self.element] += data

if __name__=="__main__":
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("--zap",help="erase outfile",action="store_true")
    parser.usage = "usage: %prog [options] input.txt output.csv"
    (options,args) = parser.parse_args()
    if len(args)!=2:
        parser.print_help()
        exit(1)

    (infile,outfile) = args
    
    if os.path.exists(outfile) and not options.zap:
        raise IOError(outfile+" Exists")
        exit(1)

    invalid_tags = 0
    tags = set()
    print("Input file: {}".format(infile))
    print("Output file: {}".format(outfile))
    out = open(outfile,"w")
    print("Scanning for EXIF tags...")
    for line in open(infile,'r'):
        if ord(line[0:1])==65279: line=line[1:]
        if line[0:1]=='#': continue
        (offset,hash,xmlstr) = line.split("\t")
        try:
            p = ExifParser(xmlstr)
            for tag in p.data.keys():
                tags.add(tag)
        except xml.parsers.expat.ExpatError:
            invalid_tags += 1
            pass
    taglist = list(tags)
    print("There are {} exif tags ".format(len(taglist)))
    if invalid_tags:
        print("There are {} invalid tags".format(invalid_tags))

    # sort the tags into some sensible order
    def sortfun(a):
        return (".entry_" in a,a)
    taglist.sort(key=sortfun)

    import csv
    writer = csv.writer(open(outfile,"w"),delimiter=',',quotechar='"',quoting=csv.QUOTE_MINIMAL)
    writer.writerow(taglist)
    for line in open(infile,'r'):
        if ord(line[0:1])==65279: line=line[1:]
        if line[0:1]=='#': continue
        (offset,hash,xmlstr) = line.split("\t")
        try:
            p = ExifParser(xmlstr)
            writer.writerow([p.data.get(key,"") for key in taglist])
        except xml.parsers.expat.ExpatError:
            print("Invalid XML: {}".format(xmlstr))
            pass




        
        
