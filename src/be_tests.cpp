// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#include <cstring>
#include <iostream>
#include <memory>
#include <cstdio>
#include <stdexcept>

#include <unistd.h>

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"

#include "be13_api/scanner_set.h"
#include "be13_api/catch.hpp"
#include "be13_api/utils.h"


#include "image_process.h"
#include "base64_forensic.h"

//#include "be13_api/bulk_extractor_i.h"
//#include "phase1.h"
//#include "dig.h"
//#include "exif_reader.h"
//#include "findopts.h"
//#include "pattern_scanner.h"
//#include "pattern_scanner_utils.h"
//#include "pyxpress.h"
//#include "sbuf_flex_scanner.h"
//#include "scan_ccns2.h"

/* Bring in the definitions for the  */
#define SCANNER(scanner) extern "C" scanner_t scan_ ## scanner;
#include "bulk_extractor_scanners.h"
#undef SCANNER


#include "threadpool.hpp"

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

    sc.outdir = NamedTemporaryDirectory();

    scanner_set ss(sc, frs_flags);
    ss.add_scanners(my_scanners);
    ss.apply_scanner_commands();
    REQUIRE (ss.get_enabled_scanners().size()==1); // json
    REQUIRE (ss.feature_file_list().size()==2); // alert & json

    /* Make an sbuf */
    auto  sbuf = new sbuf_t("hello {\"hello\": 10, \"world\": 20, \"another\": 30, \"language\": 40} world");
    ss.phase_scan();
    ss.process_sbuf(sbuf);
    ss.shutdown();

    /* Read the output */
    std::vector<std::string> json_txt = getLines( sc.outdir + "/json.txt");
    auto last = json_txt[json_txt.size()-1];
    REQUIRE(last.substr( last.size() - 40) == "6ee8c369e2f111caa9610afc99d7fae877e616c9");
}

/* Test the threadpool */
std::atomic<int> counter{0};
TEST_CASE("threadpool", "[threads]") {
    class threadpool t(10);
    for(int i=0;i<1000;i++){
        t.push( []{ counter += 1; } );
    }
    t.join();
    REQUIRE( counter==1000 );
}

/* Test the threadpool with a function
 * and some threadsafety for printing
 */
std::mutex M;
std::atomic<int> counter2{0};
void inc_counter2(int i)
{
    counter2 += i;
}
TEST_CASE("threadpool2", "[threads]") {
    class threadpool t(10);
    for(int i=0;i<1000;i++){
        t.push( [i]{ inc_counter2(i); } );
    }
    t.join();
    REQUIRE( counter2==499500 );
}

sbuf_t *make_sbuf()
{
    auto sbuf = new sbuf_t("Hello World!");
    std::lock_guard<std::mutex> lock(M);
    return sbuf;
}

/* Test that sbuf data  are not copied when moved to a child.*/
const uint8_t *sbuf_buf_loc = nullptr;
void process_sbuf(sbuf_t *sbuf)
{
    std::lock_guard<std::mutex> lock(M);
    if (sbuf_buf_loc != nullptr) {
        REQUIRE( sbuf_buf_loc == sbuf->buf );
    }
    delete sbuf;
}

TEST_CASE("sbuf_no_copy", "[threads]") {
    for(int i=0;i<100;i++){
        auto sbuf = make_sbuf();
        sbuf_buf_loc = sbuf->buf;
        process_sbuf(sbuf);
    }
}

TEST_CASE("threadpool3", "[threads]") {
    class threadpool t(10);
    for(int i=0;i<100;i++){
        auto sbuf = make_sbuf();
        t.push( [sbuf]{ process_sbuf(sbuf); } );
    }
    t.join();
    REQUIRE( counter==1000 );
}

TEST_CASE("image_process", "[phase1]") {
    //auto outdir = NamedTemporaryDirectory();
    image_process *p = nullptr;
    REQUIRE_THROWS_AS( p = image_process::open( "no-such-file", false, 65536, 65536), image_process::NoSuchFile);
    p = image_process::open( "tests/test_json.txt", false, 65536, 65536);
    REQUIRE( p != nullptr );
    int times = 0;
    for(auto it = p->begin(); it!=p->end(); ++it){
        REQUIRE( times==0 );
        sbuf_t *sbufp = it.sbuf_alloc();
        REQUIRE( sbufp->bufsize == 79 );
        REQUIRE( sbufp->pagesize == 79 );
        delete sbufp;
        times += 1;
    }
}

#if 0
TEST_CASE("get_sbuf", "[phase1]") {
    image_process *p = image_process::open( image_fname, opt_recurse, cfg.opt_pagesize, cfg.opt_marginsize);
    dfxml_writer  *xreport = new dfxml_writer(reportfilename, false);
    Phase1::Config   cfg;  // config for the image_processing system
    scanner_config sc;
    const feature_recorder_set::flags_t frs_flags;
    scanner_set ss(sc, frs_flags);
    Phase1 phase1(*xreport, cfg, *p, ss);
}
#endif
