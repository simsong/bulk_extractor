/*
 *
 * sbuf_decompress.h:
 *
 * Decompress an sbuf using zlib.
 */

/* Decompress an sbuf with zlib and return an sbuf that needs to be deleted.
 * Returns nullptr if nothing can be decompressed
 */

#include "config.h"
#include "sbuf_decompress.h"

#define ZLIB_CONST
#ifdef HAVE_DIAGNOSTIC_UNDEF
#  pragma GCC diagnostic ignored "-Wundef"
#endif
#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

std::ostream & operator<<(std::ostream &os, const z_stream &zs)
{
    os << " zs.next_in=" << static_cast<const void *>(zs.next_in)
       << " avail_in=" << zs.avail_in
       << " zs.next_out=" << static_cast<const void *>(zs.next_out)
       << " zs.avail_out=" << zs.avail_out;
    return os;
}




sbuf_t *sbuf_decompress::sbuf_new_decompress(const sbuf_t &sbuf, uint32_t max_uncompr_size, const std::string name,
                                             sbuf_decompress::mode_t mode)
{
    sbuf_t *ret = sbuf_t::sbuf_malloc(sbuf.pos0 + name, max_uncompr_size);
    /* Generic zlib decompresser. Works with all the versions we've seen zlib be used. */

    z_stream zs;
    memset(&zs,0,sizeof(zs));

    zs.next_in  = static_cast<const Bytef *>(sbuf.get_buf());
    zs.avail_in = sbuf.bufsize;
    zs.next_out = static_cast<Bytef *>(ret->malloc_buf());
    zs.avail_out = max_uncompr_size;

    /* If there is a gzip header, "Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection" */
    int r = 0;
    switch (mode) {
    case mode_t::GZIP:
        // attempt gzip decoding
        r = inflateInit2(&zs, 32+MAX_WBITS);
        if (r!=0){                  // something go wrong. get out of here.
            throw std::runtime_error("GZIP inflateInit2 failed");
        }
        r = inflate(&zs,Z_SYNC_FLUSH);
        inflateEnd(&zs);
        break;
    case mode_t::PDF:
        r = inflateInit(&zs);
        if (r!=0){                  // something went wrong.
            throw std::runtime_error("PDF inflateInit failed");
        }
        r = inflate(&zs, Z_FINISH);
        inflateEnd(&zs);
        break;
    case mode_t::ZIP:
        r = inflateInit2(&zs,-15);
        if (r!=0){                  // something went wrong.
            throw std::runtime_error("ZIP inflateInit failed");
        }
        r = inflate(&zs,Z_SYNC_FLUSH);
        break;
    default:
        throw std::runtime_error("sbuf_decompress.cpp: invalid mode");
    }
    /* Ignore the error code; process data if we got any */
    deflateEnd(&zs);
    if (zs.total_out > 0){
        /* Shrink the allocated region */
        ret = ret->realloc(zs.total_out);
        return ret;
    }
    delete ret;
    return nullptr;                     // couldn't decompress
}
