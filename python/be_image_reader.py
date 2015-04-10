#!/usr/bin/env python3
#
# A program to read part of a disk image using bulk_extractor
#
from subprocess import Popen,PIPE

# It would be better to do this with the HTTP client, but I couldn't get that to work under Python

class BEImageReader:
    def __init__(self,fname):
        open(fname,"rb").close() # generate a file not found error if it does not exist
        self.fname = fname
        self.p = Popen(["bulk_extractor","-p",'-http',self.fname],stdin=PIPE,stdout=PIPE,bufsize=0)

    def read(self,offset,amount):
        def readline():
            r = b'';
            while True:
                r += self.p.stdout.read(1)
                if r[-1]==ord('\n'):
                    return r
            
        self.p.stdin.write("GET {} HTTP/1.1\r\nRange: bytes=0-{}\r\n\r\n".format(offset,amount-1).encode('utf-8'))
        params = {}
        while True:
            buf = readline().decode('utf-8').strip()
            if buf=='': break
            (n,v) = buf.split(": ")
            params[n] = v
        toread = int(params['Content-Length'])
        buf = b''
        while len(buf)!=toread:
            buf += self.p.stdout.read(toread)
        return buf

def all_null(buf):
    for ch in buf:
        if ch!=0: return False
    return True

if __name__=="__main__":
    import sys
    from argparse import ArgumentParser
    parser = ArgumentParser(prog='read_image.py', description='Read the disk image')
    parser.add_argument('image', type=str,  help='Image filename')
    parser.add_argument('sector', type=int, help='Offset to read')
    args = parser.parse_args()
    
    r = BEImageReader(args.image)
    buf = r.read(args.sector*512,512)
    # Determine if it is all null
    if all_null(buf):
        print("sector {} length {} is all nulls".format(args.sector,len(buf)))
    
