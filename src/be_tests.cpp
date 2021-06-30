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

#ifdef HAVE_MACH_O_DYLD_H
#include "mach-o/dyld.h"
#endif

#include "be13_api/dfxml/src/dfxml_writer.h"
#include "be13_api/scanner_set.h"
#include "be13_api/catch.hpp"
#include "be13_api/utils.h"             // needs config.h

#include "image_process.h"
#include "base64_forensic.h"
#include "phase1.h"

#include "sbuf_decompress.h"
#include "bulk_extractor_scanners.h"
#include "scan_base64.h"
#include "scan_vcard.h"
#include "scan_pdf.h"

#include "scan_email.h"
#include "exif_reader.h"
#include "jpeg_validator.h"


#include "threadpool.hpp"

const std::string JSON1 {"[{\"1\": \"one@company.com\"}, {\"2\": \"two@company.com\"}, {\"3\": \"two@company.com\"}]"};
const std::string JSON2 {"[{\"1\": \"one@base64.com\"}, {\"2\": \"two@base64.com\"}, {\"3\": \"three@base64.com\"}]\n"};

std::filesystem::path test_dir()
{
#ifdef HAVE__NSGETEXECUTABLEPATH
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0){
        std::cerr << "executable path is " << path << "\n";
    }
    return std::filesystem::path(path).parent_path() / "tests";
#else
    return std::filesystem::canonical("/proc/self/exe").parent_path() / "tests";
#endif
}

sbuf_t *map_file(std::filesystem::path p)
{
    return sbuf_t::map_file( test_dir() / p );
}


