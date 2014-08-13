/**
 * scan_xml:
 * Extracts text from XML files.
 *  
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "image_process.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <cassert>

#define ZLIB_CONST
#ifdef HAVE_DIAGNOSTIC_UNDEF
#  pragma GCC diagnostic ignored "-Wundef"
#endif
#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

using namespace std;
static bool pdf_dump = false;

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
void scan_msxml(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name           = "msxml";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Extracts text from Microsoft XML files";
        sp.info->scanner_version= "1.0";
        sp.info->flags          = scanner_info::SCANNER_RECURSE;
        sp.info->get_config("pdf_dump",&pdf_dump,"Dump the contents of PDF buffers");
	return;	/* No features recorded */
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN){

	const sbuf_t &sbuf = sp.sbuf;
        if (sbuf.substr(0,6)=="<?xml "){
            /* copy out the data to a new buffer. The < character turns off copying the > character turns it on */
            std::stringstream ss;
            bool instring = false;
            for(size_t i=0;i<sbuf.bufsize;i++){
                switch(sbuf[i]){
                case '<':
                    instring=false;
                    if(sbuf.substr(i,6)=="</w:p>"){
                        ss << "\n";
                    }
                    break;
                case '>':
                    instring=true;
                    break;
                default:
                    if(instring) ss << sbuf[i];
                }
            }
            std::string  bufstr = ss.str();
            size_t       buflen = bufstr.size();
            managed_malloc<char *>buf(buflen);
            if(buf.buf){
                memcpy(buf.buf,bufstr.c_str(),buflen);
                pos0_t pos0_xml    = sbuf.pos0 + rcb.partName;
                const  sbuf_t sbuf_new(pos0_xml,reinterpret_cast<const u_char *>(buf.buf),buflen,buflen,false);
                (*rcb.callback)(scanner_params(sp,sbuf_new));
            }
        }
    }
}

