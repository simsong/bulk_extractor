/**
 * scan_xml:
 * Extracts text from XML files.
 *
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "config.h"
#include "be13_api/scanner_params.h"

#include "image_process.h"


#define ZLIB_CONST
#ifdef HAVE_DIAGNOSTIC_UNDEF
#  pragma GCC diagnostic ignored "-Wundef"
#endif
#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

static bool pdf_dump = false;

#define SCANNER_NAME "msxml"

/*
 * The problem with trying to extract text from PDF is that sometimes PDF splits actual
 * things that we want, like (exampl) (le@co) (mpany.com).
 * Other times it doesn't, but we don't want to combine because that will
 * break thigs, like (email) (me) (at) (example@company.com).
 *
 * There's no good solution here without rendering the PDF file, and even that doesn't work
 * all the time (witness has poor Adobe's extract text from PDF is.
 *
 * We could do both, but then there would need to be a way to distinguish the mode.
 *
 * So the approach that is used is to scan the entire block and see the largest chunk
 * within (parentheses). If we find spaces within the parentheses, don't add spaces between
 * them, otherwise do.
 *
 * Spaces are always added between arrays [foo].
 * So we just put a space between them all and hope.
 */

extern "C"
void scan_msxml(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT){
        sp.info = new scanner_params::scanner_info( scan_msxml, SCANNER_NAME );
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Extracts text from Microsoft XML files";
        sp.info->scanner_version= "1.0";
        //sp.info->flags          = scanner_info::SCANNER_RECURSE;
        sp.ss.sc.get_config("pdf_dump",&pdf_dump,"Dump the contents of PDF buffers");
        //sp.info = info;
	return;	/* No features recorded */
    }
    if (sp.phase==scanner_params::PHASE_SCAN){

	const sbuf_t &sbuf = *(sp.sbuf);
        if (sbuf.substr(0,6)=="<?xml "){
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
            /* Two copy operations. Ick. */
            std::string  bufstr = ss.str();
            auto *dbuf = sbuf_t::sbuf_malloc(sbuf.pos0+"MSXML", bufstr.size());
            memcpy(bufstr.c_str(), dbuf.malloc_buf(), bufstr.size());
            sp.recurse(dbuf);
        }
    }
}
