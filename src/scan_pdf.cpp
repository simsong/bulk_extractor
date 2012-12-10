#include "bulk_extractor.h"
#include "image_process.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <cassert>

#define ZLIB_CONST
#ifdef GNUC_HAS_DIAGNOSTIC_PRAGMA
#  pragma GCC diagnostic ignored "-Wundef"
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

using namespace std;

/*
 * Return TRUE if most of the characters (90%) are printable ASCII.
 */

static bool mostly_printable_ascii(const unsigned char *buf,size_t bufsize)
{
    size_t count = 0;
    for(const unsigned char *cc = buf; cc<buf+bufsize;cc++){
	if(isprint(*cc) || isspace(*cc)) count++;
    }
    return count > (bufsize*9/10);
}

/*
 * The problem with trying to extract text from PDF is that sometimes PDF splits actual
 * things that we want, like (exampl) (le@co) (mpany.com).
 * Other times it doesn't, but we don't want to combine because that will
 * break thigs, like (email) (me) (at) (example@company.com).
 * There's no good solution here without rendering the PDF file, and even that doesn't work
 * all the time (witness has poor Adobe's extract text from PDF is.
 *
 * So we just put a space between them all and hope.
 */
 
static void pdf_extract_text(std::string &tbuf,const unsigned char *buf,size_t bufsize)
{
    bool in = false;
    for(const unsigned char *cc = buf;cc<buf+bufsize;cc++){
	switch(*cc){
	case '(':
	    in = true;
	    break;
	case ')':
	    in = false;
	    tbuf.push_back(' ');
	    break;
	default:
	    if(in) tbuf.push_back(*cc);
	}
    }
}

extern "C"
void scan_pdf(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name  = "pdf";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Extracts text from PDF files";
        sp.info->scanner_version= "1.0";
	return;	/* No features recorded */
    }
    if(sp.phase==scanner_params::shutdown) return;
    if(sp.phase==scanner_params::scan){

	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

	/* Look for signature for the beginning of a PDF stream */
	for(size_t loc=0;loc+15<sbuf.pagesize;loc++){
	    ssize_t stream = sbuf.find("stream",loc);
	    if(stream==-1) break;
	    /* Now skip past the \r or \r\n or \n */
	    size_t stream_start = stream+6;
	    if(sbuf[stream_start]=='\r' && sbuf[stream_start+1]=='\n') stream_start+=2;
	    else stream_start +=1;

	    /* See if we can find the endstream; here we can scan to the end of the buffer.
	     * Also, make sure that the endstream comes before the next stream. This is easily
	     * determined by doing a search for 'stream' and 'endstream' and making sure that
	     * the next 'stream' we find is, in fact, in the 'endsream'.
	     */
	    ssize_t endstream = sbuf.find("endstream",stream_start);
	    if(endstream==-1) break;

	    ssize_t nextstream = sbuf.find("stream",stream_start);
	    if(endstream+3==nextstream){           
		size_t compr_size = endstream-stream_start;
		size_t uncompr_size = compr_size * 8; // good assumption for expansion
		managed_malloc<Bytef>decomp(uncompr_size);
		if(decomp.buf){
		    z_stream zs;
		    memset(&zs,0,sizeof(zs));
		    zs.next_in = (Bytef *)sbuf.buf+stream_start;
		    zs.avail_in = compr_size;
		    zs.next_out = decomp.buf;
		    zs.avail_out = uncompr_size;
		    int r = inflateInit(&zs);
		    if(r==Z_OK){
			r = inflate(&zs,Z_FINISH);
			if(zs.total_out>0){
			    if(mostly_printable_ascii(decomp.buf,zs.total_out)){
				std::string text;
				pdf_extract_text(text,decomp.buf,zs.total_out);
				if(text.size()>0){
				    pos0_t pos0_pdf    = (pos0 + stream) + rcb.partName;
				    const  sbuf_t sbuf_new(pos0_pdf, reinterpret_cast<const uint8_t *>(&text[0]),
							   text.size(),text.size(),false);
				    (*rcb.callback)(scanner_params(sp,sbuf_new));
				    if(rcb.returnAfterFound){
					return;
				    }
				}
			    }
			}
		    }
		}
	    }
	    loc=endstream+9;
	}
    }
}

