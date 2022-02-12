/*
 *
 * sbuf_decompress.h:
 *
 * Decompress an sbuf.
 */

#ifndef SBUF_DECOMPRESS_H
#define SBUF_DECOMPRESS_H

#include "be20_api/sbuf.h"

struct sbuf_decompress {

    /* returns true if it is possible to decompress at offset.
     * inline for maximum speed
     */
    static bool is_gzip_header(const sbuf_t &sbuf, size_t offset) {
        return sbuf[offset+0]==0x1f && sbuf[offset+1]==0x8b && sbuf[offset+2]==0x08;
    }

    enum mode_t {
        GZIP,                           // seen in GZIP files
        PDF,                            // seen in PDF files
        ZIP                             // seen in ZIP files
    };

    /* Decompress an sbuf with zlib and return an sbuf that needs to be deleted.
     * Returns nullptr if no decompression posssible.
     * @param sbuf - the data to decompress.
     * @param max_uncompr_size - a hint - how much data (max) should we decompress.
     * @param name - what 'name' should go into the forensic path of the resulting sbuf.
     * @param header_size - how much header was *before* sbuf[0] (and should be subtracted from sbuf[0]'s forensic path when the sub-sbuf is created.
     *
     * @return - the decompressed buffer with the correct forensic path, or nullptr
     */

    static sbuf_t *sbuf_new_decompress(const sbuf_t &sbuf, uint32_t max_uncompr_size, const std::string name, mode_t mode, ssize_t header_size);
};

#endif
