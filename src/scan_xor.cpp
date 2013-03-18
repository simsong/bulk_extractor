/*
 * scan_xor: optimistically search for features trivially obfuscated with xor
 * author:   Michael Shick <mfshick@nps.edu>
 * created:  2013-03-18
 */
#include "bulk_extractor.h"

#ifdef HAVE_XMMINTRIN_H
#include<xmmintrin.h>
#endif

using namespace std;

static int USER_SSE_ON = 1;
static uint8_t XOR_MASK = 0xFF;
#ifdef HAVE_XMMINTRIN_H
typedef union {
    __m128 vector;
    uint8_t array[];
} sse_loadable;
static sse_loadable *SSE_XOR_MASK;
#endif

extern "C"
void scan_xor(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::startup) {
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "xor";
	sp.info->author = "Michael Shick";
	sp.info->description = "optimistic XOR deobfuscator";
	sp.info->flags = scanner_info::SCANNER_DISABLED;
        if(be_config["sse"] == "NO") {
            USER_SSE_ON = 0;
        }
        string mask_string = be_config["xor_mask"];
        if(mask_string != "") {
            XOR_MASK = (uint8_t) strtol(mask_string.c_str(), NULL, 16);
        }
#ifdef HAVE_XMMINTRIN_H
        SSE_XOR_MASK = (sse_loadable *) _mm_malloc(16, 16);
        for(size_t ii = 0; ii < 16; ii++) {
            SSE_XOR_MASK->array[ii] = XOR_MASK;
        }
#endif
	return;
    }
    if(sp.phase==scanner_params::scan) {
	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

        // dodge infinite recursion by refusing to operate on an XOR'd buffer
        if(rcb.partName == pos0.lastAddedPart()) {
            return;
        }

        managed_malloc<uint8_t>dbuf(sbuf.bufsize);

        if(!dbuf.buf){
            //ss << "<disposition>calloc-failed</disposition></zipinfo>";
            //zip_recorder->write(pos0+pos,name,ss.str());
            return;
        }
        
        // 0x00 is 8-bit xor identity
        if(XOR_MASK != 0x00) {
#ifdef HAVE_XMMINTRIN_H
            // If SSE is explicitly disabled or if the buffers are
            // unacceptable for SSE, use the old fashioned method
            if(!USER_SSE_ON || (uint64_t) dbuf.buf & 0xF || (uint64_t) sbuf.buf & 0xF || sbuf.bufsize % 16) {
                for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
                    dbuf.buf[ii] = sbuf.buf[ii] ^ XOR_MASK;
                }
            }
            else {
                const __m128 *sse_sbuf = (__m128 *) sbuf.buf;
                __m128 *sse_dbuf = (__m128 *) dbuf.buf;

                for(size_t ii = 0; ii < sbuf.bufsize / 16; ii++) {
                    sse_dbuf[ii] = _mm_xor_ps(sse_sbuf[ii], SSE_XOR_MASK->vector);
                }
            }
#else
            for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
                dbuf.buf[ii] = sbuf.buf[ii] ^ XOR_MASK;
            }
#endif
        }

        const pos0_t pos0_xor = pos0 + rcb.partName;
        const sbuf_t child_sbuf(pos0_xor, dbuf.buf, sbuf.bufsize, sbuf.pagesize, false);
        scanner_params child_params(sp, child_sbuf);
    
        // call scanners on deobfuscated buffer
        (*rcb.callback)(child_params);
        if(rcb.returnAfterFound) {
            return;
        }
    }
}
