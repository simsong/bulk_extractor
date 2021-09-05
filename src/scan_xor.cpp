/*
 * scan_xor: optimistically search for features trivially obfuscated with xor
 * author:   Michael Shick <mfshick@nps.edu>
 * created:  2013-03-18
 */
#include "config.h"
#include "be13_api/scanner_params.h"
#include "be13_api/utils.h"

static uint8_t xor_mask = 255;
extern "C"
void scan_xor(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT) {
        sp.info->set_name("xor" );
	sp.info->author      = "Michael Shick";
	sp.info->description = "optimistic XOR deobfuscator";
	sp.info->scanner_flags.default_enabled = false;
        sp.info->scanner_flags.recurse = true;
        sp.info->scanner_flags.recurse_always = true;
        sp.get_scanner_config("xor_mask",&xor_mask,"XOR mask value, in decimal");
	return;
    }
    if (sp.phase==scanner_params::PHASE_SCAN) {
	const sbuf_t &sbuf = (*sp.sbuf);
	const pos0_t &pos0 = sbuf.pos0;

        if (xor_mask == 0x00){           // this would do nothing
            return;
        }

        // dodge infinite recursion by refusing to operate on an XOR'd buffer
        if (pos0.lastAddedPart() == "XOR" ) {
            return;
        }

        // dodge running after unzip after self
        std::vector<std::string> parts = split(pos0.str(),'-');
        if (parts.size()>4){
            std::string parent = parts.at(parts.size()-2);
            std::string grandp = parts.at(parts.size()-4);
            if (parent.find("ZIP") != std::string::npos && grandp == "XOR"){
                return;
            }
        }

        std::stringstream ss;
        ss << "XOR(" << uint32_t(xor_mask) << ")";
        const pos0_t pos0_xor = pos0 + ss.str();

        // managed_malloc throws an exception if allocation fails.
        auto *dbuf = sbuf_t::sbuf_malloc(pos0_xor, sbuf.bufsize, sbuf.pagesize);
        assert( dbuf!= nullptr);
        if (sbuf.depth()+1 != dbuf->depth()) {
            std::cerr << "sbuf: " << sbuf << "\n";
            std::cerr << "dbuf: " << *dbuf << "\n";
        }
        assert( sbuf.depth() +1 == dbuf->depth());

        for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
            dbuf->wbuf(ii, sbuf[ii] ^ xor_mask);
        }
        sp.recurse(dbuf);
    }
}
