#!/usr/bin/env python3
#
# Little program to dump compressed PDF sections, useful for debugging. Not compiled for bulk_extractor
import re
import zlib
import os
import sys

STREAM_RE = re.compile(rb'.*?FlateDecode.*?stream(.*?)endstream', re.S)

def process(fname):
   print("============= {} ============".format(fname))
   with open(fname,'rb') as f:
      for s in STREAM_RE.findall(f.read()):
         try:
            #while len(s)>0 and s[0] in ['\r\n']:
            #   s = s[1:]
            s = s.strip(b"\r\n")
            print( zlib.decompress( s ))
            print("  --- ")
         except RuntimeError as e:
            print(e)


if __name__=="__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("files",nargs='+')
    args = parser.parse_args()
    for fname in args.files:
       process(fname)
