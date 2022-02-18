#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "config.h"

#include "sbuf_decompress.h"
#include "be20_api/scanner_params.h"

uint32_t   gzip_max_uncompr_size = 256*1024*1024; // don't decompress objects larger than this

extern "C"
void scan_gzip(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT){
        sp.info->set_name("gzip" );
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Searches for GZIP-compressed data";
        sp.info->scanner_version= "1.1";
        sp.info->scanner_flags.recurse = true;
        sp.get_scanner_config("gzip_max_uncompr_size",&gzip_max_uncompr_size,"maximum size for decompressing GZIP objects");
	return ;		/* no features */
    }
    if (sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = (*sp.sbuf);

        for (unsigned int i=0; i < sbuf.pagesize && i < sbuf.bufsize-4; i++) {

	    /** Look for the signature for beginning of a GZIP file.
	     * See zlib.h and RFC1952
	     * http://www.15seconds.com/Issue/020314.htm
	     *
	     */
            if( sbuf_decompress::is_gzip_header( sbuf, i)){
                auto *decomp = sbuf_decompress::sbuf_new_decompress( sbuf.slice(i),
                                                                     gzip_max_uncompr_size, "GZIP" ,sbuf_decompress::mode_t::GZIP, 0);
                if (decomp!=nullptr) {
                    assert(sbuf.depth()+1 == decomp->depth()); // make sure it is 1 deeper!
                    sp.recurse(decomp);                        // recurse will free the sbuf
                }
            }
	}
    }
}
