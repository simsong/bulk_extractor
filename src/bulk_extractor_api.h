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

#define BULK_EXTRACTOR_API_CODE_HEARTBEAT       0
#define BULK_EXTRACTOR_API_CODE_FEATURE         1
#define BULK_EXTRACTOR_API_CODE_HISTOGRAM       2
#define BULK_EXTRACTOR_API_CODE_CARVED          3
#define BULK_EXTRACTOR_API_CODE_FEATURELIST    10
#define BULK_EXTRACTOR_API_EXCEPTION         1000 // exception in name

typedef struct BEFILE_t BEFILE;
typedef int be_callback_t(void *user,
                          uint32_t code,
                          uint64_t arg,
                          const char *name, // typically the feature file name
                          const char *pos,  // forensic path of the feature
                          const char *feature,size_t feature_len,
                          const char *context,size_t context_len);

/* Enable is called before open() to enable or disable */
#define BEAPI_PROCESS_COMMANDS 0     // process the enable/disable commands
#define BEAPI_SCANNER_DISABLE  1              // disable the scanner
#define BEAPI_SCANNER_ENABLE   2              // enable the scanner
#define BEAPI_FEATURE_DISABLE  3              // disable the feature file
#define BEAPI_FEATURE_ENABLE   4              // enable the feature file
#define BEAPI_MEMHIST_ENABLE   5      // no feature file, memory histograms
#define BEAPI_MEMHIST_LIMIT    6      // no feature file, memory histograms
#define BEAPI_DISABLE_ALL      7
#define BEAPI_FEATURE_LIST     8        // list all of the feature files to callback function please
#define BEAPI_SCANNER_LIST     9        // list all of the enabled scanners

typedef BEFILE * (*bulk_extractor_open_t)(void *user,be_callback_t cb);
extern "C" BEFILE *bulk_extractor_open(void *user,be_callback_t cb);
#define BULK_EXTRACTOR_OPEN "bulk_extractor_open"

/* Config the scanner or whatever */
typedef void (*bulk_extractor_config_t)(BEFILE *bef,uint32_t cmd,const char *scanner_name,int64_t arg);
extern "C" void bulk_extractor_config(BEFILE *bef,uint32_t cmd,const char *scanner_name,int64_t arg);
#define BULK_EXTRACTOR_CONFIG "bulk_extractor_config"

typedef int (*bulk_extractor_analyze_buf_t)(BEFILE *bef,uint8_t *buf,size_t buflen);
extern "C" int bulk_extractor_analyze_buf(BEFILE *bef,uint8_t *buf,size_t buflen);
#define BULK_EXTRACTOR_ANALYZE_BUF "bulk_extractor_analyze_buf"

typedef int (*bulk_extractor_analyze_dev_t)(BEFILE *bef,const char *path,float frac,int samplesize);
extern "C" int bulk_extractor_analyze_dev(BEFILE *bef,const char *path,float frac,int samplesize);
#define BULK_EXTRACTOR_ANALYZE_DEV "bulk_extractor_analyze_dev"

typedef int (*bulk_extractor_close_t)(BEFILE *bef);
extern "C" int bulk_extractor_close(BEFILE *bef);
#define BULK_EXTRACTOR_CLOSE "bulk_extractor_close"

#endif
