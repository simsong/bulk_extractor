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
#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sstream>

#include "utf8.h"

using namespace std;

extern "C"
void scan_vcard(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "vcard";
        sp.info->author         = "Simson Garfinkel and Tony Melaragno";
        sp.info->description    = "Scans for VCARD data";
        sp.info->scanner_version= "1.0";
	sp.info->feature_names.insert("vcard");
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = sp.sbuf;
	feature_recorder_set &fs = sp.fs;
	feature_recorder *vcard_recorder = fs.get_name("vcard");
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
		    vcard_recorder->carve(sbuf,begin,(end-begin)+end_len,".vcf");
		    i = end+end_len;		// skip to the end of the vcard
		    continue;			// loop again!
		}
	    } 
	    /* vcard is incomplete; carve what we can and then continue */
	    /* RIGHT NOW, JUST GIVE UP */
	    break;				// stop when we don't have a valid VCARD
	}
    }
}
