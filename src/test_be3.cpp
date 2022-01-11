/****************************************************************
 * end-to-end tests
 */



#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"

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

#include "test_be.h"

/* print and count the args */
int argv_count(const char **argv)
{
    std::cout << "$ ";                  // show that we are testing with this command line
    int argc = 0;
    while(argv[argc]){
        std::cout << argv[argc++] << " ";
    }
    std::cout << std::endl;
    return argc;
}

int run_be(std::ostream &cout, std::ostream &cerr, const char **argv)
{
    auto t0 = time(0);
    int ret = bulk_extractor_main(cout, cerr, argv_count(argv), const_cast<char * const *>(argv));
    auto t  = time(0) - t0;
    if (t>10) {
        std::cout << "elapsed time: " << time(0) - t0 << "s" << std::endl;
    }
    return ret;
}

int run_be(std::ostream &ss, const char **argv)
{
    return run_be(ss, ss, argv);
}

TEST_CASE("e2e-no-args", "[end-to-end]") {
    const char *argv[] = {"bulk_extractor", nullptr};
    std::stringstream ss;
    int ret = run_be(ss, argv);
    REQUIRE( ret==3 );                  // produces 3
}

TEST_CASE("e2e-h", "[end-to-end]") {
    /* Try the -h option */
    const char *argv[] = {"bulk_extractor", "-h", nullptr};
    std::stringstream ss;
    int ret = run_be(ss, argv);
    REQUIRE( ret==1 );                  // -h now produces 1
}

TEST_CASE("e2e-H", "[end-to-end]") {
    /* Try the -H option */
    const char *argv[] = {"bulk_extractor", "-H", nullptr};
    std::stringstream ss;
    int ret = run_be(ss, argv);
    REQUIRE( ret==2 );                  // -H produces 2
}

