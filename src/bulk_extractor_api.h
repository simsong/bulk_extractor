#ifndef BULK_EXTRACTOR_API_H
#define BULK_EXTRACTOR_API_H

#include <sys/types.h>
#include <stdint.h>

#define BULK_EXTRACTOR_API_FLAG_FEATURE   0x0001
#define BULK_EXTRACTOR_API_FLAG_HISTOGRAM 0x0002
#define BULK_EXTRACTOR_API_FLAG_CARVED    0x0004

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


typedef BEFILE * (*bulk_extractor_open_t)(be_callback cb);
extern "C" bulk_extractor_open_t bulk_extractor_open;

typedef int (*bulk_extractor_analyze_buf_t)(BEFILE *bef,uint8_t *buf,size_t buflen);
extern "C" bulk_extractor_analyze_buf_t bulk_extractor_analyze_buf;

typedef int (*bulk_extractor_close_t)(BEFILE *bef);
extern "C" bulk_extractor_close_t  bulk_extractor_close;


#endif
