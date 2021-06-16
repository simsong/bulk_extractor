#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "be13_api/scanner_params.h"
#include "managed_malloc.h"


#define ZLIB_CONST
#ifdef HAVE_DIAGNOSTIC_UNDEF
#  pragma GCC diagnostic ignored "-Wundef"
#endif
#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

#include <iostream>
#include <iomanip>
#include <cassert>

#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

uint32_t   gzip_max_uncompr_size = 256*1024*1024; // don't decompress objects larger than this

extern "C"
void scan_gzip(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT){
        auto info = new scanner_params::scanner_info( scan_gzip, "gzip" );
        info->author         = "Simson Garfinkel";
        info->description    = "Searches for GZIP-compressed data";
        info->scanner_version= "1.1";
        //info->flags          = scanner_info::SCANNER_RECURSE | scanner_info::SCANNER_RECURSE_EXPAND;
        sp.ss.sc.get_config("gzip_max_uncompr_size",&gzip_max_uncompr_size,"maximum size for decompressing GZIP objects");
        sp.info = info;
	return ;		/* no features */
    }
    if (sp.phase==scanner_params::PHASE_SCAN){

	const sbuf_t &sbuf = (*sp.sbuf);
	const pos0_t &pos0 = sbuf.pos0;

        for (int i=0; i < sbuf.pagesize && i < sbuf.bufsize-4; i++) {

	    /** Look for the signature for beginning of a GZIP file.
	     * See zlib.h and RFC1952
	     * http://www.15seconds.com/Issue/020314.htm
	     *
	     */
	    if (sbuf[i+0]==0x1f && sbuf[i+1]==0x8b && sbuf[i+2]==0x08){ // gzip HTTP flag
		u_int compr_size = sbuf.bufsize - i; // up to the end of the buffer
                u_char *decompress_buf = (u_char *)malloc(gzip_max_uncompr_size); // allocate a huge chunk of memory
                if (decompress_buf==nullptr){
                    throw std::bad_alloc();
                }
                const unsigned char *src = sbuf.get_buf() + i;
                z_stream zs;
                memset(&zs,0,sizeof(zs));

                zs.next_in  = (Bytef *)src;
                zs.avail_in = compr_size;
                zs.next_out = (Bytef *)decompress_buf;
                zs.avail_out = gzip_max_uncompr_size;

                gz_header_s gzh;
                memset(&gzh,0,sizeof(gzh));

                int r = inflateInit2(&zs,16+MAX_WBITS);
                if (r==0){
                    r = inflate(&zs,Z_SYNC_FLUSH);
                    /* Ignore the error code; process data if we got any */
                    if (zs.total_out>0){
                        /* Shrink the allocated region */
                        decompress_buf = realloc(decompress_buf, zs.total_out);

                        /* run decompress.buf through the recognizer.
                         */
                        const pos0_t pos0_gzip = (pos0 + i) + "GZIP";
                        auto *nsbuf = sbuf_t::sbuf_new( pos0_gzip, decompress_buf, zs.total_out, zs.total_out);
                        sp.recurse(nsbuf);
                    }
                    r = inflateEnd(&zs);
                } else {
                    /* just free the buffer */
                    free(decompress_buf);
                }
            }
	}
    }
}
