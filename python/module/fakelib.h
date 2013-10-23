#ifndef _FAKELIB_H_
#define _FAKELIB_H_

#include<stdint.h>
#include<stddef.h>

#define BE_FLAG_FEATURE 0x0001
#define BE_FLAG_HISTOGRAM 0x0002
#define BE_FLAG_CARVE 0x0004

typedef int (*be_callback)(int32_t, uint32_t, const char *, const char *, size_t, const char *, size_t);

typedef struct _be_handle {
    int descriptor;
} be_handle;

#ifdef __cplusplus
extern "C" {
#endif

be_handle* bulk_extractor_open();
int bulk_extractor_analyze_dev(be_handle* handle, be_callback cb, const char *dev);
int bulk_extractor_analyze_dir(be_handle* handle, be_callback cb, const char *dir);
int bulk_extractor_analyze_buf(be_handle* handle, be_callback cb, const uint8_t *buf, size_t buflen);
int bulk_extractor_close(be_handle* handle);

#ifdef __cplusplus
}
#endif

#endif
