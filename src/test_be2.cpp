/****************************************************************
 * end-to-end tests
 */



#include "config.h"
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include <algorithm>
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
int argv_count(char * const *argv)
{
    std::cout << "testing with command line:" << std::endl;
    int argc = 0;
    while(argv[argc]){
        std::cout << argv[argc++] << " ";
    }
    std::cout << std::endl;
    return argc;
}

TEST_CASE("e2e-h", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "nps-2010-emails.100k.raw";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try the -h option */
    const char *argv[] = {"bulk_extractor", "-h", nullptr};
    std::stringstream ss;
    int ret = bulk_extractor_main(ss, ss, 2, const_cast<char * const *>(argv));
    REQUIRE( ret==1 );                  // -h now produces 1
}

TEST_CASE("e2e-H", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "nps-2010-emails.100k.raw";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try the -H option */
    const char *argv[] = {"bulk_extractor", "-H", nullptr};
    std::stringstream ss;
    int ret = bulk_extractor_main(ss, ss, 2, const_cast<char * const *>(argv));
    REQUIRE( ret==2 );                  // -H produces 2
}

TEST_CASE("e2e-no-imagefile", "[end-to-end]") {
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try the -H option */
    const char *argv[] = {"bulk_extractor", nullptr};
    std::stringstream ss;
    int ret = bulk_extractor_main(ss, ss, 1, const_cast<char * const *>(argv));
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

TEST_CASE("5gb-flatfile","[end-to-end") {
    /* Make a 5GB file and try to read it. Make sure we get back the known content. */
    const uint64_t count = 5000;
    const uint64_t sz = 1000000;
    std::filesystem::path fgb_path = test_dir() / "5gb-flatfile.raw";
    if (!std::filesystem::exists( fgb_path )) {
        std::cout << "Creating 5GB flatfile to test >4GiB file handling" << std::endl;
        std::ofstream of(fgb_path, std::ios::out | std::ios::binary);
        REQUIRE( of.is_open());
        char *spaces = new char[sz];
        memset(spaces,' ',sz);
        for (unsigned int i=0;i<count;i++){
            of.write(spaces,sz);
        }
        of << "email_one@company.com "; // 22 characters
        of << "email_two@company.com "; // 22 characters
        of.close();
    }
    REQUIRE( std::filesystem::file_size( fgb_path ) == count * sz + 22 * 2);
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::string fgb_string = fgb_path.string();
    const char *argv[] = {"bulk_extractor","-Eemail", "-1", "-o", outdir_string.c_str(), fgb_string.c_str(), nullptr};
    std::stringstream ss;
    int ret = bulk_extractor_main(ss, std::cerr,
                                  argv_count(const_cast<char * const *>(argv)),
                                  const_cast<char * const *>(argv));
    REQUIRE( ret==0 );
    /* Look for the output line */
    auto lines = getLines( outdir / "report.xml" );
    auto pos = std::find(lines.begin(), lines.end(),
                         "    <hashdigest type='SHA1'>dd3aa4543413c448433e2e504424a32c886abdb4</hashdigest>");
    REQUIRE( pos != lines.end());
}

TEST_CASE("30mb-segmented","[end-to-end") {
    /* make a segmented file, but this time with 20MB segments */
    const uint64_t count = 1000 * 1000;
    const int segments = 5;
    std::filesystem::path seg_base;
    for (int segment = 0; segment < segments; segment++) {
        char fname[64];
        snprintf(fname,sizeof(fname),"30mb-segmented.00%d", segment);
        std::filesystem::path seg_path = test_dir() / fname;
        if (segment==0) seg_base = seg_path;
        if (!std::filesystem::exists( seg_path )) {
            std::ofstream of(seg_path, std::ios::out | std::ios::binary);
            REQUIRE( of.is_open());
            for (unsigned int i=0;i<count;i++){
                of << "This is segment " << segment << " line " << i << " \n";
            }
            if (segment == segments-1) {
                of << "email_one@company.com "; // 22 characters
                of << "email_two@company.com "; // 22 characters
            }
            of.close();
        }
    }
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::string seg_string = seg_base.string();
    const char *argv[] = {"bulk_extractor","-Eemail", "-1", "-o", outdir_string.c_str(), seg_string.c_str(), nullptr};
    std::stringstream ss;
    int ret = bulk_extractor_main(ss, std::cerr,
                                  argv_count(const_cast<char * const *>(argv)),
                                  const_cast<char * const *>(argv));
    REQUIRE( ret==0 );

    auto lines = getLines( outdir / "report.xml" );
    auto pos = std::find(lines.begin(), lines.end(),
                         "    <hashdigest type='SHA1'>bf2a268075c68fa1f1f40660434965028b13537d</hashdigest>");
    REQUIRE( pos != lines.end());
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
