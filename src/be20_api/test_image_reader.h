#ifndef TEST_IMAGE_READER
#define TEST_IMAGE_READER


#include "abstract_image_reader.h"

class test_image_reader : public abstract_image_reader {
public:
    test_image_reader();
    virtual ~test_image_reader();
    virtual ssize_t pread(void *buf, size_t bufsize, uint64_t offset) const;
    virtual int64_t image_size() const;
    virtual std::filesystem::path image_fname() const { return std::filesystem::path("/"); }
};

#endif