/* Read all of the lines of a file and return them as a vector */
std::vector<std::string> getLines(const std::string &filename)
{
    std::vector<std::string> lines;
    std::string line;
    std::ifstream inFile;
    inFile.open( filename);
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

/* Setup and run a scanner. Return the output directory */
std::vector<scanner_config::scanner_command> enable_all_scanners = {
    scanner_config::scanner_command(scanner_config::scanner_command::ALL_SCANNERS,
                                    scanner_config::scanner_command::ENABLE)
};


std::filesystem::path test_scanner(scanner_t scanner, sbuf_t *sbuf)
{
    REQUIRE(sbuf->children == 0);

    const feature_recorder_set::flags_t frs_flags;
    scanner_config sc;
    sc.outdir           = NamedTemporaryDirectory();
    sc.scanner_commands = enable_all_scanners;

    std::cerr << "Single scanner output in " << sc.outdir << "\n";

    scanner_set ss(sc, frs_flags);
    ss.add_scanner(scanner);
    ss.apply_scanner_commands();

    REQUIRE (ss.get_enabled_scanners().size()==1); // the one scanner
    std::cerr << "output in " << sc.outdir << " for " << ss.get_enabled_scanners()[0] << "\n";
    REQUIRE(sbuf->children == 0);
    ss.phase_scan();
    REQUIRE(sbuf->children == 0);
    ss.process_sbuf(sbuf);
    ss.shutdown();
    return sc.outdir;
}

TEST_CASE("base64_forensic", "[support]") {
    const char *encoded="SGVsbG8gV29ybGQhCg==";
    const char *decoded="Hello World!\n";
    unsigned char output[64];
    size_t result = b64_pton_forensic(encoded, strlen(encoded), output, sizeof(output));
    REQUIRE( result == strlen(decoded) );
    REQUIRE( strncmp( (char *)output, decoded, strlen(decoded))==0 );
}

TEST_CASE("scan_base64_functions", "[support]" ){
    base64array_initialize();
    auto sbuf1 = new sbuf_t("W3siMSI6ICJvbmVAYmFzZTY0LmNvbSJ9LCB7IjIiOiAidHdvQGJhc2U2NC5jb20i");
    auto sbuf2 = new sbuf_t("W3siMSI6ICJvbmVAYmFzZTY0LmNvbSJ9LCB7IjIiOiAidHdvQGJhc2U2NC5jb20i\n"
                            "fSwgeyIzIjogInRocmVlQGJhc2U2NC5jb20ifV0K");
    bool found_equal = false;
    REQUIRE(sbuf_line_is_base64(*sbuf1, 0, sbuf1->bufsize, found_equal) == true);
    REQUIRE(found_equal == false);
    auto sbuf3 = decode_base64(*sbuf2, 0, sbuf2->bufsize);
    REQUIRE(sbuf3->bufsize == 78);
    REQUIRE(sbuf3->asString() == JSON2);
}

/* scan_email.flex checks */
TEST_CASE("scan_email_functions", "[support]") {
    REQUIRE( extra_validate_email("this@that.com")==true);
    REQUIRE( extra_validate_email("this@that..com")==false);
    auto s1 = sbuf_t("this@that.com");
    auto s2 = sbuf_t("this_that.com");
    REQUIRE( find_host_in_email(s1) == 5);
    REQUIRE( find_host_in_email(s2) == -1);


    auto s3 = sbuf_t("https://domain.com/foobar");
    size_t domain_len = 0;
    REQUIRE( find_host_in_url(s3, &domain_len)==8);
    REQUIRE( domain_len == 10);
}

TEST_CASE("sbuf_decompress_zlib_new", "[support]") {
    auto *sbufj = map_file("test_hello.gz");
    REQUIRE( sbuf_gzip_header( *sbufj, 0) == true);
    REQUIRE( sbuf_gzip_header( *sbufj, 10) == false);
    auto *decomp = sbuf_decompress_zlib_new( *sbufj, 1024*1024, "GZIP" );
    REQUIRE( decomp != nullptr);
    REQUIRE( decomp->asString() == "hello@world.com\n");
    delete decomp;
    delete sbufj;
}

TEST_CASE("scan_exif", "[scanners]") {
    auto sbufj = map_file("1.jpg");
    REQUIRE( sbufj->bufsize == 7323 );
    auto res = jpeg_validator::validate_jpeg(*sbufj);
    REQUIRE( res.how == jpeg_validator::COMPLETE );
}

TEST_CASE("scan_pdf", "[scanners]") {
    auto *sbufj = map_file("pdf_words2.pdf");
    pdf_extractor pe(*sbufj);

    pe.find_streams();
    REQUIRE( pe.streams.size() == 4 );
    REQUIRE( pe.streams[1].stream_start == 2214);
    REQUIRE( pe.streams[1].endstream == 4827);
    pe.decompress_streams_extract_text();
    REQUIRE( pe.texts.size() == 1 );
    REQUIRE( pe.texts[0].txt.substr(0,30) == "-rw-r--r--    1 simsong  staff");
    delete sbufj;
}


TEST_CASE("scan_json1", "[scanners]") {
    /* Make a scanner set with a single scanner and a single command to enable all the scanners.
     */
    auto  sbuf1 = new sbuf_t("hello {\"hello\": 10, \"world\": 20, \"another\": 30, \"language\": 40} world");
    auto outdir = test_scanner(scan_json, sbuf1);

    /* Read the output */
    auto json_txt = getLines( outdir / "json.txt" );
    auto last = json_txt[json_txt.size()-1];

    REQUIRE(last.substr( last.size() - 40) == "6ee8c369e2f111caa9610afc99d7fae877e616c9");
    REQUIRE(true);
}

TEST_CASE("scan_net", "[scanners]") {
//TODO: Add checks for IPv4 and IPv6 header checksumers
}

TEST_CASE("scan_vcard", "[scanners]") {
    /* Make a scanner set with a single scanner and a single command to enable all the scanners.
     */
    auto sbuf2 = map_file( "john_jakes.vcf" );
    auto outdir = test_scanner(scan_vcard, sbuf2); // deletes sbuf2

    /* Read the output */
}


struct Check {
    Check(std::string fname_, Feature feature_):
        fname(fname_),
        feature(feature_) {};
    std::string fname;
    Feature feature;
};

/*
 * Run all of the built-in scanners on a specific image, look for the given features, and return the directory.
 */
std::string validate(std::string image_fname, std::vector<Check> &expected)
{
    std::cerr << "================ validate  " << image_fname << " ================\n";

    auto p = image_process::open( test_dir() / image_fname, false, 65536, 65536);
    Phase1::Config cfg;  // config for the image_processing system
    scanner_config sc;

    sc.outdir = NamedTemporaryDirectory();
    sc.scanner_commands = enable_all_scanners;
    const feature_recorder_set::flags_t frs_flags;
    scanner_set ss(sc, frs_flags);
    ss.add_scanners(scanners_builtin);
    ss.apply_scanner_commands();

    auto *xreport = new dfxml_writer(sc.outdir / "report.xml", false);
    Phase1 phase1(*xreport, cfg, *p, ss);
    phase1.dfxml_create( 0, nullptr);
    ss.phase_scan();
    phase1.run();
    ss.shutdown();
    xreport->pop();                     // dfxml
    xreport->close();

    for (size_t i=0; i<expected.size(); i++){
        std::filesystem::path fname  = sc.outdir / expected[i].fname;
        std::cerr << "---- " << i << " -- " << fname.string() << " ----\n";
        bool found = false;
        for (int pass=0 ; pass<2 && !found;pass++){
            std::string line;
            std::ifstream inFile;
            if (pass==1) {
                std::cerr << fname << ":\n";
            }
            inFile.open(fname);
            if (!inFile.is_open()) {
                throw std::runtime_error("validate_scanners:[phase1] Could not open "+fname.string());
            }
            while (std::getline(inFile, line)) {
                if (pass==1) {
                    std::cerr << line << "\n"; // print the file the second time through
                }
                auto words = split(line, '\t');
                if (words.size()==3 &&
                    words[0]==expected[i].feature.pos &&
                    words[1]==expected[i].feature.feature &&
                    words[2]==expected[i].feature.context){
                    found = true;
                    break;
                }
            }
        }
        if (!found){
            std::cerr << fname << " did not find " << expected[i].feature.pos
                      << " " << expected[i].feature.feature << " " << expected[i].feature.context << "\t";
        }
        REQUIRE(found);
    }
    std::cerr << "--- done ---\n\n";
    return sc.outdir;
}

TEST_CASE("validate_scanners", "[phase1]") {
    auto fn1 = "test_json.txt";
    std::vector<Check> ex1 {
        Check("json.txt",
              Feature( "0",
                       JSON1,
                       "ef2b5d7ee21e14eeebb5623784f73724218ee5dd")),
    };
    validate(fn1, ex1);

    auto fn2 = "test_base16json.txt";
    std::vector<Check> ex2 {
        Check("json.txt",
              Feature( "50-BASE16-0",
                       "[{\"1\": \"one@base16_company.com\"}, "
                       "{\"2\": \"two@base16_company.com\"}, "
                       "{\"3\": \"two@base16_company.com\"}]",
                       "41e3ec783b9e2c2ffd93fe82079b3eef8579a6cd")),

        Check("email.txt",
              Feature( "50-BASE16-8",
                       "one@base16_company.com",
                       "[{\"1\": \"one@base16_company.com\"}, {\"2\": \"two@b")),

    };
    validate(fn2, ex2);

    auto fn3 = "test_hello.gz";
    std::vector<Check> ex3 {
        Check("email.txt",
              Feature( "0-GZIP-0",
                       "hello@world.com",
                       "hello@world.com\\x0A"))

    };
    validate(fn3, ex3);
}



/****************************************************************/
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
        REQUIRE( sbuf_buf_loc == sbuf->get_buf() );
    }
    delete sbuf;
}

TEST_CASE("sbuf_no_copy", "[threads]") {
    for(int i=0;i<100;i++){
        auto sbuf = make_sbuf();
        sbuf_buf_loc = sbuf->get_buf();
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

/****************************************************************/
TEST_CASE("image_process", "[phase1]") {
    image_process *p = nullptr;
    REQUIRE_THROWS_AS( p = image_process::open( "no-such-file", false, 65536, 65536), image_process::NoSuchFile);
    REQUIRE_THROWS_AS( p = image_process::open( "no-such-file", false, 65536, 65536), image_process::NoSuchFile);
    p = image_process::open( test_dir() / "test_json.txt", false, 65536, 65536);
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
    REQUIRE(times==1);
}


#if 0
TEST_CASE("get_sbuf", "[phase1]") {
    image_process *p = image_process::open( image_fname, opt_recurse, cfg.opt_pagesize, cfg.opt_marginsize);
}
#endif
