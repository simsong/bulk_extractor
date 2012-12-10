#include "bulk_extractor.h"

#include <stdlib.h>
#include <string.h>

#define ZLIB_CONST
#ifdef GNUC_HAS_DIAGNOSTIC_PRAGMA
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;

#ifdef GNUC_HAS_DIAGNOSTIC_PRAGMA
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

extern "C"
void scan_gzip(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name  = "gzip";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Searches for GZIP-compressed data";
        sp.info->scanner_version= "1.0";
	return ;			/* no features */
    }
    if(sp.phase==scanner_params::shutdown) return;
    if(sp.phase==scanner_params::scan){

	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

	for(const unsigned char *cc=sbuf.buf ;
	    cc < sbuf.buf+sbuf.pagesize && cc < sbuf.buf+sbuf.bufsize-4 ;
	    cc++){
	    /** Look for the signature for beginning of a GZIP file.
	     * See zlib.h and RFC1952
	     * http://www.15seconds.com/Issue/020314.htm
	     *
	     * I'd like to use managed_malloc<> here, but I do a realloc to ease
	     * memory pressure...
	     */
	    if(cc[0]==0x1f && cc[1]==0x8b && cc[2]==0x08){ // gzip HTTP flag
		u_int compr_size = sbuf.bufsize - (cc-sbuf.buf); // up to the end of the buffer 
		u_char *decompress_buf = (u_char *)calloc(max_uncompr_size,1);
		if(decompress_buf){
		    z_stream zs;
		    memset(&zs,0,sizeof(zs));
		
		    zs.next_in = (Bytef *)cc;
		    zs.avail_in = compr_size;
		    zs.next_out = (Bytef *)decompress_buf;
		    zs.avail_out = max_uncompr_size;
		
		    gz_header_s gzh;
		    memset(&gzh,0,sizeof(gzh));

		    int r = inflateInit2(&zs,16+MAX_WBITS);
		    if(r==0){
			r = inflate(&zs,Z_SYNC_FLUSH);
			/* Ignore the error code; process data if we got any */
			if(zs.total_out>0){	
			    /* run decompress_buf through the recognizer.
			     * First, free what we didn't use (relieve memory pressure)
			     */
			    decompress_buf = (u_char *)realloc(decompress_buf,zs.total_out);
			    const ssize_t pos = cc-sbuf.buf;
			    const pos0_t pos0_gzip = (pos0 + pos) + rcb.partName;
			    const sbuf_t sbuf_new(pos0_gzip,decompress_buf,zs.total_out,zs.total_out,false);
			    (*rcb.callback)(scanner_params(sp,sbuf_new)); // recurse
			    if(rcb.returnAfterFound){
				free(decompress_buf);
				return;
			    }
			}
			r = inflateEnd(&zs);
		    }
		    free(decompress_buf);
		}
	    }
	}
    }
}
