#ifndef BULK_EXTRACTOR_API_H
#define BULK_EXTRACTOR_API_H

#include <sys/types.h>
#include <stdint.h>
typedef struct BEFILE_t BEFILE;
typedef int be_callback(int32_t flag,
                        uint32_t arg,
                        const char *feature_recorder_name,
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len);


typedef struct BEFILE_t BEFILE;
typedef int be_callback(int32_t flag,
                        uint32_t arg,
                        const char *feature_recorder_name,
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len);


extern "C" {BEFILE *bulk_extractor_open(be_callback cb);}
typedef BEFILE * (*be_open_t)(be_callback cb);           

extern "C" {int bulk_extractor_analyze_buf(BEFILE *bef,uint8_t *buf,size_t buflen);}
typedef int (*be_analyze_buf_t)(BEFILE *bef,uint8_t *buf,size_t buflen);

extern "C" {int bulk_extractor_close(BEFILE *bef);}
typedef int (*be_close_t)(BEFILE *bef);


#endif
