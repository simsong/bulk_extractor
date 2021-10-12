/****************************************************************
 * end-to-end tests
 */



#include "config.h"
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include <cstring>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
#include <unistd.h>
#include <string>
#include <string_view>
#include <sstream>

#include "be13_api/catch.hpp"

#include "dfxml_cpp/src/dfxml_writer.h"
#include "be13_api/path_printer.h"
#include "be13_api/scanner_set.h"
#include "be13_api/utils.h"             // needs config.h

#include "test_be.h"

#include "bulk_extractor.h"
#include "base64_forensic.h"
#include "bulk_extractor_restarter.h"
#include "bulk_extractor_scanners.h"
#include "exif_reader.h"
#include "image_process.h"
#include "jpeg_validator.h"
#include "phase1.h"
#include "sbuf_decompress.h"
#include "scan_aes.h"
#include "scan_base64.h"
#include "scan_email.h"
#include "scan_msxml.h"
#include "scan_net.h"
#include "scan_pdf.h"
#include "scan_vcard.h"
#include "scan_wordlist.h"

/* print and count the args */
int arg_count(char * const *argv)
{
    std::cout << "testing with command line:" << std::endl;
    int argc = 0;
    while(argv[argc]){
        std::cout << argv[argc++] << " ";
    }
    std::cout << std::endl;
    std::cout << "argc=" << argc << "\n";
    return argc;
}

TEST_CASE("e2e-h", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "nps-2010-emails.100k.raw";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try the -h option */
    const char *argv[] = {"bulk_extractor", "-h", nullptr};
    int ret = bulk_extractor_main(std::cout, std::cerr, 2, const_cast<char * const *>(argv));
    REQUIRE( ret==1 );                  // -h now produces 1
}

TEST_CASE("e2e-H", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "nps-2010-emails.100k.raw";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try the -H option */
    const char *argv[] = {"bulk_extractor", "-H", nullptr};
    int ret = bulk_extractor_main(std::cout, std::cerr, 2, const_cast<char * const *>(argv));
    REQUIRE( ret==2 );                  // -H produces 2
}

TEST_CASE("e2e-no-imagefile", "[end-to-end]") {
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try the -H option */
    const char *argv[] = {"bulk_extractor", nullptr};
    int ret = bulk_extractor_main(std::cout, std::cerr, 1, const_cast<char * const *>(argv));
    REQUIRE( ret==3 );                  // produces 3
}

TEST_CASE("e2e-0", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "nps-2010-emails.100k.raw";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try to run twice. There seems to be a problem with the second time through.  */
    std::string inpath_string = inpath.string();
    std::string outdir_string = outdir.string();
    const char *argv[] = {"bulk_extractor", "-0", "-o", outdir_string.c_str(), inpath_string.c_str(), nullptr};

    std::stringstream cout, cerr;
    int ret = bulk_extractor_main(cout, cerr, 5, const_cast<char * const *>(argv));
    if (ret!=0) {
        std::cerr << "STDOUT:" << std::endl << cout.str() << std::endl << std::endl << "STDERR:" << std::endl << cerr.str() << std::endl;
        REQUIRE( ret==0 );
    }
    // https://stackoverflow.com/questions/20731/how-do-you-clear-a-stringstream-variable
    std::stringstream().swap(cout);
    std::stringstream().swap(cerr);

    ret = bulk_extractor_main(cout, cerr, 5, const_cast<char * const *>(argv));
    if (ret!=0) {
        std::cerr << "STDOUT:" << std::endl << cout.str() << std::endl << std::endl << "STDERR:" << std::endl << cerr.str() << std::endl;
        REQUIRE( ret==0 );
    }

    /* Validate the output dfxml file */
    std::string validate = std::string("xmllint --noout ") + outdir_string + "/report.xml";
    int code = system( validate.c_str());
    REQUIRE( code==0 );
}

TEST_CASE("path-printer", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "test_base64json.txt";
    std::string inpath_string = inpath.string();
    const char *argv[] = {"bulk_extractor","-p","0:64/h", inpath_string.c_str(), nullptr};
    std::stringstream ss;
    int ret = bulk_extractor_main(ss, std::cerr, 4, const_cast<char * const *>(argv));
    std::string EXPECTED =
        "0000: 5733 7369 4d53 4936 4943 4a76 626d 5641 596d 467a 5a54 5930 4c6d 4e76 6253 4a39 W3siMSI6ICJvbmVAYmFzZTY0LmNvbSJ9\n"
        "0020: 4c43 4237 496a 4969 4f69 4169 6448 6476 5147 4a68 6332 5532 4e43 356a 6232 3069 LCB7IjIiOiAidHdvQGJhc2U2NC5jb20i\n";
    REQUIRE( ret == 0);
    REQUIRE( ss.str() == EXPECTED);
}

/****************************************************************
 * Test restarter
 */

TEST_CASE("restarter", "[restarter]") {
    scanner_config   sc;   // config for be13_api
    sc.input_fname = test_dir() / "1mb_fat32.dmg";
    sc.outdir = NamedTemporaryDirectory();

    std::filesystem::copy(test_dir() / "interrupted_report.xml",
                          sc.outdir  / "report.xml");

    Phase1::Config   cfg;  // config for the image_processing system
    bulk_extractor_restarter r(sc, cfg);

    REQUIRE( std::filesystem::exists( sc.outdir / "report.xml") == true); // because it has not been renamed yet
    r.restart();
    REQUIRE( std::filesystem::exists( sc.outdir / "report.xml") == false); // because now it has been renamed
    REQUIRE( cfg.seen_page_ids.find("369098752") != cfg.seen_page_ids.end() );
    REQUIRE( cfg.seen_page_ids.find("369098752+") == cfg.seen_page_ids.end() );
}
