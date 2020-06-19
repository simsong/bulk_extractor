/*
 * scan_outlook: optimistically decrypt Outlook Compressible Encryption
 * author:   Simson L. Garfinkel
 * created:  2014-01-28
 */
#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "scan_outlook.h"
#include "utils.h"

extern "C"
void scan_outlook(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP) {
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "outlook";
	sp.info->author = "Simson L. Garfinkel";
	sp.info->description = "Outlook Compressible Encryption";
	sp.info->flags = scanner_info::SCANNER_DISABLED \
            | scanner_info::SCANNER_RECURSE | scanner_info::SCANNER_DEPTH_0 ;
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN) {
	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

        // dodge infinite recursion by refusing to operate on an OFE'd buffer
        if(rcb.partName == pos0.lastAddedPart()) {
            return;
        }

        // managed_malloc throws an exception if allocation fails.
        managed_malloc<uint8_t>dbuf(sbuf.bufsize);
        for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
            uint8_t ch = sbuf.buf[ii];
            dbuf.buf[ii] = libpff_encryption_compressible[ ch ];
        }
        
        const pos0_t pos0_oce = pos0 + "OUTLOOK";
        const sbuf_t child_sbuf(pos0_oce, dbuf.buf, sbuf.bufsize, sbuf.pagesize, false);
        scanner_params child_params(sp, child_sbuf);
        (*rcb.callback)(child_params);    // recurse on deobfuscated buffer
    }
}
