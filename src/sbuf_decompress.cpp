/*
 *
 * sbuf_decompress.h:
 *
 * Decompress an sbuf.
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
    if (!sbuf_decompress_zlib_possible(sbuf, 0)){
        return nullptr;
    }
    Bytef *decompress_buf = reinterpret_cast<Bytef *>(malloc(max_uncompr_size)); // allocate a huge chunk of memory
    if (decompress_buf==nullptr){
        throw std::bad_alloc();
    }
    z_stream zs;
    memset(&zs,0,sizeof(zs));

    zs.next_in  = reinterpret_cast<const Bytef *>(sbuf.get_buf());
    zs.avail_in = sbuf.pagesize;
    zs.next_out = decompress_buf;
    zs.avail_out = max_uncompr_size;

    gz_header_s gzh;
    memset(&gzh,0,sizeof(gzh));

    int r = inflateInit2(&zs,16+MAX_WBITS);
    if (r!=0){
        free(decompress_buf);
        return nullptr;
    }
    r = inflate(&zs,Z_SYNC_FLUSH);
    /* Ignore the error code; process data if we got any */
    if (zs.total_out<=0){
        free(decompress_buf);
        return nullptr;
    }
    /* Shrink the allocated region */
    decompress_buf = reinterpret_cast<u_char *>(realloc(decompress_buf, zs.total_out));

    /* And return a new sbuf, which will be freed by the caller */
    return sbuf_t::sbuf_new( sbuf.pos0 + name, decompress_buf, zs.total_out, zs.total_out );
}
