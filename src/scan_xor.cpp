/*
 * scan_xor: optimistically search for features trivially obfuscated with xor
 * author:   Michael Shick <mfshick@nps.edu>
 * created:  2013-03-18
 */
#include "bulk_extractor.h"

using namespace std;

static uint8_t XOR_MASK = 0xFF;

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

        string mask_string = be_config["xor_mask"];
        if(mask_string != "") {
            XOR_MASK = (uint8_t) strtol(mask_string.c_str(), NULL, 16);
        }
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
            for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
                dbuf.buf[ii] = sbuf.buf[ii] ^ XOR_MASK;
            }
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
