/*
 * scan_xor: optimistically search for features trivially obfuscated with xor
 * author:   Michael Shick <mfshick@nps.edu>
 * created:  2013-03-18
 */
#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "utils.h"

static uint8_t xor_mask = 255;
extern "C"
void scan_xor(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP) {
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "xor";
	sp.info->author = "Michael Shick";
	sp.info->description = "optimistic XOR deobfuscator";
	sp.info->flags = scanner_info::SCANNER_DISABLED | scanner_info::SCANNER_RECURSE;
        sp.info->get_config("xor_mask",&xor_mask,"XOR mask value, in decimal");
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN) {
	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

        if(xor_mask == 0x00){           // this would do nothing
            return;
        }

        // dodge infinite recursion by refusing to operate on an XOR'd buffer
        if(rcb.partName == pos0.lastAddedPart()) {
            return;
        }

        // dodge running after unzip after self
        std::vector<std::string> parts = split(pos0.str(),'-');
        if(parts.size()>4){
            std::string parent = parts.at(parts.size()-2);
            std::string grandp = parts.at(parts.size()-4);
            if(parent.find("ZIP") != std::string::npos &&
               grandp == rcb.partName){
                return;
            }
        }

        // managed_malloc throws an exception if allocation fails.
        managed_malloc<uint8_t>dbuf(sbuf.bufsize);
        for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
            dbuf.buf[ii] = sbuf.buf[ii] ^ xor_mask;
        }
        
        std::stringstream ss;
        ss << "XOR(" << uint32_t(xor_mask) << ")";
        
        const pos0_t pos0_xor = pos0 + ss.str();
        const sbuf_t child_sbuf(pos0_xor, dbuf.buf, sbuf.bufsize, sbuf.pagesize, false);
        scanner_params child_params(sp, child_sbuf);
        (*rcb.callback)(child_params);    // recurse on deobfuscated buffer
    }
}
