/**
 *
 *
 * scan_httplogs extracts various web server (httpd/nginx/etc) access logs
 * without tying to any specific log format
 *
 *
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"

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
void scan_httplogs(const class scanner_params &sp, const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name		= "httplogs";
        sp.info->author		= "Maxim Suhanov";
        sp.info->description	= "Extract various web server access logs";
        sp.info->feature_names.insert("httplogs");
        return;
    }

    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;

    if(sp.phase==scanner_params::PHASE_SCAN){
	feature_recorder *httplogs_recorder = sp.fs.get_name("httplogs");

        for (size_t p = 0; p < sp.sbuf.pagesize; p++) {
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
            ssize_t bytes_left = sp.sbuf.bufsize - p;
            if(((bytes_left > 3)
             && (sp.sbuf[p+0] == 'G') && (sp.sbuf[p+1] == 'E') && (sp.sbuf[p+2] == 'T')
             && (sp.sbuf[p+3] == ' ')) ||

             ((bytes_left > 4)
             && (sp.sbuf[p+0] == 'H') && (sp.sbuf[p+1] == 'E') && (sp.sbuf[p+2] == 'A')
             && (sp.sbuf[p+3] == 'D') && (sp.sbuf[p+4] == ' ')) ||

             ((bytes_left > 4)
             && (sp.sbuf[p+0] == 'P') && (sp.sbuf[p+1] == 'O') && (sp.sbuf[p+2] == 'S')
             && (sp.sbuf[p+3] == 'T') && (sp.sbuf[p+4] == ' ')) ||

             ((bytes_left > 3)
             && (sp.sbuf[p+0] == 'P') && (sp.sbuf[p+1] == 'U') && (sp.sbuf[p+2] == 'T')
             && (sp.sbuf[p+3] == ' ')) ||

             ((bytes_left > 6)
             && (sp.sbuf[p+0] == 'D') && (sp.sbuf[p+1] == 'E') && (sp.sbuf[p+2] == 'L')
             && (sp.sbuf[p+3] == 'E') && (sp.sbuf[p+4] == 'T') && (sp.sbuf[p+5] == 'E')
             && (sp.sbuf[p+6] == ' ')) ||

             ((bytes_left > 5)
             && (sp.sbuf[p+0] == 'T') && (sp.sbuf[p+1] == 'R') && (sp.sbuf[p+2] == 'A')
             && (sp.sbuf[p+3] == 'C') && (sp.sbuf[p+4] == 'E')
             && (sp.sbuf[p+5] == ' ')) ||

             ((bytes_left > 7)
             && (sp.sbuf[p+0] == 'O') && (sp.sbuf[p+1] == 'P') && (sp.sbuf[p+2] == 'T')
             && (sp.sbuf[p+3] == 'I') && (sp.sbuf[p+4] == 'O') && (sp.sbuf[p+5] == 'N')
             && (sp.sbuf[p+6] == 'S') && (sp.sbuf[p+7] == ' ')) ||

             ((bytes_left > 7)
             && (sp.sbuf[p+0] == 'C') && (sp.sbuf[p+1] == 'O') && (sp.sbuf[p+2] == 'N')
             && (sp.sbuf[p+3] == 'N') && (sp.sbuf[p+4] == 'E') && (sp.sbuf[p+5] == 'C')
             && (sp.sbuf[p+6] == 'T') && (sp.sbuf[p+7] == ' '))) {
                /* Got something, now we should find the next \n (in the nearest kilobyte) */
                size_t lineend = 0;
                for (size_t np = p + 5; ((np < sp.sbuf.bufsize) && (np <= p + 5 + 1024)); np++) {
                if(!isok(sp.sbuf[np])) break; /* Bad character found */
                    if(sp.sbuf[np] == '\n') {
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
                        if((!isok(sp.sbuf[np])) || (sp.sbuf[np] == '\n')) {
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
                        sbuf_t n(sp.sbuf, linestart, length);
                        for (size_t cp = 0; (cp < length) && !ipaddrfound; cp++) {
                            ipaddrfound = validDottedQuad(std::string(n.asString(), cp, length-cp));
                            if((cp > 0) && (n[cp-1] == '/')) ipaddrfound = false; /* False positive */
                        }

                        /* Output the entry found */
                        if(ipaddrfound) httplogs_recorder->write_buf(n, 0, length);
                        p = lineend;
                    }
                }
            }
        }
    }
}
