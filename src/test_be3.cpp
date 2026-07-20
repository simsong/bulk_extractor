/****************************************************************
 * test_be3.cpp:
 * end-to-end tests
 */



#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#include <unistd.h>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>

#include "be20_api/catch.hpp"

#include "dfxml_cpp/src/dfxml_writer.h"
#include "be20_api/path_printer.h"
#include "be20_api/scanner_set.h"
#include "be20_api/utils.h"             // needs config.h

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
#ifdef HAVE_ADDRESS_SANITIZER
    std::cout << "[asan] ";
#endif
#ifdef HAVE_THREAD_SANITIZER
    std::cout << "[tsan] ";
#endif

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
    RUNNING_UNDER_CATCH=true;
    auto t0 = time(0);
    int ret = bulk_extractor_main(cout, cerr, argv_count(argv), const_cast<char * const *>(argv));
    auto t  = time(0) - t0;
    if (t>10) {
        std::cout << "elapsed time: " << time(0) - t0 << "s" << std::endl;
    }
    return ret;
}

/*
 * Run BE and capture the output
 */

int run_be(std::ostream &ss, const char **argv)
{
    return run_be(ss, ss, argv);
}

#if defined(HAVE_SYS_RESOURCE_H) && defined(HAVE_SIGNAL_H)
class file_size_limit {
    struct rlimit old_limit {};
    struct sigaction old_action {};
public:
    file_size_limit()
    {
        if (getrlimit(RLIMIT_FSIZE, &old_limit) != 0) {
            throw std::runtime_error("getrlimit(RLIMIT_FSIZE) failed");
        }
        struct sigaction action {};
        action.sa_handler = SIG_IGN;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGXFSZ, &action, &old_action) != 0) {
            throw std::runtime_error("sigaction(SIGXFSZ) failed");
        }

        struct rlimit limit = old_limit;
        limit.rlim_cur = 0;
        if (setrlimit(RLIMIT_FSIZE, &limit) != 0) {
            sigaction(SIGXFSZ, &old_action, nullptr);
            throw std::runtime_error("setrlimit(RLIMIT_FSIZE) failed");
        }
    }

    ~file_size_limit()
    {
        setrlimit(RLIMIT_FSIZE, &old_limit);
        sigaction(SIGXFSZ, &old_action, nullptr);
    }
};
#endif

/****************************************************************
 * Test process_dir
 */
TEST_CASE("process_dir", "[process_dir]") {

    /* This should throw NoSuchFile because there is is an E01 file */
    REQUIRE_THROWS_AS(image_process::open( test_dir(), true, 65536, 65536), image_process::FoundDiskImage);

    /* Get the right return code */
    std::filesystem::path inpath = test_dir();
    std::string inpath_string = inpath.string();
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor", notify(), "-Ro", outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(ss, ss, argv);
    REQUIRE( ret==6 );

    /* This should return the jpegs */
    std::unique_ptr<image_process> p;
    try {
        p = image_process::open( test_dir() / "jpegs", true, 65536, 65536);
    }
    catch (image_process::FoundDiskImage &e) {
        std::cerr << "FoundDiskImage: " << e.what() << std::endl;
        exit(1);
    }
    catch (image_process::IsADirectory &e) {
        std::cerr << "IsAdirectory: " << e.what() << std::endl;
        exit(1);
    }
    catch (image_process::NoSuchFile &e) {
        std::cerr << "NoSuchFile: " << e.what() << std::endl;
        std::cerr << "Current Directory: " << std::filesystem::current_path() << std::endl;
        exit(1);
    }

    //int count = 0;
    for( image_process::iterator it = p->begin(); it != p->end(); ++it ){
        //count++;
        pos0_t pos0 = it.get_pos0();
        REQUIRE( pos0.str().find(".jpg") != std::string::npos );
    }
}


TEST_CASE("e2e-no-args", "[end-to-end]") {
    const char *argv[] = {"bulk_extractor", nullptr};
    std::stringstream ss;
    int ret = run_be(ss, argv);
    REQUIRE( ret==3 );                  // produces 3
}

/* Test -h */
TEST_CASE("e2e-h", "[end-to-end]") {
    /* Try the -h option */
    const char *argv[] = {"bulk_extractor", "-h", nullptr};
    std::stringstream ss;
    int ret = run_be(ss, argv);
    REQUIRE( ret==1 );                  // -h now produces 1
}

/* Test -H */
TEST_CASE("e2e-H", "[end-to-end]") {
    /* Try the -H option */
    const char *argv[] = {"bulk_extractor", "-H", nullptr};
    std::stringstream ss;
    int ret = run_be(ss, argv);
    REQUIRE( ret==2 );                  // -H produces 2
}

