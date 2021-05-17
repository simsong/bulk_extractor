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
        scanner_config::scanner_command(scanner_config::scanner_command::ALL_SCANNERS,
                                        scanner_config::scanner_command::ENABLE)
    };
    const feature_recorder_set::flags_t frs_flags;
    scanner_config sc;
    sc.scanner_commands = my_scanner_commands;
    sc.outdir = std::filesystem::temp_directory_path().string();
    scanner_set ss(sc, frs_flags);
    ss.add_scanners(my_scanners);
    std::cerr << "1commands: " << my_scanner_commands.size() << "\n";
    std::cerr << "2commands: " << sc.scanner_commands.size() << "\n";
    ss.apply_scanner_commands();
#if 0
    for (auto it: ss.get_enabled_scanners()) {
        std::cerr << "enabled scanner: " << it << "\n";
    }
    for (auto it: ss.feature_file_list()) {
        std::cerr << "feature_file: " << it << "\n";
    }
#endif
    REQUIRE (ss.get_enabled_scanners().size()==1); // json
    REQUIRE (ss.feature_file_list().size()==2); // alert & json

    /* Make an sbuf */
    sbuf_t sbuf("hello [1,2,3] world");
    ss.phase_scan();
    ss.process_sbuf(sbuf);
    ss.shutdown();
    std::cerr << "A6  \n";
    //plugin::phase_shutdown(fs);
    std::cerr << "A7  \n";
    //fs.process_histograms(0);
    /* And verify that the histogram is correct */
    std::cerr << "check in " << sc.outdir << "\n";
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
