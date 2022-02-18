/**
 *
 *
 * scan_httplogs extracts various web server (httpd/nginx/etc) access logs
 * without tying to any specific log format
 *
 *
 */

#include "config.h"

#include "be20_api/scanner_params.h"

/* We accept printable ASCII characters and \n, \r only */
bool isok(const char chr)
{
    if((chr == '\n') || (chr == '\r')) return true;
    return !((chr > 0x7e) || (chr < ' '));
}

/* Check if the character is a decimal digit */
bool isdigit(const char chr)
{
    if((chr >= '0') && (chr <= '9')) return true;
    return false;
}

/* Check if the string starts (!) with a valid dotted quad (IP address) */
bool validDottedQuad(std::string str)
{
    unsigned long val = 0;
    unsigned int dots = 0;
    bool partpresent = false;
    char c;

    std::string::iterator i = str.begin();
    while(i != str.end()) {
        c = *i;
        for(;;) {
            if(isdigit(c) && (i != str.end())) {
                partpresent = true;
                val = val * 10 + (c - '0');
                i++;
                c = *i;
            } else break;
        }
        if((c == '.') && partpresent && (i != str.end())) {
            if((val > 255) || (dots > 3)) return false;
            val = 0;
            dots++;
            i++;
            partpresent = false;
        } else break; /* Ignore further characters */
    }
    return ((dots == 3) && partpresent && (val <= 255));
}

/* Main function */
extern "C"
void scan_httplogs(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.info->set_name("httplogs");
        sp.info->author		= "Maxim Suhanov";
        sp.info->description	= "Extract various web server access logs";
        sp.info->feature_defs.push_back( feature_recorder_def("httplogs"));
        return;
    }

    if(sp.phase==scanner_params::PHASE_SCAN){
	feature_recorder &httplogs_recorder = sp.named_feature_recorder("httplogs");
        const sbuf_t &sbuf = *(sp.sbuf);

        for (size_t p = 0; p < sbuf.pagesize; p++) {
            /* We support the following methods: GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS, CONNECT;
             * and the following protocol versions: HTTP/1.1, HTTP/1.0, HTTP/0.9.
             *
             * Request lines are similar to these:
             *  - HTTP/1.1: GET / HTTP/1.1
             *  - HTTP/1.0: GET / HTTP/1.0
             *  - HTTP/0.9: GET /
             *
             * The plugin should output access log entries even for incorrect requests (like POST in HTTP/0.9).
             */
            ssize_t bytes_left = sbuf.bufsize - p;
            if(((bytes_left > 3)
             && (sbuf[p+0] == 'G') && (sbuf[p+1] == 'E') && (sbuf[p+2] == 'T')
             && (sbuf[p+3] == ' ')) ||

             ((bytes_left > 4)
             && (sbuf[p+0] == 'H') && (sbuf[p+1] == 'E') && (sbuf[p+2] == 'A')
             && (sbuf[p+3] == 'D') && (sbuf[p+4] == ' ')) ||

             ((bytes_left > 4)
             && (sbuf[p+0] == 'P') && (sbuf[p+1] == 'O') && (sbuf[p+2] == 'S')
             && (sbuf[p+3] == 'T') && (sbuf[p+4] == ' ')) ||

             ((bytes_left > 3)
             && (sbuf[p+0] == 'P') && (sbuf[p+1] == 'U') && (sbuf[p+2] == 'T')
             && (sbuf[p+3] == ' ')) ||

             ((bytes_left > 6)
             && (sbuf[p+0] == 'D') && (sbuf[p+1] == 'E') && (sbuf[p+2] == 'L')
             && (sbuf[p+3] == 'E') && (sbuf[p+4] == 'T') && (sbuf[p+5] == 'E')
             && (sbuf[p+6] == ' ')) ||

             ((bytes_left > 5)
             && (sbuf[p+0] == 'T') && (sbuf[p+1] == 'R') && (sbuf[p+2] == 'A')
             && (sbuf[p+3] == 'C') && (sbuf[p+4] == 'E')
             && (sbuf[p+5] == ' ')) ||

             ((bytes_left > 7)
             && (sbuf[p+0] == 'O') && (sbuf[p+1] == 'P') && (sbuf[p+2] == 'T')
             && (sbuf[p+3] == 'I') && (sbuf[p+4] == 'O') && (sbuf[p+5] == 'N')
             && (sbuf[p+6] == 'S') && (sbuf[p+7] == ' ')) ||

             ((bytes_left > 7)
             && (sbuf[p+0] == 'C') && (sbuf[p+1] == 'O') && (sbuf[p+2] == 'N')
             && (sbuf[p+3] == 'N') && (sbuf[p+4] == 'E') && (sbuf[p+5] == 'C')
             && (sbuf[p+6] == 'T') && (sbuf[p+7] == ' '))) {
                /* Got something, now we should find the next \n (in the nearest kilobyte) */
                size_t lineend = 0;
                for (size_t np = p + 5; ((np < sbuf.bufsize) && (np <= p + 5 + 1024)); np++) {
                if(!isok(sbuf[np])) break; /* Bad character found */
                    if(sbuf[np] == '\n') {
                        lineend = np;
                        break;
                    }
                }

                if(lineend > 0) {
                    /* Now we should find the previous \n or any non-printable character (in the nearest kilobyte) */
                    size_t np;
                    size_t linestart = 0;
                    bool foundlinestart = false;
                    for (np = p-1; ((np > 0) && (p - np <= 1024)); np--) {
                        if((!isok(sbuf[np])) || (sbuf[np] == '\n')) {
                            linestart = np + 1;
                            foundlinestart = true;
                            break;
                        }
                    }
                    if(np == 0) foundlinestart = true;

                    if(foundlinestart) {
                        /* Check for a valid IP address (dotted quad) */
                        bool ipaddrfound = false;
                        size_t length = lineend - linestart;
                        sbuf_t n(sbuf, linestart, length);
                        for (size_t cp = 0; (cp < length) && !ipaddrfound; cp++) {
                            ipaddrfound = validDottedQuad(std::string(n.asString(), cp, length-cp));
                            if((cp > 0) && (n[cp-1] == '/')) ipaddrfound = false; /* False positive */
                        }

                        /* Output the entry found */
                        if(ipaddrfound) httplogs_recorder.write_buf(n, 0, length);
                        p = lineend;
                    }
                }
            }
        }
    }
}
