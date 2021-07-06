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

#include "scan_pdf.h"
#include "sbuf_decompress.h"
#include "be13_api/scanner_params.h"
#include "image_process.h"



/* Debug by setting DEBUG or by setting pdf_dump at runtime */

bool pdf_extractor::pdf_dump_hex  = false;       // dump the contents HEX
bool pdf_extractor::pdf_dump_text = false;      // dump the extracted text.

pdf_extractor::pdf_extractor(const sbuf_t &sbuf_):
    sbuf(sbuf_)
{
}

pdf_extractor::~pdf_extractor()
{
    streams.clear();     // may not be necessary
    texts.clear();       // may not be necessary
}

/*
 * Return TRUE if most of the characters (90%) are printable ASCII.
 */

bool pdf_extractor::mostly_printable_ascii(const sbuf_t &s)
{
    size_t count = 0;
    for(u_int i=0; i<s.pagesize; i++){
        if (isprint(s[i]) || isspace(s[i])) count++;
    }
    return count > (s.pagesize * 9 / 10);
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

/*
 * Extract the text from the decompressed sbuf and return it.
 */
std::string  pdf_extractor::extract_text(const sbuf_t &sb)
{
    std::string tbuf {};
    int maxwordsize = 0;
    bool words_have_spaces = false;
    /* pass = 0 --- analysis. Find maxwordsize
     * pass = 1 --- creation.
     */
    for (u_int pass=0;pass<2;pass++){
        bool in_paren = false;
        int  wordsize = 0;
        for (u_int i=0;i<sb.pagesize;i++){
            const unsigned char cc = sb[i];
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

void pdf_extractor::recurse_texts(scanner_params &sp)
{
    for (const auto &it: texts) {
        if(it.txt.size()>0){
            if (pdf_dump_text){
                std::cout << "====== " << it.pos0 << " TEXT =====\n";
                std::cout << it.txt << "\n";
            }
            auto *nsbuf = sbuf_t::sbuf_new( (it.pos0) + "PDF", it.txt);
            sp.recurse(nsbuf);
        }
    }
}

void pdf_extractor::decompress_streams_extract_text()
{
    // note: below we do *not* use a const auto,
    // because we want to modify the contents of the vector
    for (const auto &it: streams) {           //
        size_t compr_size = it.endstream - it.stream_start;
        size_t max_uncompr_size = compr_size * 8;       // good assumption for expansion

        auto *dbuf = sbuf_decompress_zlib_new( sbuf.slice(it.stream_start, compr_size), max_uncompr_size, "PDFZLIB");
        if (dbuf==nullptr) {
            continue ;   // could not decompress
        }

        if (pdf_dump_hex){
            std::cout << "====== " << dbuf->pos0 << " HEX =====\n";
            dbuf->hex_dump(std::cout);
            std::cout << "mostly printable: " << (mostly_printable_ascii(*dbuf) ? "true" : "false") << "\n";
            std::cout << "\n";
        }
        if (mostly_printable_ascii(*dbuf)){
            texts.push_back(text(pos0_t(dbuf->pos0), extract_text( *dbuf )));
        }
        delete dbuf;
    }
}


void pdf_extractor::find_streams()
{
    /* Look for signature for the beginning of a PDF stream and record the start and end of each */
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
        if (endstream==-1) break;    // no endstream tag

        ssize_t nextstream = sbuf.find("stream",stream_start);

        if (endstream+3 != nextstream){
            /* The 'stream' after the stream_tag is not the 'endstream',
             * so advance loc so that it will find the nextstream
             */
            loc = nextstream - 1;
            continue;
        }
        /* Remember the stream to analyze later */

        /* Analyze this stream and then see if there is another one following!
         * This is a change from BE1.5, which stopped after it found the first stream.
         */
        streams.push_back( stream( stream_start, endstream ));
        loc=endstream+9;
    }
}

void pdf_extractor::run(scanner_params &sp)
{
    find_streams();
    decompress_streams_extract_text();
    recurse_texts(sp);
}

extern "C"
void scan_pdf(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.info = new scanner_params::scanner_info( scan_pdf, "pdf" );
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Extracts text from PDF files";
        sp.info->scanner_version= "1.0";
        sp.info->scanner_flags.recurse = true;
        sp.ss.sc.get_config("pdf_dump_hex" , &pdf_extractor::pdf_dump_hex, "Dump the contents of PDF buffers as hex");
        sp.ss.sc.get_config("pdf_dump_text", &pdf_extractor::pdf_dump_text, "Dump the contents of PDF buffers showing extracted text");
        if (getenv("DEBUG_PDF_DUMP_HEX")) pdf_extractor::pdf_dump_hex=true;
        if (getenv("DEBUG_PDF_DUMP_TEXT")) pdf_extractor::pdf_dump_text=true;
	return;	/* No features recorded */
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        pdf_extractor ex(*sp.sbuf);
        ex.run(sp);
    }
}
