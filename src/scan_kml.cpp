/**
 * kml scanner.
 * theory of operation:
 * 1. Find a <?xml header.
 * 2. find a <kml field after the <?xml
 * 3. Find a </kml> after the <kml
 * 4. Verify that from <?xml to <?kml> is UTF-8 (question: can KML be other than UTF-8?)
 * 5. Carve it! Move on
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
void scan_kml(const class scanner_params &sp,const recursion_control_block &rcb)
{
    string myString;
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "kml";
        sp.info->author         = "Simson Garfinkel ";
        sp.info->description    = "Scans for KML files";
        sp.info->scanner_version= "1.0";
	sp.info->feature_names.insert("kml");
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = sp.sbuf;
	feature_recorder_set &fs = sp.fs;
	feature_recorder *kml_recorder = fs.get_name("kml");

	// Search for <xml BEGIN:VCARD\r in the sbuf
	// we could do this with a loop, or with 
	for(size_t i = 0;  i < sbuf.bufsize;)	{
	    ssize_t xml_loc = sbuf.find("<?xml ",i);
	    if(xml_loc==-1) return;		// no more
	    ssize_t kml_loc = sbuf.find("<kml ",xml_loc);
	    if(kml_loc==-1) return;
	    ssize_t ekml_loc = sbuf.find("</kml>",kml_loc);
	    if(ekml_loc==-1) return;
	    ssize_t kml_len = (ekml_loc-xml_loc)+6;

	    /* verify the utf-8 */
	    std::string possible_kml = sbuf.substr(xml_loc,kml_len);
	    if(utf8::find_invalid(possible_kml.begin(),possible_kml.end()) == possible_kml.end()){
		/* No invalid UTF-8 */
		kml_recorder->carve(sbuf,xml_loc,kml_len,".kml");
		i = ekml_loc + 6;	// skip past end of </kml>
	    }
	    else {
		i = xml_loc + 6;	// skip past <?xml ; there may be another with no errors
	    }
	}
    }
}
