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

/* Read all of the lines of a file and return them as a vector */
std::vector<std::string> getLines(const std::string &filename)
{
    std::vector<std::string> lines;
    std::string line;
    std::ifstream inFile;
    inFile.open(filename.c_str());
    if (!inFile.is_open()) {
        throw std::runtime_error("getLines: Cannot open file: "+filename);
    }
    while (std::getline(inFile, line)){
        if (line.size()>0){
            lines.push_back(line);
        }
    }
    return lines;
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

    char tmpl[] = "/tmp/dirXXXXXX";
    mkdtemp(tmpl);
    sc.outdir = tmpl;

    scanner_set ss(sc, frs_flags);
    ss.add_scanners(my_scanners);
    ss.apply_scanner_commands();
    REQUIRE (ss.get_enabled_scanners().size()==1); // json
    REQUIRE (ss.feature_file_list().size()==2); // alert & json

    /* Make an sbuf */
    sbuf_t sbuf("hello {\"hello\": 10, \"world\": 20, \"another\": 30, \"language\": 40} world");
    ss.phase_scan();
    ss.process_sbuf(sbuf);
    ss.shutdown();

    /* Read the output */
    std::vector<std::string> json_txt = getLines( sc.outdir + "/json.txt");
    auto last = json_txt[json_txt.size()-1];
    REQUIRE(last.substr( last.size() - 40) == "6ee8c369e2f111caa9610afc99d7fae877e616c9");
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
