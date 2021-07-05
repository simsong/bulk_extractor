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

sbuf_t *sbuf_decompress_zlib_new(const sbuf_t &sbuf, uint32_t max_uncompr_size, const std::string name)
{
    Bytef *decompress_buf = reinterpret_cast<Bytef *>(malloc(max_uncompr_size)); // allocate a huge chunk of memory
    if (decompress_buf==nullptr){
        throw std::bad_alloc();
    }
    /* Generic zlib decompresser. If there is a gzip header, try that first, then try raw. Otherwise try
     * raw first, then gzip
     */

    for (int pass=0; pass<2; pass++){
        z_stream zs;
        memset(&zs,0,sizeof(zs));

        zs.next_in  = reinterpret_cast<const Bytef *>(sbuf.get_buf());
        zs.avail_in = sbuf.bufsize;
        zs.next_out = decompress_buf;
        zs.avail_out = max_uncompr_size;

        /* If there is a gzip header, "Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection" */
        int r = 0;
        if ((pass==0 && sbuf_gzip_header(sbuf,0)) || (pass==1 && !sbuf_gzip_header(sbuf,0))) {
            r = inflateInit2(&zs, 32+MAX_WBITS);
            if (r!=0){                  // something go wrong. get out of here.
                break;
            }
            r = inflate(&zs,Z_SYNC_FLUSH);
        }
        else {
            r = inflateInit(&zs);
            if (r!=0){                  // something went wrong.
                break;
            }
            r = inflate(&zs, Z_FINISH);
        }

        /* Ignore the error code; process data if we got any */
        if (zs.total_out > 0){
            /* Shrink the allocated region */
            decompress_buf = reinterpret_cast<u_char *>(realloc(decompress_buf, zs.total_out));

            /* And return a new sbuf, which will be freed by the caller */
            return sbuf_t::sbuf_new( sbuf.pos0 + name, decompress_buf, zs.total_out, zs.total_out );
        }
    }
    free(decompress_buf);
    return nullptr;                     // couldn't decompress
}
