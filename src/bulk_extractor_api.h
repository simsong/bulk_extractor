#ifndef BULK_EXTRACTOR_API_H
#define BULK_EXTRACTOR_API_H

#include <sys/types.h>
#include <stdint.h>

#define BULK_EXTRACTOR_API_FLAG_FEATURE   0x0001
#define BULK_EXTRACTOR_API_FLAG_HISTOGRAM 0x0002
#define BULK_EXTRACTOR_API_FLAG_CARVED    0x0004

typedef struct BEFILE_t BEFILE;
typedef int be_callback_t(int32_t flag,
                        uint32_t arg,
                        const char *feature_recorder_name,
                        const char *pos, // forensic path of the feature
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len);

/* Enable is called before open() to enable or disable */

typedef void (*bulk_extractor_set_enabled_t)(const char *scanner_name,bool enable);
//extern "C" bulk_extractor_set_enabled_t bulk_extractor_set_enabled;
#define BULK_EXTRACTOR_SET_ENABLED "bulk_extractor_set_enabled"

typedef BEFILE * (*bulk_extractor_open_t)(be_callback_t cb);
//extern "C" bulk_extractor_open_t bulk_extractor_open;
#define BULK_EXTRACTOR_OPEN "bulk_extractor_open"

typedef int (*bulk_extractor_analyze_buf_t)(BEFILE *bef,uint8_t *buf,size_t buflen);
//extern "C" bulk_extractor_analyze_buf_t bulk_extractor_analyze_buf;
#define BULK_EXTRACTOR_ANALYZE_BUF "bulk_extractor_analyze_buf"

typedef int (*bulk_extractor_analyze_dev_t)(BEFILE *bef,const char *path);
//extern "C" be_analyze_dev_t bulk_extractor_analyze_dev(BEFILE *bef,const char *path);
#define BULK_EXTRACTOR_ANALYZE_DEV "bulk_extractor_analyze_dev"

typedef int (*bulk_extractor_close_t)(BEFILE *bef);
//extern "C" bulk_extractor_close_t  bulk_extractor_close;
#define BULK_EXTRACTOR_CLOSE "bulk_extractor_close"

#endif
