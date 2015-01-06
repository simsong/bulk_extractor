#!/usr/bin/python
#
# Test the bulk_extractor HTTP interface

from subprocess import Popen,PIPE

fn = '/corp/nps/drives/nps-2009-ubnist1/ubnist1.gen3.E01'

cmd = ["bulk_extractor","-p","-http",fn]
print(" ".join(cmd))
p = Popen(cmd,stdout=PIPE,stdin=PIPE,bufsize=0)

def send(headers):
    for h in headers:
        print("SEND: %s" % h)
        p.stdin.write(h)
        p.stdin.write(b"\r\n")
    p.stdin.write(b"\r\n")
    print("")

def get_response():
    content_length = None
    while True:
        line = p.stdout.readline()
        if line=="\r\n": break
        line = line.decode('utf-8').strip()
        print("GOT: %s" % line)
        if line.startswith("Content-Length: "):
            content_length = int(line[16:])
    ret = p.stdout.read(content_length)
    print("------------------")
    return ret


if __name__=="__main__":
    # "/corp/drives/nps/nps-2009-ubnist1/ubnist1.gen3.E01"
    send([b"GET 1118910976-GZIP-0 HTTP/1.1", b"Range: bytes=80000-90000"])
    get_response()

    send([b"GET 1118910976-GZIP-0 HTTP/1.1", b"Range: bytes=80000-90000"])
    get_response()

    exit(1)

