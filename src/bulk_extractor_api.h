#ifndef BULK_EXTRACTOR_API_H
#define BULK_EXTRACTOR_API_H

#include <sys/types.h>
#include <stdint.h>

/*
 * The bulk_extractor_api is an easy-to-use API for bulk_extractor.
 * You define a callback and it automatically gets called when features are found.
 * Feature files are not created; everything is done in memory (so you better have enough!).
 * Histograms are stored in memory.
 */

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

#define BE_SET_ENABLED_SCANNER_DISABLE  0              // disable the scanner
#define BE_SET_ENABLED_SCANNER_ENABLE   1              // enable the scanner
#define BE_SET_ENABLED_FEATURE_DISABLE  2              // disable the feature file
#define BE_SET_ENABLED_FEATURE_ENABLE   3              // enable the feature file
#define BE_SET_ENABLED_MEMHIST_ENABLE   5      // no feature file, memory histograms

typedef BEFILE * (*bulk_extractor_open_t)(be_callback_t cb);
extern "C" BEFILE *bulk_extractor_open(be_callback_t cb);
#define BULK_EXTRACTOR_OPEN "bulk_extractor_open"

typedef void (*bulk_extractor_set_enabled_t)(BEFILE *bef,const char *scanner_name,int code);
extern "C" void bulk_extractor_set_enabled(BEFILE *bef,const char *scanner_name,int code);
#define BULK_EXTRACTOR_SET_ENABLED "bulk_extractor_set_enabled"

typedef int (*bulk_extractor_analyze_buf_t)(BEFILE *bef,uint8_t *buf,size_t buflen);
extern "C" int bulk_extractor_analyze_buf(BEFILE *bef,uint8_t *buf,size_t buflen);
#define BULK_EXTRACTOR_ANALYZE_BUF "bulk_extractor_analyze_buf"

typedef int (*bulk_extractor_analyze_dev_t)(BEFILE *bef,const char *path);
extern "C" int bulk_extractor_analyze_dev(BEFILE *bef,const char *path);
#define BULK_EXTRACTOR_ANALYZE_DEV "bulk_extractor_analyze_dev"

typedef int (*bulk_extractor_close_t)(BEFILE *bef);
extern "C" int bulk_extractor_close(BEFILE *bef);
#define BULK_EXTRACTOR_CLOSE "bulk_extractor_close"

#endif
