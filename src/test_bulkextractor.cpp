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

TEST_CASE("scan_json", "[scanners]") {
    /* Test the json scanner without a mock */
    plugin::scanners_process_enable_disable_commands();
    feature_file_names_t feature_file_names;
    plugin::get_scanner_feature_file_names(feature_file_names);
    feature_recorder_set fs(0);	// where the features will be put
    fs.init(feature_file_names,argv[0],opt_outdir);
    plugin::scanners_init(fs);

    /* Make the sbuf */
    sbuf_t *sbuf = sbuf_t::map_file(argv[0]);
    if(!sbuf){
	err(1,"Cannot map file %s:%s\n",argv[0],strerror(errno));
    }

    plugin::process_sbuf(scanner_params(scanner_params::PHASE_SCAN,*sbuf,fs));
    plugin::phase_shutdown(fs);
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
    REQUIRE( counter=1000 )
#import <iostream>
#import <unistd.h>

std::mutex M;
int main(int argc,char **argv)
{

    exit(0);
}


}
