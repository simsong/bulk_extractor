#!/usr/bin/env python3
#
# given a dist and the bulk_extractor release, print the files missing.
# used for debugging 'make dist' and 'make checkdist'
"""
I used this:

sh bootstrap.sh && ./configure && make dist && python3 etc/whats-missing-from-dist.py bulk_extractor-2.1.0.tar.gz

Debug by looking for:
make[1]: *** No rule to make target 'be20_api/dfxml_cpp/src/Makefile.defs'.  Stop.

in:
bulk_extractor-2.1.0/_build/sub/
"""



import os
from os.path import dirname,abspath
import re
import subprocess
import pathlib

DEV_ROOT = dirname(dirname(abspath(__file__)))

IGNORE_PATTERNS = ['[.]git',
                   'autom4te.cache',
                   '[.]Po',
                   'bulk_extractor-(.*)[.]tar[.]gz',
                   'bulk_extractor.opensuse.spec',
                   'bulk_extractor.fedora.spec',
                   'java_gui/src/Config.java',
                   '~$',
                   'stamp-h1?',
                   'config.h$',
                   '[.]log$',
                   '[.]status$'
                   ]
compiled_ignore_patterns = [re.compile(i) for i in IGNORE_PATTERNS]

def ignore_fname( fn ):
    for p in compiled_ignore_patterns:
        if p.search(fn):
            return True
    return False

def remove_first( fn ):
    return "/".join(fn.split('/')[1:])


def compare(root, archive):
    os.chdir(root)
    dev_files = set(  [remove_first(p) for p in subprocess.check_output(["find",".","-type","f","-print"],encoding='utf-8').split("\n")] )
    rel_files = set(  [remove_first(p) for p in subprocess.check_output(["tar","tfz",archive],encoding='utf-8').split("\n")] )
    #for p in sorted(dev_files):
    #    print('dev:', p)
    #for p in sorted(rel_files):
    #    print('rel:', p)

    for fn in sorted(dev_files):
        if (fn not in rel_files) and (not ignore_fname(fn)):
            print('missing:',fn)


if __name__=='__main__':
    import argparse
    parser = argparse.ArgumentParser(description="Report what's missing form a distribution",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('tgz', help='bulk_extractor .tar.gz release')
    args = parser.parse_args()
    compare(DEV_ROOT,args.tgz)
