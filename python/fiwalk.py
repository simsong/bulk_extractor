#!/usr/bin/env python
### DO NOT MODIFY THIS FILE ###
### DOWNLOAD NEW FILE FROM https://raw.github.com/simsong/dfxml/master/python/fiwalk.py

# This software was developed in whole or in part by employees of the
# Federal Government in the course of their official duties, and with
# other Federal assistance. Pursuant to title 17 Section 105 of the
# United States Code portions of this software authored by Federal
# employees are not subject to copyright protection within the United
# States. For portions not authored by Federal employees, the Federal
# Government has been granted unlimited rights, and no claim to
# copyright is made. The Federal Government assumes no responsibility
# whatsoever for its use by other parties, and makes no guarantees,
# expressed or implied, about its quality, reliability, or any other
# characteristic.
#
# We would appreciate acknowledgement if the software is used.

#
# fiwalk version 0.6.3
#
# %%% BEGIN NO FILL
"""fiwalk module

This is the part of dfxml that is dependent on fiwalk.py

"""
import dfxml
from sys import stderr
from subprocess import Popen,PIPE
ALLOC_ONLY = 1

fiwalk_cached_installed_version = None
def fiwalk_installed_version(fiwalk='fiwalk'):
    """Return the current version of fiwalk that is installed"""
    global fiwalk_cached_installed_version
    if fiwalk_cached_installed_version:
        return fiwalk_cached_installed_version
    from subprocess import Popen,PIPE
    import re
    for line in Popen([fiwalk,'-V'],stdout=PIPE).stdout.read().decode('utf-8').split("\n"):
        g = re.search("^FIWalk Version:\s+(.*)$",line)
        if g:
            fiwalk_cached_installed_version = g.group(1)
            return fiwalk_cached_installed_version
        g = re.search("^SleuthKit Version:\s+(.*)$",line)
        if g:
            fiwalk_cached_installed_version = g.group(1)
            return fiwalk_cached_installed_version
    return None

class XMLDone(Exception):
    def __init__(self,value):
        self.value = value

class version:
    def __init__(self):
        self.cdata = ""
        self.in_element = []
        self.version = None
    def start_element(self,name,attrs):
        if(name=='volume'):     # too far?
            raise XMLDone(None)
        self.in_element += [name]
        self.cdata = ""
    def end_element(self,name):
        if ("fiwalk" in self.in_element) and ("creator" in self.in_element) and ("version" in self.in_element):
            raise XMLDone(self.cdata)
        if ("fiwalk" in self.in_element) and ("fiwalk_version" in self.in_element):
            raise XMLDone(self.cdata)
        if ("version" in self.in_element) and ("dfxml" in self.in_element) and ("creator" in self.in_element):
            raise XMLDone(self.cdata)
        self.in_element.pop()
        self.cdata = ""
    def char_data(self,data):
        self.cdata += data

    def get_version(self,fn):
        import xml.parsers.expat
        p = xml.parsers.expat.ParserCreate()
        p.StartElementHandler  = self.start_element
        p.EndElementHandler    = self.end_element
        p.CharacterDataHandler = self.char_data
        try:
            p.ParseFile(open(fn,'rb'))
        except XMLDone as e:
            return e.value
        except xml.parsers.expat.ExpatError:
            return None             # XML error
        
def fiwalk_xml_version(filename=None):
    """Returns the fiwalk version that was used to create an XML file.
    Uses the "quick and dirty" approach to getting to getting out the XML version."""

    p = version()
    return p.get_version(filename)

################################################################
def E01_glob(fn):
    import os.path
    """If the filename ends .E01, then glob it. Currently only handles E01 through EZZ"""
    ret = [fn]
    if fn.endswith(".E01") and os.path.exists(fn):
        fmt = fn.replace(".E01",".E%02d")
        for i in range(2,100):
            f2 = fmt % i
            if os.path.exists(f2):
                ret.append(f2)
            else:
                return ret
        # Got through E99, now do EAA through EZZ
        fmt = fn.replace(".E01",".E%c%c")
        for i in range(0,26):
            for j in range(0,26):
                f2 = fmt % (i+ord('A'),j+ord('A'))
                if os.path.exists(f2):
                    ret.append(f2)
                else:
                    return ret
        return ret              # don't do F01 through F99, etc.
    return ret


def fiwalk_xml_stream(imagefile=None,flags=0,fiwalk="fiwalk",fiwalk_args=""):
    """ Returns an fiwalk XML stream given a disk image by running fiwalk."""
    if flags & ALLOC_ONLY: fiwalk_args += "-O"
    from subprocess import call,Popen,PIPE
    # Make sure we have a valid fiwalk
    try:
        res = Popen([fiwalk,'-V'],stdout=PIPE).communicate()[0]
    except OSError:
        raise RuntimeError("Cannot execute fiwalk executable: "+fiwalk)
    cmd = [fiwalk,'-x']
    if fiwalk_args: cmd += fiwalk_args.split()
    p = Popen(cmd + E01_glob(imagefile.name),stdout=PIPE)
    return p.stdout

def fiwalk_using_sax(imagefile=None,xmlfile=None,fiwalk="fiwalk",flags=0,callback=None,fiwalk_args=""):
    """Processes an image using expat, calling a callback for every file object encountered.
    If xmlfile is provided, use that as the xmlfile, otherwise runs fiwalk."""
    import dfxml
    if xmlfile==None:
        xmlfile = fiwalk_xml_stream(imagefile=imagefile,flags=flags,fiwalk=fiwalk,fiwalk_args=fiwalk_args)
    r = dfxml.fileobject_reader(flags=flags)
    r.imagefile = imagefile
    r.process_xml_stream(xmlfile,callback)

def fileobjects_using_sax(imagefile=None,xmlfile=None,fiwalk="fiwalk",flags=0):
    ret = []
    fiwalk_using_sax(imagefile=imagefile,xmlfile=xmlfile,fiwalk=fiwalk,flags=flags,
                     callback = lambda fi:ret.append(fi))
    return ret

def fileobjects_using_dom(imagefile=None,xmlfile=None,fiwalk="fiwalk",flags=0,callback=None):
    """Processes an image using expat, calling a callback for every file object encountered.
    If xmlfile is provided, use that as the xmlfile, otherwise runs fiwalk."""
    import dfxml
    if xmlfile==None:
        xmlfile = fiwalk_xml_stream(imagefile=imagefile,flags=flags,fiwalk=fiwalk)
    return dfxml.fileobjects_dom(xmlfile=xmlfile,imagefile=imagefile,flags=flags)

ctr = 0
def cb_count(fn):
    global ctr
    ctr += 1

if __name__=="__main__":
    import sys
    for fn in sys.argv[1:]:
        print("{} contains fiwalk version {}".format(fn,fiwalk_xml_version(fn)))
        # Count the number of files
        fiwalk_using_sax(xmlfile=open(fn,'rb'),callback=cb_count)
        print("Files: {}".format(ctr))
