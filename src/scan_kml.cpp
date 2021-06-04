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
#include "be13_api/scanner_params.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sstream>


#include "utf8.h"

extern "C"
void scan_kml(scanner_params &sp)
{
    std::string myString;
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        auto info = new scanner_params::scanner_info(scan_kml,"kml");
	info->name		= "kml";
        info->author         = "Simson Garfinkel ";
        info->description    = "Scans for KML files";
        info->scanner_version= "1.0";
        info->feature_defs.push_back( feature_recorder_def("kml"));
        sp.register_info(info);
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = *(sp.sbuf);
	feature_recorder &kml_recorder = sp.ss.named_feature_recorder("kml");

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
	    std::string possible_kml = sbuf.substr(xml_loc, kml_len);
	    if(utf8::find_invalid(possible_kml.begin(),possible_kml.end()) == possible_kml.end()){
		/* No invalid UTF-8 */
		kml_recorder.carve_data(sbuf, xml_loc, kml_len, ".kml");
		i = ekml_loc + 6;	// skip past end of </kml>
	    }
	    else {
		i = xml_loc + 6;	// skip past <?xml ; there may be another with no errors
	    }
	}
    }
}