TEST_CASE("e2e-0", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "nps-2010-emails.100k.raw";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    /* Try to run twice. There seems to be a problem with the second time through.  */
    std::string inpath_string = inpath.string();
    std::string outdir_string = outdir.string();
    std::stringstream cout, cerr;
    const char *argv[] = {"bulk_extractor", "-0q", "-o", outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(cout, cerr, argv);
    if (ret!=0) {
        std::cerr << "STDOUT:" << std::endl << cout.str() << std::endl << std::endl << "STDERR:" << std::endl << cerr.str() << std::endl;
        REQUIRE( ret==0 );
    }
    // https://stackoverflow.com/questions/20731/how-do-you-clear-a-stringstream-variable
    std::stringstream().swap(cout);
    std::stringstream().swap(cerr);

    ret = run_be(cout, cerr, argv);
    if (ret!=0) {
        std::cerr << "STDOUT:" << std::endl << cout.str() << std::endl << std::endl
                  << "STDERR:" << std::endl << cerr.str() << std::endl;
        REQUIRE( ret==0 );
    }

    /* Validate the output dfxml file */
    std::string validate = std::string("xmllint --noout ") + outdir_string + "/report.xml";
    int code = system( validate.c_str());
    REQUIRE( code==0 );
}


TEST_CASE("scan_find", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "pdf_words2.pdf";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string inpath_string = inpath.string();
    std::string outdir_string = outdir.string();
    std::stringstream cout, cerr;
    const char *argv[] = {"bulk_extractor", "-0q", "-f", "simsong", "-o", outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(cout, cerr, argv);
    if (ret!=0) {
        std::cerr << "STDOUT:" << std::endl << cout.str() << std::endl << std::endl << "STDERR:" << std::endl << cerr.str() << std::endl;
        REQUIRE( ret==0 );
    }

    /* Look for "simsong" in output */
    std::cerr << "outdir: " << outdir << std::endl;
    auto lines = getLines( outdir / "find.txt" );
    REQUIRE( lines.size() > 0 );
    std::cerr << "lines.size() = " << lines.size() << std::endl;
}

TEST_CASE("5gb-flatfile", "[end-to-end]") {
    /* Make a 5GB file and try to read it. Make sure we get back the known content. */
    if (!getenv_debug("DEBUG_5G")){
        std::cerr << "DEBUG_5G not set; skipping 5gb-flatfile test" << std::endl;
        return;
    }
    std::cerr << "DEBUG_5G is set; starting 5G test" << std::endl;

    const uint64_t count = 5000;
    const uint64_t sz    = 1000000;
    std::filesystem::path fgb_path     = std::filesystem::temp_directory_path() / "5gb-flatfile.raw";
    std::filesystem::path fgb_path_tmp = std::filesystem::temp_directory_path() / "5gb-flatfile.raw.tmp";

    if (!std::filesystem::exists( fgb_path )) {
        std::cout << "Creating 5GB flatfile " << fgb_path << " to test >4GiB file handling" << std::endl;
	// This takes a while, so we write to a tmp file in case we are interrupted.
        std::ofstream of(fgb_path_tmp, std::ios::out | std::ios::binary);
        if (! of.is_open()) {
            std::cerr << "Cannot open " << std::filesystem::absolute(fgb_path) << " for writing: " << ::strerror(errno) << std::endl;
        }
        REQUIRE( of.is_open() );
        char *spaces = new char[sz];
        memset(spaces,' ',sz);
        for (unsigned int i=0;i<count;i++){
            of.write(spaces,sz);
            if (of.rdstate() & (std::ios::badbit|std::ios::failbit|std::ios::eofbit)){
                std::cerr << "write failed: " << ::strerror(errno) << std::endl;
            }
            REQUIRE( (of.rdstate() & (std::ios::badbit|std::ios::failbit|std::ios::eofbit)) == 0 );
        }
        of << "email_one@company.com "; // 22 characters
        of << "email_two@company.com "; // 22 characters
        of.close();
	std::filesystem::rename( fgb_path_tmp, fgb_path );
    }
    REQUIRE( std::filesystem::file_size( fgb_path ) == count * sz + 22 * 2);
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::string fgb_string = fgb_path.string();
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor","-Eemail", notify(), "-1q", "-o", outdir_string.c_str(), fgb_string.c_str(), nullptr};
    int ret = run_be(ss, std::cerr, argv);
    REQUIRE( ret==0 );
    /* Look for the output line */
    auto lines = getLines( outdir / "report.xml" );
    auto pos = std::find(lines.begin(), lines.end(), "    <hashdigest type='SHA1'>dd3aa4543413c448433e2e504424a32c886abdb4</hashdigest>");
    REQUIRE( pos != lines.end());
}

TEST_CASE("30mb-segmented", "[end-to-end]") {
    if (getenv_debug("DEBUG_FAST")){
        std::cerr << "DEBUG_FAST set; 30mb-segmented test" << std::endl;
        return;
    }

    /* make a segmented file, but this time with 20MB segments */
    const uint64_t count = 1000 * 1000;
    const int segments = 5;
    std::filesystem::path seg_base;
    for (int segment = 0; segment < segments; segment++) {
        char fname[64];
        snprintf(fname,sizeof(fname),"30mb-segmented.00%d", segment);
        std::filesystem::path seg_path = std::filesystem::temp_directory_path() / fname;
        if (segment==0) seg_base = seg_path;
        if (!std::filesystem::exists( seg_path ) ||
            std::filesystem::file_size( seg_path ) < 30000000) {
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
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor","-Eemail", notify(), "-1q", "-o", outdir_string.c_str(), seg_string.c_str(), nullptr};
    int ret = run_be(ss, std::cerr, argv);
    REQUIRE( ret==0 );

    auto lines = getLines( outdir / "report.xml" );
    auto pos = std::find(lines.begin(), lines.end(),
                         "    <hashdigest type='SHA1'>d8a220406f4261335a78df2bd3778568677a6c36</hashdigest>");
    REQUIRE( pos != lines.end());
}

TEST_CASE("path-printer2", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "test_base64json.txt";
    std::string inpath_string = inpath.string();
    const char *argv[] = {"bulk_extractor", "-p","0:64/h", inpath_string.c_str(), nullptr};
    std::stringstream ss;
    int ret = run_be(ss, std::cerr, argv);
    std::string EXPECTED =
        "0000: 5733 7369 4d53 4936 4943 4a76 626d 5641 596d 467a 5a54 5930 4c6d 4e76 6253 4a39 W3siMSI6ICJvbmVAYmFzZTY0LmNvbSJ9\n"
        "0020: 4c43 4237 496a 4969 4f69 4169 6448 6476 5147 4a68 6332 5532 4e43 356a 6232 3069 LCB7IjIiOiAidHdvQGJhc2U2NC5jb20i\n";
    std::cerr << "ss: " << std::endl << ss.str() << std::endl;
    REQUIRE( ret == 0);
    REQUIRE( ss.str() == EXPECTED);
}

TEST_CASE("e2e-CFReDS001", "[end-to-end]") {
    if (getenv_debug("DEBUG_FAST")){
        std::cerr << "DEBUG_FAST set; e2e-CFReDS001" << std::endl;
        return;
    }

    std::filesystem::path inpath = test_dir() / "CFReDS001.E01";
    std::string inpath_string = inpath.string();
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor",notify(), "-1qo",outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(ss, argv);
    REQUIRE( ret==0 );
}

TEST_CASE("e2e-email_test", "[end-to-end]") {
    if (getenv_debug("DEBUG_FAST")){
        std::cerr << "DEBUG_FAST set; skipping e2e-email_test" << std::endl;
        return;
    }

    std::filesystem::path inpath = test_dir() / "email_test.E01";
    std::string inpath_string = inpath.string();
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor", notify(), "-1qo",outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(ss, std::cerr,argv);
    REQUIRE( ret==0 );

    /* Verify that every email address is found from user0@company.com through user49993@company.com.
     * This will check the margins and the reading of multi-segment E01 files - sometimes the email address
     * is split across E01 chunks. Remember, the email addresses may not be sorted.
     */

    std::filesystem::path email_feature_file = outdir / "email.txt";
    std::ifstream inFile( email_feature_file );
    std::string line;
    std::unordered_set<std::string> seen;
    while (std::getline(inFile, line)) {
        if (line[0]!='#') {
            auto words = split(line, '\t');
            REQUIRE( words.size() == 3);
            REQUIRE( starts_with(words[1], "user") );
            seen.insert(words[1]);
        }
    }
    for ( int i=0;i<=49993; i++) {
        std::string email = std::string("user") + std::to_string(i) + std::string("@company.com");
        if ( seen.find(email) == seen.end()) {
            std::cerr << "not in " << email_feature_file << ": "  << email << std::endl;
            REQUIRE( seen.find(email) != seen.end());
        }
    }
}



/****************************************************************
 * Test restarter
 */

TEST_CASE("restarter", "[restarter]") {
    scanner_config   sc;   // config for be13_api
    sc.input_fname = test_dir() / "1mb_fat32.dmg";
    sc.outdir = NamedTemporaryDirectory();

    std::filesystem::path ie_xml = test_dir() / "interrupted_report.xml";
    std::filesystem::path out_xml = sc.outdir / "report.xml";

    REQUIRE( std::filesystem::exists( ie_xml ));

    std::filesystem::copy(ie_xml, out_xml);
    REQUIRE( std::filesystem::exists( out_xml ));

    /* make sure it is writable */
    std::filesystem::permissions(out_xml, std::filesystem::perms::owner_all, std::filesystem::perm_options::add);

    Phase1::Config   cfg;  // config for the image_processing system
    bulk_extractor_restarter r(sc, cfg);

    REQUIRE( std::filesystem::exists( out_xml ) == true); // because it has not been renamed yet
    r.restart();
    REQUIRE( std::filesystem::exists( out_xml ) == false); // because now it has been renamed
    REQUIRE( cfg.seen_page_ids.find("369098752") != cfg.seen_page_ids.end() );
    REQUIRE( cfg.seen_page_ids.find("369098752+") == cfg.seen_page_ids.end() );
}
