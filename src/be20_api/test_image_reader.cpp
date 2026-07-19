#include "test_image_reader.h"

test_image_reader::test_image_reader()
{
}

test_image_reader::~test_image_reader()
{
}

/*
 * Virtual data is 0..255 in positions 0..255
 */

ssize_t test_image_reader::pread(void *buf, size_t bufsize, uint64_t offset) const
{
    if ( offset>=256) return 0;
    if ( offset+bufsize > 256) bufsize = 256-offset;
    for ( size_t i=0;i<bufsize;i++){
        (reinterpret_cast<uint8_t *>(buf))[i] = i+offset;
    }
    return bufsize;
}

int64_t test_image_reader::image_size() const
{
    return 256;
}
