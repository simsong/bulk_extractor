#include<stdlib.h>
#include"fakelib.h"

#ifdef __cplusplus
extern "C" {
#endif

be_handle* bulk_extractor_open()
{
    be_handle* output = (be_handle*) malloc(sizeof(be_handle));
    output->descriptor = 0;
    return output;
}

int bulk_extractor_analyze_dev(be_handle* handle, be_callback cb, const char *dev)
{
    cb(BE_FLAG_FEATURE, 0, "dev", "feature", 7, "this is a feature from a device", 31);
    return 0;
}

int bulk_extractor_analyze_dir(be_handle* handle, be_callback cb, const char *dir)
{
    cb(BE_FLAG_FEATURE, 0, "dir", "feature", 7, "this is a feature from a directory", 34);
    return 0;
}

int bulk_extractor_analyze_buf(be_handle* handle, be_callback cb, const uint8_t *buf, size_t buflen)
{
    cb(BE_FLAG_FEATURE, 0, "buf", "feature", 7, "this is a feature from a buffer", 31);
    return 0;
}

int bulk_extractor_close(be_handle *handle)
{
    free(handle);
    return 0;
}

#ifdef __cplusplus
}
#endif