/* Run on the first 100k of the emails dataset
 * bulk_extractor -0q -o [outdir] nps-2010-emails.100k.raw
 * Runs twice, so that we can also test the restarting logic
 */
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

    /* make sure that there are both debug:work_start and debug:work_stop tags in the output */
    auto xml_file = outdir_string + "/report.xml";
    grep( "debug:work_start", xml_file);
    grep( "debug:work_stop", xml_file);

    /* Validate the dfxml file is valid dfxml*/
    std::string validate = std::string("xmllint --noout ") + xml_file;
    int code = system( validate.c_str());
    REQUIRE( code==0 );

    // This is the second time through - clear cout and cerr first
    // https://stackoverflow.com/questions/20731/how-do-you-clear-a-stringstream-variable
    std::stringstream().swap(cout);
    std::stringstream().swap(cerr);

    // Re-run to make sure that works
    ret = run_be(cout, cerr, argv);
    if (ret!=0) {
        std::cerr << "STDOUT:" << std::endl << cout.str() << std::endl << std::endl
                  << "STDERR:" << std::endl << cerr.str() << std::endl;
        REQUIRE( ret==0 );
    }

    /* make sure that both tags ended up in the second XML file (the one created from restarting) */
    grep( "debug:work_start", xml_file);
    grep( "debug:work_stop", xml_file);
}

TEST_CASE("e2e-disk-write-error-stops-notifier", "[end-to-end]") {
#if defined(HAVE_SYS_RESOURCE_H) && defined(HAVE_SIGNAL_H)
    std::filesystem::path inpath = test_dir() / "nps-2010-emails.100k.raw";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string inpath_string = inpath.string();
    std::string outdir_string = outdir.string();
    std::stringstream cout, cerr;

    {
        file_size_limit disk_full;
        const char *argv[] = {"bulk_extractor", "--notify_async", "-0q", "-x", "all", "-e", "email",
                              "-o", outdir_string.c_str(), inpath_string.c_str(), nullptr};
        REQUIRE(run_be(cout, cerr, argv) == 6);
    }
    REQUIRE(cerr.str().find("Disk write error during Phase 1") != std::string::npos);
#else
    SUCCEED("This platform has no file-size resource limit.");
#endif
}

/*
 * -x all -e wordlist
 */
TEST_CASE("select_scanners", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "pdf_words2.pdf";
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string inpath_string = inpath.string();
    std::string outdir_string = outdir.string();
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor", "-0q", "-x", "all", "-e", "wordlist", "-o", outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(ss, std::cerr, argv);
    REQUIRE( ret==0 );

    auto lines = getLines( outdir / "report.xml" );
    auto startpos = std::find(lines.begin(), lines.end(), "    <scanners>");
    auto endpos = std::find(lines.begin(), lines.end(), "    </scanners>");
    REQUIRE( startpos != lines.end());
    REQUIRE( endpos != startpos + 1);
}

/* -f simsong
 */
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
    grep( Feature(pos0_t("70-PDF-366"), "simsong", ""), outdir / "find.txt" );
}

/*
 * Test the 5gb flat file if it is present and if the DEBUG_5G environment variable is set.
 */

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

TEST_CASE("e2e-jpeg", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "len6192.jpg";
    std::string inpath_string = inpath.string();
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor",notify(), "-S","jpeg_carve_mode=2","-1q","-o",outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(ss, argv);
    REQUIRE( ret==0 );
    auto lines = getLines( outdir / "report.xml" );
    auto pos = std::find(lines.begin(), lines.end(),
                         "    <hashdigest type='SHA1'>69cee372e6cd7e8e3181aebdb03fc53e18124bff</hashdigest>");
    REQUIRE( pos != lines.end());
}

