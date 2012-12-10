#!/usr/bin/python
#
# Test the bulk_extractor HTTP interface

from subprocess import Popen,PIPE

fn = '/corp/drives/nus/in/IN10-0117/IN10-0117.E01'
#fn = '/corp/drives/nps/nps-2009-domexusers/nps-2009-domexusers.E01'

cmd = ["src/bulk_extractor","-p","-http",fn]
print " ".join(cmd)
p = Popen(cmd,stdout=PIPE,stdin=PIPE)

def send(headers):
    for h in headers:
        print("SEND: %s" % h)
        p.stdin.write(h)
        p.stdin.write("\r\n")
    p.stdin.write("\r\n")
    print("")

def get_response():
    content_length = None
    while True:
        line = p.stdout.readline()
        if line=="\r\n": break
        line = line.strip()
        print("GOT: %s" % line)
        if "libewf_handle_open" in line:
            raise IOError("error in "+line)
        if line.startswith("Content-Length: "):
            content_length = int(line[16:])
    ret = p.stdout.read(content_length)
    print "------------------"
    return ret


if __name__=="__main__":
    send(["GET /info HTTP/1.1"])
    get_response()

    send(["GET 18646961664-GZIP-1138524 HTTP/1.1", "Range: bytes=0-9"])
    get_response()

    send(["GET 18646961664-GZIP-1138524 HTTP/1.1", "Range: bytes=0-9"])
    get_response()

    exit(1)
    

    # "/corp/drives/nps/nps-2009-ubnist1/ubnist1.gen3.E01"
    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Range: bytes=80000-90000"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Range: bytes=80000-90000"])
    get_response()

    exit(1)

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Content-Length: 100"])
    get_response()


    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Range: bytes=10-100"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Range: bytes=20-50"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Content-Length: 50"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Content-Length: 40"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Content-Length: 10000000000"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Content-Length: 10000000000"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Content-Range: bytes=1000-10000"])
    get_response()

    send(["GET 1118910976-GZIP-0 HTTP/1.1", "Content-Range: bytes=80000-90000"])
    get_response()


