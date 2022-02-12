/**
 * scan_xml:
 * Extracts text from XML files and recursively re-analyzes. Never records.
 *
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "config.h"
#include "be20_api/scanner_params.h"

#include "image_process.h"
#include "scan_msxml.h"


#define ZLIB_CONST
#ifdef HAVE_DIAGNOSTIC_UNDEF
#  pragma GCC diagnostic ignored "-Wundef"
#endif
#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

#define SCANNER_NAME "msxml"

std::string msxml_extract_text(const sbuf_t &sbuf)
{
    /* copy out the data to a new buffer. The < character turns off copying the > character turns it on */
    std::stringstream ss;
    bool instring = false;
    for(size_t i=0;i<sbuf.bufsize;i++){
        switch(sbuf[i]){
        case '<':
            instring=false;
            if (sbuf.substr(i,6)=="</w:p>"){
                ss << "\n";
            }
            break;
        case '>':
            instring=true;
            break;
        default:
            if (instring) ss << sbuf[i];
        }
    }
    return ss.str();
}

extern "C"
void scan_msxml(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT){
        sp.info->set_name("msxml");
        sp.info->description     = "Extracts text from Microsoft XML files";
        sp.info->scanner_version = "1.0";
        sp.info->scanner_flags.recurse = true;
	return;
    }
    if (sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = *(sp.sbuf);
        if (sbuf.substr(0,6)=="<?xml "){
            std::string bufstr = msxml_extract_text(sbuf);
            auto *dbuf = sbuf_t::sbuf_malloc(sbuf.pos0+"MSXML", bufstr.size(), bufstr.size());
            memcpy(dbuf->malloc_buf(), bufstr.c_str(), bufstr.size());
            sp.recurse(dbuf);           // will delete dbuf
        }
    }
}