TEST_CASE("e2e-jpeg-carving-disabled", "[end-to-end]") {
    std::filesystem::path inpath = test_dir() / "len6192.jpg";
    std::string inpath_string = inpath.string();
    std::filesystem::path outdir = NamedTemporaryDirectory();
    std::string outdir_string = outdir.string();
    std::stringstream ss;
    const char *argv[] = {"bulk_extractor", notify(), "-S", "jpeg_carve_mode=0", "-1q", "-o", outdir_string.c_str(), inpath_string.c_str(), nullptr};
    int ret = run_be(ss, argv);
    REQUIRE( ret == 0 );
    REQUIRE_FALSE( std::filesystem::exists(outdir / "jpeg_carved") );
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
    scanner_config   sc;   // config for be20_api
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


/****************************************************************
 * Test restarter
 ** test sbufs (which is this here?
 */

/****************************************************************/
TEST_CASE("image_process", "[phase1]") {
    std::unique_ptr<image_process> p;
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

    const std::filesystem::path split_dir = NamedTemporaryDirectory();
    const std::filesystem::path split0 = split_dir / "split%image.000";
    const std::filesystem::path split1 = split_dir / "split%image.001";
    {
        std::ofstream(split0, std::ios::binary) << "a";
        std::ofstream(split1, std::ios::binary) << "bc";
    }
    auto split = image_process::open(split0, false, 65536, 65536);
    REQUIRE(split->image_size() == 3);

    const std::filesystem::path e01_dir = NamedTemporaryDirectory();
    const std::filesystem::path lower_e01 = e01_dir / "CFReDS001.e01";
    std::filesystem::copy_file(test_dir() / "CFReDS001.E01", lower_e01);
    auto e01 = image_process::open(lower_e01, false, 65536, 65536);
    REQUIRE(e01 != nullptr);
    REQUIRE_THROWS_AS(image_process::open(e01_dir, true, 65536, 65536), image_process::FoundDiskImage);
}

#if defined(__linux__)
// Linux makes truncation visible to an already-open descriptor.  APFS retains
// the old view, so this real short-read test cannot run on macOS.
TEST_CASE("image_process short raw read", "[phase1]") {
    constexpr size_t page_size = 128 * 1024;
    constexpr size_t image_size = 4 * page_size;
    constexpr size_t short_size = page_size + page_size / 2;
    const auto dir = NamedTemporaryDirectory();
    const auto image = dir / "short.raw";
    {
        std::ofstream out(image, std::ios::binary);
        out << std::string(image_size, 'a');
    }

    process_raw reader(image, page_size, page_size);
    REQUIRE(reader.open() == 0);
    std::filesystem::resize_file(image, short_size);
    REQUIRE(std::filesystem::file_size(image) == short_size);

    auto it = reader.begin();
    std::unique_ptr<sbuf_t> sbuf(it.sbuf_alloc());
    REQUIRE(sbuf->bufsize == short_size);
    REQUIRE(sbuf->pagesize == page_size);
    REQUIRE(sbuf->asString() == std::string(short_size, 'a'));
}
#endif

#ifdef HAVE_LIBEWF
TEST_CASE("image_process short EWF read", "[phase1]") {
    class partial_ewf_reader final : public process_ewf {
        size_t max_read;

    public:
        partial_ewf_reader(std::filesystem::path image, size_t page_size, size_t margin, size_t max_read_)
            : process_ewf(image, page_size, margin), max_read(max_read_) {}

        ssize_t pread(void *buf, size_t bytes, uint64_t offset) const override {
            if (max_read == 0) return 0;
            return process_ewf::pread(buf, std::min(bytes, max_read), offset);
        }
    };

    constexpr size_t page_size = 4096;
    constexpr size_t short_size = 2048;
    const auto image = test_dir() / "CFReDS001.E01";
    partial_ewf_reader reader(image, page_size, page_size, short_size);
    REQUIRE(reader.open() == 0);

    std::vector<uint8_t> expected(short_size);
    REQUIRE(reader.process_ewf::pread(expected.data(), expected.size(), 0) == short_size);

    auto it = reader.begin();
    std::unique_ptr<sbuf_t> sbuf(it.sbuf_alloc());
    REQUIRE(sbuf->bufsize == short_size);
    REQUIRE(sbuf->pagesize == short_size);
    REQUIRE(sbuf->asString() == std::string(expected.begin(), expected.end()));

    auto end = reader.end();
    REQUIRE(end.sbuf_alloc() == nullptr);
    REQUIRE(end.eof);

    partial_ewf_reader eof_reader(image, page_size, page_size, 0);
    REQUIRE(eof_reader.open() == 0);
    auto eof = eof_reader.begin();
    REQUIRE(eof.sbuf_alloc() == nullptr);
    REQUIRE(eof.eof);
}
#endif

/****************************************************************
 ** Test the path printer
 **/
TEST_CASE("path-printer1", "[path_printer]") {
    scanner_config sc;
    sc.input_fname = test_dir() / "test_hello.512b.gz";
    sc.enable_all_scanners();
    sc.allow_recurse = true;

    scanner_set ss(sc, feature_recorder_set::flags_disabled(), nullptr);
    ss.add_scanners(scanners_builtin);
    ss.apply_scanner_commands();

    auto reader = image_process::open( sc.input_fname, false, 65536, 65536 );
    std::stringstream str;
    class path_printer pp(ss, reader.get(), str);
    pp.process_path("512-GZIP-0/h");    // create a hex dump

    REQUIRE(str.str() == "0000: 6865 6c6c 6f40 776f 726c 642e 636f 6d0a hello@world.com.\n");
    str.str("");

    pp.process_path("512-GZIP-2/r");    // create a hex dump with a different path and the /r
    REQUIRE( str.str() == "14\r\nllo@world.com\n" );
}
