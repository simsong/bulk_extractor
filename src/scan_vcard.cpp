/**
 * Plugin: scan_vcard
 * Purpose: The purpose of the plugin is to scan the following from a vCard and extract
 * 	the data:
 * - Names
 * - Emails
 * - Phone
 * - URL
 *
 * This is a very simple algorithm
 * 1 - Find a BEGIN:VCARD
 * 2 - Find a END:VCARD
 * 3 - If everything between them is UTF-8, throw it into a file.
 *
 * Slightly more advanced:
 * 1 - Find a BEGIN:VCARD
 * 2 - Scan a line-at-a-time until we find:
 *     2a - END:VCARD
 *     2b - invalid UTF-8
 *     2c - non-printable
 * 3 - Throw it in a file (except the invalid stuff, of course)
 * 4 - If we didn't write a END:VCARD, add an END:VCARD  (clean it up)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sstream>


#include "config.h"
#include "be13_api/scanner_params.h"
#include "scan_vcard.h"

#include "utf8.h"
void carve_vcards(const sbuf_t &sbuf, feature_recorder &vcard_recorder)
{
    size_t end_len = strlen("END:VCARD\r\n");

    // Search for BEGIN:VCARD\r in the sbuf
    // we could do this with a loop, or with
    for(size_t i = 0;  i < sbuf.bufsize;i++)	{
        ssize_t begin = sbuf.find("BEGIN:VCARD\r",i);
        if(begin==-1) return;		// no more

        /* We found a BEGIN:VCARD\r. Is there an end? */
        ssize_t end = sbuf.find("END:VCARD\r",begin);

        if(end!=-1){
            /* We found a beginning and an ending; verify if what's between them is
             * printable UTF-8.
             */
            bool valid = true;
            for(ssize_t j = begin; j<end && valid; j++){
                if(sbuf[j]<' ' && sbuf[j]!='\r' && sbuf[j]!='\n' && sbuf[j]!='\t'){
                    valid=false;
                }
            }
            /* We should probably validate the UTF-8. */
            if(valid){
                /* got a valid card; I can carve it! */
                vcard_recorder.carve(sbuf.slice(begin,end-begin+end_len), ".vcf", 0);
                i = end+end_len;		// skip to the end of the vcard
                continue;			// loop again!
            }
        }
        /* TODO: vcard is incomplete; carve what we can and then continue */
        /* RIGHT NOW, JUST GIVE UP */
        break;				// stop when we don't have a valid VCARD
    }
}


extern "C"
void scan_vcard(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.info.set_name("vcard");
        sp.info->author         = "Simson Garfinkel and Tony Melaragno";
        sp.info->description    = "Scans for VCARD data";
        sp.info->scanner_version= "1.1";
        sp.info->feature_defs.push_back( feature_recorder_def("vcard"));
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf       = *sp.sbuf;
	feature_recorder &vcard_recorder = sp.named_feature_recorder("vcard");
        carve_vcards(sbuf, vcard_recorder);
    }
}
