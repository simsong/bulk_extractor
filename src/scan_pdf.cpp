/**
 * scan_pdf:
 * Extracts text from PDF files by decompressing streams and extracting text between parentheses.
 * Currently this is dead-simple. It should be rewritten to position the text on an (x,y) grid and find the words.
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "config.h"

#include "sbuf_decompress.h"
#include "be13_api/scanner_params.h"
#include "image_process.h"

/* Debug by setting DEBUG or by setting pdf_dump at runtime */

static bool pdf_dump = false;

/*
 * Return TRUE if most of the characters (90%) are printable ASCII.
 */

static bool mostly_printable_ascii(const sbuf_t &sbuf)
{
    size_t count = 0;
    for(int i=0;i<sbuf.pagesize;i++){
        if (isprint(sbuf[i]) || isspace(sbuf[i])) count++;
    }
    return count > (sbuf.pagesize);
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

static std::string  pdf_extract_text(const sbuf_t &sbuf)
{
    std::string tbuf {};
    int maxwordsize = 0;
    bool words_have_spaces = false;
    /* pass = 0 --- analysis. Find maxwordsize
     * pass = 1 --- creation.
     */
    for(int pass=0;pass<2;pass++){
        bool in_paren = false;
        int  wordsize = 0;
        for (int i=0;i<sbuf.pagesize;i++){
            const unsigned char cc = sbuf[i];
            if(in_paren==false && cc=='[') {
                /* Beginning of bracket group not in paren; ignore */
                continue;
            }
            if(in_paren==false && cc==']') {
                /* End of bracket group not in paren; ignore */
                continue;
            }
            if(in_paren==false && cc=='(') {
                /* beginning of word */
                wordsize = 0;
                in_paren = true;
                continue;
            }
            if(in_paren==true &&  cc==')') {
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
                if(cc==' ') words_have_spaces = true;
                if(pass==1) tbuf.push_back(cc);
                wordsize+=1;
            }
        }
    }
    return tbuf;
}

void analyze_stream(const scanner_params &sp, size_t stream_tag,size_t stream_start,size_t endstream)
{
    const sbuf_t &sbuf = (*sp.sbuf);
    size_t compr_size = endstream-stream_start;
    size_t max_uncompr_size = compr_size * 8;       // good assumption for expansion

    auto *dbuf = sbuf_decompress_zlib_new( sbuf.slice(stream_start, compr_size), max_uncompr_size, "PDFDECOMP");
    if (dbuf==nullptr) return;

    if(pdf_dump){
        std::cout << "====== " << dbuf->pos0 << "=====\n";
        dbuf->hex_dump(std::cout);
        std::cout << "\n";
    }
    if (mostly_printable_ascii(*dbuf)){
        std::string text = pdf_extract_text(text,decomp.buf,zs.total_out);
        if(text.size()>0){
            if (pdf_dump) std::cout << "Extracted Text:\n" << text << "================\n";
            pos0_t pos0_pdf    = (sbuf.pos0 + stream_tag) + "PDF";//rcb.partName;
            //const  sbuf_t sbuf_new(pos0_pdf, reinterpret_cast<const uint8_t *>(&text[0]), text.size(),text.size(),0, false);
            //(*rcb.callback)(scanner_params(sp,sbuf_new));
            auto *nsbuf = new sbuf_t(pos0_pdf, text);
            sp.recurse(nsbuf);
        }
    }
    return 0;
}


extern "C"
void scan_pdf(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        auto info = new scanner_params::scanner_info( scan_pdf, "pdf" );
        info->author         = "Simson Garfinkel";
        info->description    = "Extracts text from PDF files";
        info->scanner_version= "1.0";
        sp.ss.sc.get_config("pdf_dump",&pdf_dump,"Dump the contents of PDF buffers");
        sp.info = info;
	return;	/* No features recorded */
    }
    if(sp.phase==scanner_params::PHASE_SCAN){

#ifdef DEBUG
        pdf_dump = true;
#endif
	const sbuf_t &sbuf = (*sp.sbuf);

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
            if(analyze_stream(sp,stream_tag,stream_start,endstream) == -1){
                return;
            }
            loc=endstream+9;
	}
    }
}
