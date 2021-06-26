#ifndef SCAN_PDF_H
#define SCAN_PDF_H

#include <vector>
#include <memory>
#include "be13_api/scanner_params.h"

/*
 * The revised PDF extractor is designed to be testable outside of the bulk_extractor framework.
 */
class pdf_extractor {
    struct stream {
        stream(ssize_t a, ssize_t b):
            stream_start(a),
            endstream(b) {}
        ssize_t stream_start {0};
        ssize_t endstream {0};
        pos0_t *pos0 {nullptr};                   // where dbuf->pos0 was (after dbuf is deleted )
        std::string text {};                      // extracted text from the stream
        stream & operator =(const stream &)=delete;
    };

public:
    static bool pdf_dump_hex;
    static bool pdf_dump_text;

    pdf_extractor(const sbuf_t &sbuf_);
    ~pdf_extractor();

    std::vector<stream> streams {};        // streams we found
    const sbuf_t &sbuf;                 // analyzed sbuf

    static bool mostly_printable_ascii(const sbuf_t &sbuf);
    static std::string  extract_text(const sbuf_t &sbuf);

    void recurse_streams(scanner_params &sp); // for every decompressed stream, extract the text and recurse
    void decompress_streams_extract_text();         // decompress the streams that were found
    void find_streams();                // find all of the text in sbuf
    void run(scanner_params &sp); // report the text to feature recorders and recurse as necessary
};


#endif
