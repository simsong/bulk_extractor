#include "config.h"
#include "be13_api/bulk_extractor_i.h"

#include <stdlib.h>
#include <string.h>

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

using namespace std;

#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

uint32_t   gzip_max_uncompr_size = 256*1024*1024; // don't decompress objects larger than this

extern "C"
void scan_gzip(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name  = "gzip";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Searches for GZIP-compressed data";
        sp.info->scanner_version= "1.0";
        sp.info->flags          = scanner_info::SCANNER_RECURSE | scanner_info::SCANNER_RECURSE_EXPAND;
        sp.info->get_config("gzip_max_uncompr_size",&gzip_max_uncompr_size,"maximum size for decompressing GZIP objects");
	return ;		/* no features */
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN){

	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

	for(const unsigned char *cc=sbuf.buf ;
	    cc < sbuf.buf+sbuf.pagesize && cc < sbuf.buf+sbuf.bufsize-4 ;
	    cc++){
	    /** Look for the signature for beginning of a GZIP file.
	     * See zlib.h and RFC1952
	     * http://www.15seconds.com/Issue/020314.htm
	     *
	     */
	    if(cc[0]==0x1f && cc[1]==0x8b && cc[2]==0x08){ // gzip HTTP flag
		u_int compr_size = sbuf.bufsize - (cc-sbuf.buf); // up to the end of the buffer 
                managed_malloc<u_char>decompress(gzip_max_uncompr_size);
		if(decompress.buf){
		    z_stream zs;
		    memset(&zs,0,sizeof(zs));
		
		    zs.next_in = (Bytef *)cc;
		    zs.avail_in = compr_size;
		    zs.next_out = (Bytef *)decompress.buf;
		    zs.avail_out = gzip_max_uncompr_size;
		
		    gz_header_s gzh;
		    memset(&gzh,0,sizeof(gzh));

		    int r = inflateInit2(&zs,16+MAX_WBITS);
		    if(r==0){
			r = inflate(&zs,Z_SYNC_FLUSH);
			/* Ignore the error code; process data if we got any */
			if(zs.total_out>0){	
			    /* run decompress.buf through the recognizer.
			     */
			    const ssize_t pos = cc-sbuf.buf;
			    const pos0_t pos0_gzip = (pos0 + pos) + rcb.partName;
			    const sbuf_t sbuf_new(pos0_gzip,decompress.buf,zs.total_out,zs.total_out,false);
			    (*rcb.callback)(scanner_params(sp,sbuf_new)); // recurse
			}
			r = inflateEnd(&zs);
		    }
		}
	    }
	}
    }
}
