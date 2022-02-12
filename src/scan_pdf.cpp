/**
 * scan_pdf:
 * Extracts text from PDF files by decompressing streams and extracting text between parentheses.
 * Currently this is dead-simple.
 * It should be rewritten to position the text on an (x,y) grid and find the words.
 *
 * Other ideas for exploring PDF streams:
 * https://stackoverflow.com/questions/15058207/pdftk-will-not-decompress-data-streams
 * https://superuser.com/questions/264695/how-can-i-deflate-compressed-streams-inside-a-pdf
 *
 * Originally developed by Simson Garfinkel, 2012-2014.
 * (C) 2021 Simson L. Garfinkel.
 * MIT License, see ../LICENSE.md
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "config.h"

#include "scan_pdf.h"
#include "sbuf_decompress.h"
#include "be20_api/scanner_params.h"
#include "image_process.h"



/* Debug by setting DEBUG or by setting pdf_dump at runtime */

bool pdf_extractor::pdf_dump_hex  = false;       // dump the contents HEX
bool pdf_extractor::pdf_dump_text = false;      // dump the extracted text.

pdf_extractor::pdf_extractor(const sbuf_t &sbuf):
    sbuf_root(sbuf)
{
}

pdf_extractor::~pdf_extractor()
{
    streams.clear();     // clean, but may not be necessary
    texts.clear();       // clean, but may not be necessary
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
 * Two part algorithm. First we analyze the sbuf to see the PDF encoding style, then we extract the text.
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


/* Look for signature for the beginning of a PDF stream and record the start and end of each
 *
 */

void pdf_extractor::find_streams()
{
    //std::cerr << "sbuf_root: " << sbuf_root << "\n";
    for(size_t loc=0; loc+15 < sbuf_root.pagesize; loc++){
        size_t stream_tag = sbuf_root.find("stream",loc);
        //std::cerr << "stream_tag: " << stream_tag << "\n";
        if (stream_tag==std::string::npos) break; // no more 'stream' tags
        /* Now skip past the \r or \r\n or \n */
        size_t stream_start = stream_tag+6;
        if (sbuf_root[stream_start]=='\r' && sbuf_root[stream_start+1]=='\n') stream_start+=2;
        else stream_start +=1;

        /* See if we can find the endstream; here we can scan to the end of the buffer.
         * Also, make sure that the endstream comes before the next stream. This is easily
         * determined by doing a search for 'stream' and 'endstream' and making sure that
         * the next 'stream' we find is, in fact, in the 'endsream'.
         */
        size_t endstream_tag = sbuf_root.find("endstream",stream_start);
        size_t next_stream_tag = sbuf_root.find("stream",stream_start);

        if (endstream_tag==std::string::npos) break;    // no endstream tag
        if (next_stream_tag!=std::string::npos && endstream_tag +3 != next_stream_tag){
            /* The 'stream' after the stream_tag is not the 'endstream',
             * so advance loc so that it will find the nextstream
             */
            loc = next_stream_tag - 1;
            continue;
        }
        /* Remember the stream to analyze later */
        streams.push_back( stream( stream_tag, stream_start, endstream_tag ));
        loc = endstream_tag + 9;
    }
    //std::cerr << "streams found: " << streams.size() << "\n";
}

void pdf_extractor::decompress_streams_extract_text()
{
    // note: below we do *not* use a const auto,
    // because we want to modify the contents of the vector
    for (const auto &it: streams) {           //
        size_t compr_size = it.endstream_tag - it.stream_start;
        size_t max_uncompr_size = compr_size * 8;       // good assumption for expansion

        auto *dbuf = sbuf_decompress::sbuf_new_decompress( sbuf_root.slice(it.stream_start, compr_size), max_uncompr_size, "PDFZLIB",
                                                           sbuf_decompress::mode_t::PDF, 0 );
        if (dbuf==nullptr) {
            continue ;   // could not decompress
        }

        if (pdf_dump_hex){
            std::cout << "===== scan_pdf.c:decompress_streams_extract_text: dbuf->pos0 = " << dbuf->pos0 << " =====\n";
            dbuf->hex_dump(std::cout);
            std::cout << "mostly printable: " << (mostly_printable_ascii(*dbuf) ? "true" : "false") << "\n";
            std::cout << "---dbuf end---\n";
        }

        if (mostly_printable_ascii(*dbuf)){
            pos0_t pos0 = (sbuf_root.pos0 + it.stream_tag) + "PDF";
            std::string the_text = extract_text( *dbuf );

            texts.push_back( text(pos0, the_text) );
        }
        delete dbuf;
    }
}
/*
 * For all of the texts that have been found, recruse on each.
 */
void pdf_extractor::recurse_texts(scanner_params &sp)
{
    //std::cerr << "pdf_extractor::recurse_texts\n";
    for (const auto &it: texts) {
        auto lt = it.txt;
        if (lt.size()>0){
            if (pdf_dump_text){
                std::cout << "====== pdf_extractor::recurse_texts: " << it.pos0 << "  =====\n";
                std::cout << lt << "\n";
            }
            auto *nsbuf = sbuf_t::sbuf_malloc( it.pos0, lt);
            //std::cerr << "just made nsbuf:\n" << *nsbuf << "\n";
            //nsbuf->hex_dump(std::cerr);
            sp.recurse(nsbuf);          // it will delete the sbuf
            //std::cerr << "----------------- back from recurse (scan_pdf) -----------------\n";
        }
    }
}

void pdf_extractor::run(scanner_params &sp)
{
    find_streams();
    if (streams.size()) decompress_streams_extract_text();
    if (texts.size()) recurse_texts(sp);
}

extern "C"
void scan_pdf(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.info->set_name("pdf" );
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Extracts text from PDF files";
        sp.info->scanner_version= "1.0";
        sp.info->scanner_flags.recurse = true;
        sp.get_scanner_config("pdf_dump_hex" , &pdf_extractor::pdf_dump_hex, "Dump the contents of PDF buffers as hex");
        sp.get_scanner_config("pdf_dump_text", &pdf_extractor::pdf_dump_text, "Dump the contents of PDF buffers showing extracted text");
        if (getenv("DEBUG_PDF_DUMP_HEX")) pdf_extractor::pdf_dump_hex=true;
        if (getenv("DEBUG_PDF_DUMP_TEXT")) pdf_extractor::pdf_dump_text=true;
	return;	/* No features recorded */
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        pdf_extractor ex(*sp.sbuf);
        ex.run(sp);
    }
}
