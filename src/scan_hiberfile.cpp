
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "be13_api/scanner_params.h"

#include "image_process.h"
#include "pyxpress.h"

#define SCANNER_NAME "HIBERFILE"


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

static const uint32_t windows_page_size = 4096;
static const uint32_t min_uncompr_size = 4096; // allow at least this much when uncompressing

/**
 * scan_hiberfile:
 * Look for elements of the hibernation file and decompress them.
 */

extern "C"
void scan_hiberfile(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT){
        sp.info = new scanner_params::scanner_info( scan_hiberfile, "hiberfile" );
        sp.info->author         = "Simson Garfinkel and Matthieu Suiche";
        sp.info->description    = "Scans for Microsoft-XPress compressed data";
        sp.info->scanner_version= "1.0";
	return; /* no features */
    }
    if (sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if (sp.phase==scanner_params::PHASE_SCAN){

	/* Do not scan for hibernation decompression if we are already
	 * inside a hibernation file decompression.
	 * Right now this is a hack; it should be done by the system with some kind of flag.
	 */
	const sbuf_t &sbuf = *(sp.sbuf);
	const pos0_t &pos0 = sbuf.pos0;

	if (pos0.path.find( SCANNER_NAME )!=std::string::npos){ // don't do recursively
	    return;
	}


	for (size_t pos = 0 ; pos < sbuf.bufsize; pos++) {

	    /**
	     * http://www.pyflag.net/pyflag/src/lib/pyxpress.c
             * Decompress each block separetly
	     */
	    if(sbuf[pos+0]==0x81 && sbuf[pos+1]==0x81 && sbuf[pos+2]==0x78 && sbuf[pos+3]==0x70 &&
	       sbuf[pos+4]==0x72 && sbuf[pos+5]==0x65 && sbuf[pos+6]==0x73 && sbuf[pos+7]==0x73){

                u_int compressed_length = (   (sbuf[pos+8]
                                            + (sbuf[pos+9]<<8)
                                            + (sbuf[pos+10] << 16)
                                            + (sbuf[pos+11]<<24)) >> 10) + 1; // ref: Hibr2bin/MemoryBlocks.cpp

                compressed_length = (compressed_length + 7) & ~7; // ref: Hibr2bin/MemoryBlocks.cpp
		const u_char *compressed_buf = sbuf.get_buf() + pos + 32;		 // "the header contains 32 bytes"
		u_int  remaining_size = sbuf.bufsize - (pos+32); // up to the end of the buffer
		size_t compr_size = compressed_length < remaining_size ? compressed_length : remaining_size;
		size_t max_uncompr_size = compr_size * 10; // hope that's good enough
		if (max_uncompr_size < min_uncompr_size) {
                    max_uncompr_size = min_uncompr_size; // it should at least be this large!
                }

                auto *decomp_sbuf = sbuf_t::sbuf_malloc(sbuf.pos0 + "HIBERFILE", max_uncompr_size);
                u_char *decomp = reinterpret_cast<u_char *>(decomp_sbuf->malloc_buf());
		int decompress_size = Xpress_Decompress(compressed_buf, compr_size, decomp, max_uncompr_size);

                if (decompress_size<=0) {
                    delete decomp;
                    return;
                }
                /* decomp is a buffer that may extend over multiple pages.
                 * Unfortunately the pages are not logically connected, because they are physical memory, and it is
                 * highly unlikely that adjacent logical pages will have adjacent physical pages. Therefore we now
                 * break up this buffer into 4096 byte chunks and process each individually.
                 * This prevents scanners like the JPEG carver from inadvertantly reassembling objects that make no semantic sense.
                 * However, we need to copy the data into each one (a copy!) because they may be processed asynchronously.
                 */
                for(ssize_t start = 0; start < decompress_size; start += windows_page_size){
                    auto *dbuf = decomp_sbuf->new_slice_copy( start, windows_page_size);
                    sp.recurse(dbuf);
		}
                delete decomp;
	    }
	}
    }
}
