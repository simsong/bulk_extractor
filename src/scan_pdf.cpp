/**
 * scan_pdf:
 * Extracts text from PDF files by decompressing streams and extracting text between parentheses.
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
 
static void pdf_extract_text(std::string &tbuf,const unsigned char *buf,size_t bufsize)
{
    int maxwordsize = 0;
    bool words_have_spaces = false;
    for(int pass=0;pass<2;pass++){
        /* pass = 0 --- analysis. Find maxwordsize
         * pass = 1 --- creation.
         */
        bool in_paren = false;
        int  wordsize = 0;
        for(const unsigned char *cc = buf;cc<buf+bufsize;cc++){
            if(in_paren==false && *cc=='[') {
                /* Beginning of bracket group not in paren; ignore */
                continue;
            }
            if(in_paren==false && *cc==']') {
                /* End of bracket group not in paren; ignore */
                continue;
            }
            if(in_paren==false && *cc=='(') {
                /* beginning of word */
                wordsize = 0;
                in_paren = true;
                continue;
            }
            if(in_paren==true &&  *cc==')') {
                /* end of word */
                in_paren = false;
                if(pass==0 && (wordsize > maxwordsize))  maxwordsize = wordsize;
                if(pass==1 && (words_have_spaces==false)){
                    /* Second pass; words don't have spaces, so add spaces between the parens */
                    tbuf.push_back(' ');
                }
                continue;
            }
            if(in_paren){
                /* in a word */
                if(*cc==' ') words_have_spaces = true;
                if(pass==1) tbuf.push_back(*cc);
                wordsize+=1;
            }
        }
    }
}

inline int analyze_stream(const class scanner_params &sp,const recursion_control_block &rcb,
                          size_t stream_tag,size_t stream_start,size_t endstream)
{
    const sbuf_t &sbuf = sp.sbuf;
    size_t compr_size = endstream-stream_start;
    size_t uncompr_size = compr_size * 8;       // good assumption for expansion
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
                sbuf_t dbuf(sbuf.pos0 + "-PDFDECOMP",
                            decomp.buf,zs.total_out,zs.total_out,0,
                            false,false,false);
                if(pdf_dump){
                    std::cout << "====== " << dbuf.pos0 << "=====\n";
                    dbuf.hex_dump(std::cout);
                    std::cout << "\n";
                }
                if(mostly_printable_ascii(decomp.buf,zs.total_out)){
                    std::string text;
                    pdf_extract_text(text,decomp.buf,zs.total_out);
                    if(text.size()>0){
                        pos0_t pos0_pdf    = (sbuf.pos0 + stream_tag) + rcb.partName;
                        const  sbuf_t sbuf_new(pos0_pdf, reinterpret_cast<const uint8_t *>(&text[0]),
                                               text.size(),text.size(),false);
                        (*rcb.callback)(scanner_params(sp,sbuf_new));
                    }
                    if(pdf_dump) std::cout << "Extracted Text:\n" << text << "\n";
                }
                if(pdf_dump){
                    std::cout << "================\n";
                }
            }
            inflateEnd(&zs);            // prevent leak; still not exception safe.
        }
    }
    return 0;
}


extern "C"
void scan_pdf(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name  = "pdf";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Extracts text from PDF files";
        sp.info->scanner_version= "1.0";
        sp.info->flags          = scanner_info::SCANNER_RECURSE;
        sp.info->get_config("pdf_dump",&pdf_dump,"Dump the contents of PDF buffers");
	return;	/* No features recorded */
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN){

	const sbuf_t &sbuf = sp.sbuf;

	/* Look for signature for the beginning of a PDF stream */
	for(size_t loc=0;loc+15<sbuf.pagesize;loc++){
	    ssize_t stream_tag = sbuf.find("stream",loc);
	    if(stream_tag==-1) break;   
	    /* Now skip past the \r or \r\n or \n */
	    size_t stream_start = stream_tag+6;
	    if(sbuf[stream_start]=='\r' && sbuf[stream_start+1]=='\n') stream_start+=2;
	    else stream_start +=1;

	    /* See if we can find the endstream; here we can scan to the end of the buffer.
	     * Also, make sure that the endstream comes before the next stream. This is easily
	     * determined by doing a search for 'stream' and 'endstream' and making sure that
	     * the next 'stream' we find is, in fact, in the 'endsream'.
	     */
	    ssize_t endstream = sbuf.find("endstream",stream_start);
	    if(endstream==-1) break;    // no endstream tag

	    ssize_t nextstream = sbuf.find("stream",stream_start);

	    if(endstream+3!=nextstream){
                /* The 'stream' after the stream_tag is not the 'endstream',
                 * so advance loc so that it will find the nextstream
                 */
                loc = nextstream - 1;
                continue;
            }
            if(analyze_stream(sp,rcb,stream_tag,stream_start,endstream)==-1){
                return;
            }
            loc=endstream+9;
	}
    }
}

