#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "config.h"

#include "sbuf_decompress.h"
#include "be13_api/scanner_params.h"

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

        for (unsigned int i=0; i < sbuf.pagesize && i < sbuf.bufsize-4; i++) {

	    /** Look for the signature for beginning of a GZIP file.
	     * See zlib.h and RFC1952
	     * http://www.15seconds.com/Issue/020314.htm
	     *
	     */
            if(sbuf_decompress_zlib_possible( sbuf, i)){
                auto *decomp = sbuf_decompress_zlib_new( sbuf.slice(i), gzip_max_uncompr_size, "GZIP" );
                if (decomp==nullptr) continue;
                sp.recurse(decomp);      // recurse will free the sbuf
            }
	}
    }
}
