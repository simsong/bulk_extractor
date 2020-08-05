// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#include <cstring>

#define CATCH_CONFIG_MAIN
#include "config.h"
#include "be13_api/tests/catch.hpp"
#include "be13_api/bulk_extractor_i.h"
#include "phase1.h"

#include "base64_forensic.h"
#include "dig.h"
#include "exif_reader.h"
#include "findopts.h"
#include "image_process.h"
#include "pattern_scanner.h"
#include "pattern_scanner_utils.h"
#include "pyxpress.h"
#include "sbuf_flex_scanner.h"
#include "scan_ccns2.h"


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

TEST_CASE("scan_json", "[scanners]") {
    /* Test the json scanner without a mock */
    plugin::scanners_process_enable_disable_commands();
    feature_file_names_t feature_file_names;
    plugin::get_scanner_feature_file_names(feature_file_names);
    std::cerr << "A1  \n";

    feature_recorder_set fs(feature_recorder_set::DISABLE_FILE_RECORDERS,"md5","/dev/null","/tmp");	// where the features will be put
    std::cerr << "A2  \n";
TODO: REmove fs.init()
    //fs.init(feature_file_names);             // shoudln't even be used
    plugin::scanners_init(fs);
    std::cerr << "A3  \n";

    /* Make the sbuf */
    const std::string json_demo {" hello [1,2,3] world"};
    const uint8_t *buf =reinterpret_cast<const uint8_t *>(json_demo.c_str());
    std::cerr << "A4  \n";
    sbuf_t sbuf = sbuf_t(pos0_t(), buf, json_demo.size(), json_demo.size(), 0, false);
    std::cerr << "A5  \n";
    plugin::process_sbuf(scanner_params(scanner_params::PHASE_SCAN,sbuf,fs));
    std::cerr << "A6  \n";
    plugin::phase_shutdown(fs);
    std::cerr << "A7  \n";
    fs.process_histograms(0);
}

/* Test the threadpool */
std::atomic<int> counter{0};
TEST_CASE("threadpool", "[threads]") {
    thread_pool t(10);
    for(int i=0;i<1000;i++){
        t.push( []{
                    counter += 1;
                } );
    }
    t.join();
    REQUIRE( counter==1000 );
#import <iostream>
#import <unistd.h>

}
