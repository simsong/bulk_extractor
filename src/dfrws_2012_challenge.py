#!/usr/bin/env python
# coding=UTF-8
"""
DFRWS 2012 Challenge
"""

b'This module needs Python 3.3 or later.'
from subprocess import Popen,call,PIPE
import os,os.path,glob,sys,zipfile,tempfile

sys.path.append("python")

page_size = 1024*1024
margin    = 1024*1024

def write_govdocs(f,fn):
    with open("/corp/nps/files/govdocs1m/" + fn[0:3] + "/" + fn,"rb") as g:
        f.write(g.read(4096))
        
def write_file(f,fn):
    with open(fn,"rb") as g:
        f.write(g.read(4096))
        

def make_target():
    print("Making target...")
    f = open("target.raw","wb")
    f.write(b"\000"*4096)
    f.write(b"AB"*2048)
    write_file(f,"/dev/random")
    for fn in ["001207.jpg","001219.csv","001232.gif","001237.ps","001254.text",
               "001268.ppt","001272.html","001273.pdf","001920.gz","001939.doc"]:
        write_govdocs(f,fn)

def run_bulk_extractor(target,block_size,concurrency_factor,quiet=True,retbuf=False):
    """ Run the copy of bulk_extractor out of the same directory in which the script is running.
    set the DFRWS2012 challenge variables."""
    dn = os.path.dirname(os.path.realpath(__file__))
    os.putenv("DFRWS2012","1")
    d = tempfile.mkdtemp(prefix="pbulk")
    os.rmdir(d)                         # delete it, so I can make it again
    qflag = '0'
    if quiet:
        qflag = '-1'
    cmd = [os.path.join(dn,'./bulk_extractor'),
           '-e','all',                  # all modules
           '-x','accts',                # turn off accts
           '-x','email',                # turn off email
           '-x','wordlist',             # no wordlist
           '-s','DFRWS2012=YES',
           '-s','bulk_block_size='+str(block_size),
           '-s','work_start_work_end=NO',
           '-C','0',                    # no context is reported
           '-G'+str(page_size),     # page size
           '-g'+str(margin),     # margin
           '-q',qflag,            # quiet mode
           '-j',str(concurrency_factor),
           '-o',d,
           target]
    if not quiet: print(" ".join(cmd))
    if not retbuf:
        call(cmd)
    else:
        ret=Popen(cmd,stdout=PIPE).communicate()[0].strip()
        try:
            space=ret.find(' ')
            return ret[space+1:]
        except TypeError:
            space=ret.find(b' ')
            return ret[space+1:].decode('utf-8')            
    if not quiet: print("output dir: {}".format(d))    


def identify_block(blk):
    """Identify a block and return the identification"""
    fn = tempfile.mktemp()
    with open(fn,"wb") as f:
        f.write(blk)
    return run_bulk_extractor(fn,len(blk),1,quiet=True,retbuf=True)
    

if __name__=="__main__":
    import argparse 
    import sys,time

    quiet = False

    if len(sys.argv)>1 and sys.argv[1]=='--quiet':
        quiet = True
        del sys.argv[1]

    if len(sys.argv) not in (3,4):
        print("Usage: {} <target> <block_size> [<concurrency_factor>]".format(sys.argv[0]))
        sys.exit(1)

    if sys.argv[1]=="make":
        make_target()
        sys.exit(1)

    target = sys.argv[1]
    if not os.path.exists(target):
        print("{}: file does not exist".format(target))
        sys.exit(1)
    block_size = int(sys.argv[2])
    concurrency_factor = 1
    if len(sys.argv)==4:
        concurrency_factor = int(sys.argv[3])
    run_bulk_extractor(target,block_size,concurrency_factor,quiet)


