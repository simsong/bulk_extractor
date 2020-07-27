// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#include <cstring>

#define CATCH_CONFIG_MAIN
#include "config.h"
#include "base64_forensic.h"
#include "dig.h"
#include "exif_reader.h"
#include "be13_api/tests/catch.hpp"
#include "findopts.h"
#include "image_process.h"
#include "pattern_scanner.h"
#include "pattern_scanner_utils.h"
#include "phase1.h"
#include "pyxpress.h"
#include "sbuf_flex_scanner.h"
#include "scan_ccns2.h"
#include "threadpool.h"


bool test_threadpool()
{
    /* Create a threadpool and run it. */
    return true;
}
    
TEST_CASE("base64_forensic", "[utilities]") {
    const char *encoded="SGVsbG8gV29ybGQhCg==";
    const char *decoded="Hello World!\n";
    unsigned char output[64];
    int result = b64_pton_forensic(encoded, strlen(encoded), output, sizeof(output));
    REQUIRE( result == strlen(decoded) );
    REQUIRE( strncmp( (char *)output, decoded, strlen(decoded))==0 );
}

TEST_CASE("dig", "[utilities]") {
}

TEST_CASE("exif_reader", "[utilities]") {
}

/* Test the threadpool */
TEST_CASE("threadpool", "[threads]") {
    REQUIRE( test_threadpool()==true);
}

