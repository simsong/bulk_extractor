// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#include <cstring>
#include <iostream>
#include <unistd.h>

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"

#include "scanners.h"
#include "be13_api/scanner_set.h"
#include "be13_api/catch.hpp"
//#include "be13_api/bulk_extractor_i.h"
//#include "phase1.h"

#include "base64_forensic.h"

//#include "dig.h"
//#include "exif_reader.h"
//#include "findopts.h"
//#include "image_process.h"
//#include "pattern_scanner.h"
//#include "pattern_scanner_utils.h"
//#include "pyxpress.h"
//#include "sbuf_flex_scanner.h"
//#include "scan_ccns2.h"

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

scanner_t *my_scanners[] = {scan_json, 0};
TEST_CASE("scan_json", "[scanners]") {
    /* Make a scanner set with a single scanner and a single command to enable all the scanners.
     */
    std::vector<scanner_config::scanner_command> my_scanner_commands = {
        scanner_config::scanner_command(scanner_config::scanner_command::ALL_SCANNERS, scanner_config::scanner_command::ENABLE)
    };
    const feature_recorder_set::flags_t frs_flags;
    scanner_config sc;
    sc.scanner_commands = my_scanner_commands;
    sc.outdir = std::filesystem::temp_directory_path().string();
    scanner_set ss(sc, frs_flags);
#if 0
    //std::vector<scanner_config::scanner_command> scanner_commands = {
    //scanner_config::scanner_command(
    //feature_recorder_set fs(feature_recorder_set::DISABLE_FILE_RECORDERS,
    //"md5","/dev/null","/tmp");	// where the features will be put


    .......

    plugin::scanners_process_enable_disable_commands();
    feature_file_names_t feature_file_names;
    plugin::get_scanner_feature_file_names(feature_file_names);
    std::cerr << "A1  \n";

    std::cerr << "A2  \n";
    //fs.init(feature_file_names);             // shoudln't even be used
    plugin::scanners_init(fs);
    std::cerr << "A3  \n";

 TODO: Move process_sbuf out of plugin:: and into feature_recorder_set

    /* Make the sbuf */
    const std::string json_demo {" hello [1,2,3] world"};
    const uint8_t *buf =reinterpret_cast<const uint8_t *>(json_demo.c_str());
    std::cerr << "A4  \n";
    sbuf_t sbuf = sbuf_t(pos0_t(), buf, json_demo.size(), json_demo.size(), 0, false);
    std::cerr << "A5  \n";
    ss.process_sbuf(scanner_params(scanner_params::PHASE_SCAN,sbuf,fs));
    ss.shutdown();
    std::cerr << "A6  \n";
    plugin::phase_shutdown(fs);
    std::cerr << "A7  \n";
    fs.process_histograms(0);
#endif
    /* And verify that the histogram is correct */
}

#if 0
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
}
#endif
