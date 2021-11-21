// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 120
#define DO_NOT_USE_WMAIN

#include "config.h"

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

#ifdef HAVE_MACH_O_DYLD_H
#include "mach-o/dyld.h"         // Needed for _NSGetExecutablePath
#endif

#include "dfxml_cpp/src/dfxml_writer.h"
#include "be13_api/path_printer.h"
#include "be13_api/scanner_set.h"
#include "be13_api/utils.h"             // needs config.h

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

const std::string JSON1 {"[{\"1\": \"one@company.com\"}, {\"2\": \"two@company.com\"}, {\"3\": \"two@company.com\"}]"};
const std::string JSON2 {"[{\"1\": \"one@base64.com\"}, {\"2\": \"two@base64.com\"}, {\"3\": \"three@base64.com\"}]\n"};

bool debug = false;

/* We assume that the tests are being run out of bulk_extractor/src/.
 * This returns the directory of the test subdirectory.
 */
std::filesystem::path my_executable()
{
#if defined(HAVE__NSGETEXECUTABLEPATH)
    char path[4096];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0){
        return std::filesystem::path(path);
    }
    throw std::runtime_error("_NSGetExecutablePath failed???\n");
#endif
#if defined(_WIN32)
    char rawPathName[MAX_PATH];
    GetModuleFileNameA(NULL, rawPathName, MAX_PATH);
    return std::filesystem::path(rawPathName);
#endif
#if !defined(HAVE__NSGETEXECUTABLEPATH) && !defined(_WIN32)
    return std::filesystem::canonical("/proc/self/exe");
#endif
}

/* We assume that the tests are being run out of bulk_extractor/src/.
 * This returns the directory of the test subdirectory.
 */
std::filesystem::path test_dir()
{
    // if srcdir is set, use that, otherwise use the directory of the executable
    // srcdir is set when we run under autoconf 'make distcheck'
    const char *srcdir = getenv("srcdir");
    if (srcdir) {
        return std::filesystem::path(srcdir) / "tests";
    }
    return my_executable().parent_path() / "tests";
}

sbuf_t *map_file(std::filesystem::path p)
{
    std::filesystem::path dest = test_dir() / p;
    if (std::filesystem::exists( dest )==false) {
        std::cerr << "test_be.cpp:map_file - " << dest << " does not exist." << std::endl;
        std::cerr << "Environment variables:" << std::endl;
        extern char **environ;
        for (char **env = environ; *env != 0; env++) {
            std::cerr << (*env) << std::endl;
        }
    }
    REQUIRE(std::filesystem::exists(dest)==true);
    sbuf_t::debug_range_exception = true;
    return sbuf_t::map_file( dest );
}


/* Requires that a feature in a set of lines */
bool requireFeature(const std::vector<std::string> &lines, const std::string feature)
{
    for (const auto &it : lines) {
        if ( it.find(feature) != std::string::npos) return true;
    }
    std::cerr << "feature not found: " << feature << "\nfeatures found (perhaps one of these is the feature you are looking for?):\n";
    for (const auto &it : lines) {
        std::cerr << "  " << it << std::endl;
    }
    return false;
}

/* Setup and run a scanner. Return the output directory */
void enable_all_scanners(scanner_config &sc)
{
    sc.push_scanner_command(scanner_config::scanner_command::ALL_SCANNERS, scanner_config::scanner_command::ENABLE);
}


std::filesystem::path test_scanners(const std::vector<scanner_t *> & scanners, sbuf_t *sbuf)
{
    REQUIRE(sbuf->children == 0);

    feature_recorder_set::flags_t frs_flags;
    frs_flags.pedantic = true;          // for testing
    scanner_config sc;
    sc.outdir           = NamedTemporaryDirectory();
    enable_all_scanners(sc);

    scanner_set ss(sc, frs_flags, nullptr);
    for (auto const &it : scanners ){
        ss.add_scanner( it );
    }
    ss.apply_scanner_commands();

    REQUIRE (ss.get_enabled_scanners().size() == scanners.size()); // the one scanner
    if (ss.get_enabled_scanners().size()>0){
        std::cerr << "## output in " << sc.outdir << " for " << ss.get_enabled_scanners()[0] << std::endl;
    } else {
        std::cerr << "## output in " << sc.outdir << " but no enabled scanner! " << std::endl;
    }
    REQUIRE(sbuf->children == 0);
    ss.phase_scan();
    REQUIRE(sbuf->children == 0);
    try {
        ss.schedule_sbuf(sbuf);
    } catch (sbuf_t::range_exception_t &e) {
        std::cerr << "sbuf_t range exception: " << e.what() << std::endl;
        throw std::runtime_error(e.what());
    } catch (scanner_set::NoSuchScanner &e) {
        std::cerr << "no such scanner: " << e.what() << std::endl;
    } catch (std::exception &e) {
        std::cerr << "unknown exception: " << e.what() << std::endl;
        throw e;
    }

    ss.shutdown();
    return sc.outdir;
}

std::filesystem::path test_scanner(scanner_t scanner, sbuf_t *sbuf)
{
    // I couldn't figure out how to pass a vector of scanner_t objects...
    std::vector<scanner_t *>scanners = {scanner };
    return test_scanners(scanners, sbuf);
}


TEST_CASE("base64_forensic", "[support]") {
    sbuf_t::debug_range_exception = true;
    const char *encoded="SGVsbG8gV29ybGQhCg==";
    const char *decoded="Hello World!\n";
    unsigned char output[64];
    size_t result = b64_pton_forensic(encoded, strlen(encoded), output, sizeof(output));
    REQUIRE( result == strlen(decoded) );
    REQUIRE( strncmp( (char *)output, decoded, strlen(decoded))==0 );
}

TEST_CASE("scan_base64_functions", "[support]" ){
    base64array_initialize();
    sbuf_t sbuf1("W3siMSI6ICJvbmVAYmFzZTY0LmNvbSJ9LCB7IjIiOiAidHdvQGJhc2U2NC5jb20i");
    bool found_equal = false;
    REQUIRE(sbuf_line_is_base64(sbuf1, 0, sbuf1.bufsize, found_equal) == true);
    REQUIRE(found_equal == false);

    sbuf_t sbuf2("W3siMSI6ICJvbmVAYmFzZTY0LmNvbSJ9LCB7IjIiOiAidHdvQGJhc2U2NC5jb20i\n"
                 "fSwgeyIzIjogInRocmVlQGJhc2U2NC5jb20ifV0K");
    REQUIRE(sbuf_line_is_base64(sbuf2, 0, sbuf1.bufsize, found_equal) == true);
    REQUIRE(found_equal == false);

    sbuf_t *sbuf3 = decode_base64(sbuf2, 0, sbuf2.bufsize);
    REQUIRE(sbuf3 != nullptr);
    REQUIRE(sbuf3->bufsize == 78);
    REQUIRE(sbuf3->asString() == JSON2);
    delete sbuf3;
}

/* scan_email.flex checks */
TEST_CASE("scan_email1", "[support]") {
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

TEST_CASE("scan_email2", "[support]") {
    /* This is text from a PDF, decompressed */
    auto *sbufp = new sbuf_t("q Q q 72 300 460 420 re W n /Gs1 gs /Cs1 cs 1 sc 72 300 460 420re f 0 sc./Gs2 gs q 1 0 0 -1 72720 cm BT 10 0 0 -10 5 10 Tm /F1.0 1 Tf (plain_text_pdf@textedit.com).Tj ET Q Q");
    auto outdir = test_scanner(scan_email, sbufp);
    auto email_txt = getLines( outdir / "email.txt" );
    REQUIRE( requireFeature(email_txt,"135\tplain_text_pdf@textedit.com"));
}

TEST_CASE("scan_email3", "[support]") {
    auto *sbufp = new sbuf_t("plain_text_pdf@textedit.com");
    auto outdir = test_scanner(scan_email, sbufp);
    auto email_txt = getLines( outdir / "email.txt" );
    REQUIRE( requireFeature(email_txt,"0\tplain_text_pdf@textedit.com"));
}

TEST_CASE("scan_email4", "[support]") {
    std::vector<scanner_t *>scanners = {scan_email, scan_pdf };
    auto *sbufp = map_file("nps-2010-emails.100k.raw");
    auto outdir = test_scanners(scanners, sbufp);
    auto email_txt = getLines( outdir / "email.txt" );
    REQUIRE( requireFeature(email_txt,"80896\tplain_text@textedit.com"));
    REQUIRE( requireFeature(email_txt,"70727-PDF-0\tplain_text_pdf@textedit.com\t"));
    REQUIRE( requireFeature(email_txt,"81991-PDF-0\trtf_text_pdf@textedit.com\t"));
    REQUIRE( requireFeature(email_txt,"92231-PDF-0\tplain_utf16_pdf@textedit.com\t"));
}

TEST_CASE("sbuf_decompress_zlib_new", "[support]") {
    auto *sbufp = map_file("test_hello.gz");
    REQUIRE( sbuf_decompress::is_gzip_header( *sbufp, 0) == true);
    REQUIRE( sbuf_decompress::is_gzip_header( *sbufp, 10) == false);
    auto *decomp = sbuf_decompress::sbuf_new_decompress( *sbufp, 1024*1024, "GZIP", sbuf_decompress::mode_t::GZIP, 0 );
    REQUIRE( decomp != nullptr);
    REQUIRE( decomp->asString() == "hello@world.com\n");
    delete decomp;
    delete sbufp;
}

TEST_CASE("scan_email16", "[scanners]") {
    /* utf-16 tests */
    {
        uint8_t c[] {"h\000t\000t\000p\000:\000/\000/\000w\000w\000w\000.\000h\000h\000s\000.\000g\000o\000v\000/\000o\000"
                "c\000r\000/\000h\000i\000p\000a\000a\000/\000c\000o\000n\000s\000u\000m\000"
                "e\000r\000_\000r\000i\000g\000h\000t\000s\000.\000p\000d\000f\000"};
        auto *sbufp = new sbuf_t(pos0_t(), c, sizeof(c));
        auto outdir = test_scanner(scan_email, sbufp);
        auto url_txt = getLines( outdir / "url.txt" );
        REQUIRE( requireFeature(url_txt,"0\thttp://www.hhs.gov/ocr/hipaa/consumer_rights.pdf\t"));
        auto url_histogram_txt = getLines( outdir / "url_histogram.txt" );
        REQUIRE( requireFeature(url_histogram_txt,"n=1\thttp://www.hhs.gov/ocr/hipaa/consumer_rights.pdf\t(utf16=1)"));
    }
}

TEST_CASE("scan_exif", "[scanners]") {
    auto *sbufp = map_file("1.jpg");
    REQUIRE( sbufp->bufsize == 7323 );
    auto res = jpeg_validator::validate_jpeg(*sbufp);
    REQUIRE( res.how == jpeg_validator::COMPLETE );
    delete sbufp;
}

TEST_CASE("scan_msxml","[scanners]") {
    auto *sbufp = map_file("KML_Samples.kml");
    std::string bufstr = msxml_extract_text(*sbufp);
    REQUIRE( bufstr.find("http://maps.google.com/mapfiles/kml/pal3/icon19.png") != std::string::npos);
    REQUIRE( bufstr.find("A collection showing how easy it is to create 3-dimensional") != std::string::npos);
    delete sbufp;
}

TEST_CASE("scan_json1", "[scanners]") {
    /* Make a scanner set with a single scanner and a single command to enable all the scanners.
     */
    auto *sbufp = new sbuf_t("hello {\"hello\": 10, \"world\": 20, \"another\": 30, \"language\": 40} world");
    auto outdir = test_scanner(scan_json, sbufp); // delete sbufp

    /* Read the output */
    auto json_txt = getLines( outdir / "json.txt" );
    auto last = json_txt[json_txt.size()-1];

    REQUIRE(last.substr( last.size() - 40) == "6ee8c369e2f111caa9610afc99d7fae877e616c9");
    REQUIRE(true);
}



/****************************************************************
 ** Network test cases
 */

/*
 * First packet of a wget from http://www.google.com/ over ipv4:

# ifconfig en0
en0: flags=8863<UP,BROADCAST,SMART,RUNNING,SIMPLEX,MULTICAST> mtu 1500
	options=400<CHANNEL_IO>
	ether 2c:f0:a2:f3:a8:ee
	inet6 fe80::1896:319a:43fa:a6fe%en0 prefixlen 64 secured scopeid 0x4
	inet 172.20.0.185 netmask 0xfffff000 broadcast 172.20.15.255
	nd6 options=201<PERFORMNUD,DAD>
	media: autoselect
	status: active
# tcpdump -r packet1.pcap -vvvv -x
reading from file packet1.pcap, link-type EN10MB (Ethernet)
08:39:26.039111 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 64)
    172.20.0.185.59910 > lax30s03-in-f4.1e100.net.http: Flags [SEW], cksum 0x8efd (correct), seq 2878109014, win 65535, options [mss 1460,nop,wscale 6,nop,nop,TS val 1914841783 ecr 0,sackOK,eol], length 0
	0x0000:  4500 0040 0000 4000 4006 3b8d ac14 00b9
	0x0010:  acd9 a584 ea06 0050 ab8c 7556 0000 0000
	0x0020:  b0c2 ffff 8efd 0000 0204 05b4 0103 0306
	0x0030:  0101 080a 7222 2ab7 0000 0000 0402 0000
bash-3.2# xxd packet1.pcap
00000000: d4c3 b2a1 0200 0400 0000 0000 0000 0000  ................
00000010: 0000 0400 0100 0000 fe7e 0e61 c798 0000  .........~.a....
00000020: 4e00 0000 4e00 0000 0050 e804 774b 2cf0  N...N....P..wK,.
00000030: a2f3 a8ee 0800 4500 0040 0000 4000 4006  ......E..@..@.@.
00000040: 3b8d ac14 00b9 acd9 a584 ea06 0050 ab8c  ;............P..
00000050: 7556 0000 0000 b0c2 ffff 8efd 0000 0204  uV..............
00000060: 05b4 0103 0306 0101 080a 7222 2ab7 0000  ..........r"*...
00000070: 0000 0402 0000                           ......
*/

/* ethernet frame for packet above. Note that it starts 6 bytes before the source ethernet mac address.
 * validated with packet decoder at https://hpd.gasmi.net/.
 172.20.0.185 → 172.217.165.132 TCP 59910 → 80 [SYN, ECN, CWR]
Ethernet II
Destination: Nomadix_04:77:4b (00:50:e8:04:77:4b)
Source: Apple_f3:a8:ee (2c:f0:a2:f3:a8:ee)
Type: IPv4 (0x0800)
Internet Protocol Version 4
0100 .... = Version: 4
.... 0101 = Header Length: 20 bytes (5)
Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
Total Length: 64
Identification: 0x0000 (0)
Flags: 0x40, Don't fragment
Fragment Offset: 0
Time to Live: 64
Protocol: TCP (6)
Header Checksum: 0x3b8d   (15245)
Header checksum status: Unverified
Source Address: 172.20.0.185
Destination Address: 172.217.165.132
Transmission Control Protocol
Source Port: 59910
Destination Port: 80
Stream index: 0
TCP Segment Len: 0
Sequence Number: 0
Sequence Number (raw): 2878109014
Next Sequence Number: 1
Acknowledgment Number: 0
Acknowledgment number (raw): 0
1011 .... = Header Length: 44 bytes (11)
Flags: 0x0c2 (SYN, ECN, CWR)
Window: 65535
Calculated window size: 65535
Checksum: 0x8efd
Checksum Status: Unverified
Urgent Pointer: 0
Options: (24 bytes), Maximum segment size, No-Operation (NOP), Window scale, No-Operation (NOP), No-Operation (NOP), Timestamps, SACK permitted, End of Option List (EOL)
Timestamps

 */
uint8_t packet1[] = {
    0x00, 0x50, 0xe8, 0x04, 0x77, 0x4b, 0x2c, 0xf0,
    0xa2, 0xf3, 0xa8, 0xee, 0x08, 0x00, 0x45, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x40, 0x06,
    0x3b, 0x8d, 0xac, 0x14, 0x00, 0xb9, 0xac, 0xd9, 0xa5, 0x84, 0xea, 0x06, 0x00, 0x50, 0xab, 0x8c,
    0x75, 0x56, 0x00, 0x00, 0x00, 0x00, 0xb0, 0xc2, 0xff, 0xff, 0x8e, 0xfd, 0x00, 0x00, 0x02, 0x04,
    0x05, 0xb4, 0x01, 0x03, 0x03, 0x06, 0x01, 0x01, 0x08, 0x0a, 0x72, 0x22, 0x2a, 0xb7, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x02, 0x00, 0x00
};

TEST_CASE("scan_net", "[scanners]") {
    /* We did a rather involved rewrite of scan_net for BE2 so we want to check all of the methods with the
     * data a few bytes into the sbuf.
     */
    constexpr size_t frame_offset = 15;           // where we put the packet. Make sure that it is not byte-aligned!
    constexpr size_t ETHERNET_FRAME_SIZE = 14;
    uint8_t buf[1024];
    memset(buf,0xee,sizeof(buf));       // arbitrary data
    memcpy(buf + frame_offset, packet1, sizeof(packet1)); // copy it to an offset that is not byte-aligned
    sbuf_t sbuf(pos0_t(), buf, sizeof(buf));

    constexpr size_t packet1_ip_len = sizeof(packet1) - ETHERNET_FRAME_SIZE; // 14 bytes for ethernet header

    REQUIRE( packet1_ip_len == 64); // from above

    /* Make an sbuf with just the packet, for initial testing */
    sbuf_t sbufip = sbuf.slice(frame_offset + ETHERNET_FRAME_SIZE);

    scan_net_t::generic_iphdr_t h;

    REQUIRE( scan_net_t::sanityCheckIP46Header( sbufip, 0 , &h) == true );
    REQUIRE( h.checksum_valid == true );

    /* Now try with the offset */
    REQUIRE( scan_net_t::sanityCheckIP46Header( sbuf, frame_offset + ETHERNET_FRAME_SIZE, &h) == true );
    REQUIRE( h.checksum_valid == true );

    /* Change the IP address and make sure that the header is valid but the checksum is not */
    buf[frame_offset + ETHERNET_FRAME_SIZE + 14]++; // increment destination address
    REQUIRE( scan_net_t::sanityCheckIP46Header( sbufip, 0 , &h) == true );
    REQUIRE( h.checksum_valid == false );

    /* Break the port and make sure that the header is no longer valid */
    buf[frame_offset + ETHERNET_FRAME_SIZE] += 0x10; // increment header length
    REQUIRE( scan_net_t::sanityCheckIP46Header( sbufip, 0 , &h) == false );
}

TEST_CASE("scan_pdf", "[scanners]") {
    auto *sbufp = map_file("pdf_words2.pdf");
    pdf_extractor pe(*sbufp);
    pe.find_streams();
    REQUIRE( pe.streams.size() == 4 );
    REQUIRE( pe.streams[1].stream_start == 2214);
    REQUIRE( pe.streams[1].endstream_tag == 4827);
    pe.decompress_streams_extract_text();
    REQUIRE( pe.texts.size() == 1 );
    REQUIRE( pe.texts[0].txt.substr(0,30) == "-rw-r--r--    1 simsong  staff");
    delete sbufp;
}


TEST_CASE("scan_vcard", "[scanners]") {
    /* Make a scanner set with a single scanner and a single command to enable all the scanners.
     */
    auto *sbufp = map_file( "john_jakes.vcf" );
    auto outdir = test_scanner(scan_vcard, sbufp); // deletes sbuf2

    /* Read the output */
    std::string fname = "john_jakes.vcf____-0.vcf";
#ifdef _WIN32
    fname = "Z__home_user_bulk_extractor_src_tests_" + fname;
#endif
    REQUIRE( std::filesystem::exists( outdir / "vcard" / "000" / fname ) == true);
}

TEST_CASE("scan_wordlist", "[scanners]") {
    /* Make a scanner set with a single scanner and a single command to enable all the scanners.
     */
    auto *sbufp = map_file( "john_jakes.vcf" );
    auto outdir = test_scanner(scan_wordlist, sbufp); // deletes sbufp

    /* Read the output */
    auto wordlist_txt = getLines( outdir / "wordlist_dedup_1.txt");
    REQUIRE( wordlist_txt[0] == "States" );
    REQUIRE( wordlist_txt[1] == "America" );
    REQUIRE( wordlist_txt[2] == "Company" );
}

TEST_CASE("scan_zip", "[scanners]") {
    std::vector<scanner_t *>scanners = {scan_email, scan_zip };
    auto *sbufp = map_file( "testfilex.docx" );
    auto outdir = test_scanners( scanners, sbufp); // deletes sbuf2
    auto email_txt = getLines( outdir / "email.txt" );
    REQUIRE( std::filesystem::exists( outdir / "zip/000/testfilex.docx____-0-ZIP-0__Content_Types_.xml") == true);
    REQUIRE( requireFeature(email_txt,"1771-ZIP-402\tuser_docx@microsoftword.com"));
    REQUIRE( requireFeature(email_txt,"2396-ZIP-1012\tuser_docx@microsoftword.com"));
}


struct Check {
    Check(const Check &ck):fname(ck.fname),feature(ck.feature){};
    Check &operator=(const Check &ck) = delete;

    Check(std::string fname_, Feature feature_):
        fname(fname_), feature(feature_) {};
    std::string fname;
    Feature feature;                    // defined in be13_api/feature_recorder.h
};

TEST_CASE("test_validate", "[phase1]" ) {
    scanner_config sc;

    sc.outdir = NamedTemporaryDirectory();
    enable_all_scanners(sc);
    feature_recorder_set::flags_t frs_flags;
    frs_flags.pedantic = true;          // for testing

    auto *xreport = new dfxml_writer(sc.outdir / "report.xml", false);

    scanner_set ss(sc, frs_flags, xreport);
    ss.add_scanners(scanners_builtin);
    ss.apply_scanner_commands();
    ss.phase_scan();
    ss.shutdown();
    delete xreport;
}


bool feature_match(const Check &exp, const std::string &line)
{
    auto words = split(line, '\t');
    if (words.size() <2 || words.size() > 3) return false;

    if (debug) std::cerr << "check line=" << line << std::endl;

    std::string pos = exp.feature.pos.str();
    if ( pos.size() > 2 ){
        if (ends_with(pos,"-0")) {
            pos.resize(pos.size()-2);
        }
        if (ends_with(pos,"|0")) {
            pos.resize(pos.size()-2);
        }
    }

    if ( pos0_t(words[0]) != exp.feature.pos ){
        if (debug) std::cerr << "  pos " << exp.feature.pos.str() << " does not match '" << words[0] << "'" << std::endl;
        return false;
    }

    if ( words[1] != exp.feature.feature ){
        if (debug)std::cerr << "  feature '" << exp.feature.feature << "' does not match feature '" << words[1] << "'" << std::endl;
        return false;
    }

    std::string ctx = exp.feature.context;
    if (words.size()==2) return ctx=="";

    if ( (ctx=="") || (ctx==words[2]) )  return true;

    if (debug) std::cerr << "  context '" << ctx << "' (len=" << ctx.size() << ") "
                         << "does not match context '" << words[2] << "' (" << words[2].size() << ")\n";

    if ( ends_with(ctx, "*") ) {
        ctx.resize(ctx.size()-1 );
        if (starts_with(words[2], ctx )){
            return true;
        }
        if (debug) std::cerr << "  context did not start with '" << ctx << "'\n";
    } else {
        if (debug) std::cerr << "  context does not end with *\n";
    }

    return false;
}


/*
 * Run all of the built-in scanners on a specific image, look for the given features, and return the directory.
 */
std::filesystem::path validate(std::string image_fname, std::vector<Check> &expected, bool recurse=true, size_t offset=0)
{
    debug = getenv("DEBUG") ? true : false;
    sbuf_t::debug_range_exception = true;
    scanner_config sc;

    sc.outdir           = NamedTemporaryDirectory();
    enable_all_scanners(sc);
    sc.allow_recurse    = recurse;

    std::cerr << "## image_fname: " << image_fname << " outdir: " << sc.outdir << std::endl;

    if (offset==0) {
        sc.input_fname = test_dir() / image_fname;
    } else {
        std::filesystem::path offset_name = sc.outdir / "offset_file";

        std::ifstream in(  test_dir() / image_fname, std::ios::binary);
        std::ofstream out( offset_name );
        in.seekg(offset);
        char ch;
        size_t written = 0;
        while (in.get(ch)) {
            out << ch;
            written ++;
        }
        in.close();
        out.close();
        sc.input_fname = offset_name;
    }

    feature_recorder_set::flags_t frs_flags;
    frs_flags.pedantic = true;          // for testing
    auto *xreport = new dfxml_writer(sc.outdir / "report.xml", false);
    scanner_set ss(sc, frs_flags, xreport);
    ss.add_scanners(scanners_builtin);
    ss.apply_scanner_commands();

    if (image_fname != "" ) {
        try {
            auto p = image_process::open( sc.input_fname, false, 65536, 65536);
            Phase1::Config cfg; // config for the image_processing system
            cfg.opt_quiet = true;       // do not need status reports
            Phase1 phase1(cfg, *p, ss);
            phase1.dfxml_write_create( 0, nullptr);

            ss.phase_scan();
            phase1.phase1_run();
            delete p;
        } catch (image_process::NoSuchFile &e) {
            std::cerr << "sc.input_fname=" << sc.input_fname << " no such file: " << e.what() << std::endl;
            bool file_found=false;
            REQUIRE(file_found);
        }
    }
    ss.shutdown();

    xreport->pop("dfxml");
    xreport->close();
    delete xreport;

    for (const auto &exp : expected ) {

        std::filesystem::path fname  = sc.outdir / exp.fname;
        bool found = false;
        for (int pass=0 ; pass<2 && !found;pass++){
            std::string line;
            std::ifstream inFile;
            inFile.open(fname);
            if (!inFile.is_open()) {
                throw std::runtime_error("validate_scanners:[phase1] Could not open "+fname.string());
            }
            while (std::getline(inFile, line) && !found) {
                switch (pass) {
                case 0:
                    if (feature_match(exp, line)){
                        found = true;
                    }
                    break;
                case 1:
                    std::cerr << fname << ":" << line << std::endl; // print the file the second time through
                    break;
                }

            }
        }
        if (!found){
            std::cerr << fname << " did not find"
                      << " pos=" << exp.feature.pos
                      << " feature=" << exp.feature.feature
                      << " context=" << exp.feature.context
                      << std::endl;
        }
        REQUIRE(found);
    }
    return sc.outdir;
}


bool validate_files(const std::filesystem::path &fn0, const std::filesystem::path &fn1)
{
    std::ifstream in0( fn0, std::ios::binary);
    std::ifstream in1( fn1, std::ios::binary);
    REQUIRE( in0.is_open());
    REQUIRE( in1.is_open());
    int errors = 0;
    for(size_t i=0;;i++) {
        uint8_t ch0,ch1;
        in0 >> ch0;
        in1 >> ch1;
        if (ch0 != ch1 ){
            if (errors==0) {
                std::cerr << "file 0 " << fn0 << std::endl;
                std::cerr << "file 1 " << fn1 << std::endl;
            }
            std::cerr << "i=" << i << "  ch0=" << static_cast<u_int>(ch0) << " ch1=" << static_cast<u_int>(ch1) << std::endl;
            errors += 1;
        }
        if (in0.eof() || in1.eof()) break;
    }
    return errors == 0;
}


TEST_CASE("test_aes", "[phase1]") {
    /* Test rotation with various sign extension snaffu */
    uint8_t in[4];
    in[0] = 0;
    in[1] = 0xf1;
    in[2] = 2;
    in[3] = 0xf3;
    rotate32x8(in);
    REQUIRE(in[0] == 0xf1);
    REQUIRE(in[1] == 2);
    REQUIRE(in[2] == 0xf3);
    REQUIRE(in[3] == 0);

    /* Test rotation with various sign extension snaffu */
    in[0] = 0xff;
    in[1] = 1;
    in[2] = 0xf2;
    in[3] = 3;
    rotate32x8(in);
    REQUIRE(in[0] == 1);
    REQUIRE(in[1] == 0xf2);
    REQUIRE(in[2] == 3);
    REQUIRE(in[3] == 0xff);

    /* Try with sign extension */

    std::vector<Check> ex3 {
        Check("aes_keys.txt", Feature("496", "a2 6e 0e 4c 06 c4 bb bf 5d 62 8b c7 f8 b3 91 b6", "AES128")),
        Check("aes_keys.txt", Feature("1120", "dc d2 05 18 c4 16 c0 e2 8e d8 59 9c 86 ed e8 e6", "AES128")),
        Check("aes_keys.txt", Feature("7008", "09 23 e0 4d 40 44 57 1f 55 bf 43 bc ac 06 11 04 45 63 03 a1 52 c5 4c 16 ba a6 96 e9 a6 18 80 65", "AES256")),
        Check("aes_keys.txt", Feature("7304", "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f", "AES256"))
    };
    validate("ram_2pages.bin", ex3);
}


TEST_CASE("test_base16json", "[phase1]") {
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
    validate("test_base16json.txt", ex2);
}

TEST_CASE("test_elf", "[phase1]") {
    std::vector<Check> ex {
        Check("elf.txt", Feature( "0", "9e218cee3b190e8f59ef323b27f4d339481516e9", "<ELF class=\"ELFCLASS64\" data=\"ELFDATA2LSB\" osabi=\"ELFOSABI_NONE\" abiversion=\"0\" >*"))
    };
    validate("hello_elf", ex);

}

TEST_CASE("test_gzip", "[phase1]") {
    std::vector<Check> ex3 {
        Check("email.txt", Feature( "0-GZIP-0", "hello@world.com", "hello@world.com\\012"))
    };
    validate("test_hello.gz", ex3);
}

TEST_CASE("test_json", "[phase1]") {
    std::vector<Check> ex1 {
        Check("json.txt", Feature( "0", JSON1, "ef2b5d7ee21e14eeebb5623784f73724218ee5dd")),
    };
    validate("test_json.txt", ex1);
}

TEST_CASE("KML_Samples.kml","[phase1]"){
    std::vector<Check> ex4 {
        Check("kml.txt",
              Feature( "0",
                       "kml/000/0.kml",
                       "<fileobject><filename>kml/000/0.kml</filename><filesize>35919</filesize><hashdigest type='sha1'>"
                       "cffc78e27ac32414b33d595a0fefcb971eaadaa3</hashdigest></fileobject>"))
    };
    validate("KML_Samples.kml", ex4);
}

TEST_CASE("test_jpeg_rar", "[phase1]") {
    std::vector<Check> ex2 {
        Check("jpeg_carved.txt",
              Feature( "13259-RAR-0", "jpeg_carved/000/13259-RAR-0.jpg"))

    };
    validate("jpegs.rar", ex2);
}

TEST_CASE("test_net1", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "40", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "40", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=1", "192.168.0.91")),
        Check("ip_histogram.txt", Feature( "n=1", "192.168.0.55"))
    };
    auto outdir = validate("ntlm1.pcap", ex2, false);
    /* The output file should equal the input file */
    REQUIRE(validate_files(test_dir() / "ntlm1.pcap", outdir / "packets.pcap"));
}

TEST_CASE("test_net2", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "40", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "40", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip.txt", Feature( "482", "192.168.0.55", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "482", "192.168.0.91", "struct ip R (dst) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=2", "192.168.0.91")),
        Check("ip_histogram.txt", Feature( "n=2", "192.168.0.55"))
    };
    auto outdir = validate("ntlm2.pcap", ex2);
    REQUIRE(validate_files(test_dir() / "ntlm2.pcap", outdir / "packets.pcap"));
}

/* Look at a file with three packets */
TEST_CASE("test_net3", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "40", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "40", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip.txt", Feature( "482", "192.168.0.55", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "482", "192.168.0.91", "struct ip R (dst) cksum-ok")),
        Check("ip.txt", Feature( "1010", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "1010", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=3", "192.168.0.91")),
        Check("ip_histogram.txt", Feature( "n=3", "192.168.0.55"))
    };
    validate("ntlm3.pcap", ex2);
}

/* Look at a file with three packets with an offset of 10, to see if we can find the packets even when the PCAP file header is missing */
TEST_CASE("test_net3+10", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "30", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "30", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip.txt", Feature( "472", "192.168.0.55", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "472", "192.168.0.91", "struct ip R (dst) cksum-ok")),
        Check("ip.txt", Feature( "1000", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "1000", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=3", "192.168.0.91")),
        Check("ip_histogram.txt", Feature( "n=3", "192.168.0.55"))
    };
    validate("ntlm3.pcap", ex2, false, 10);
}

/* Look at a file with three packets with an offset of 24, to see if we can find the packets even when the PCAP record header is missing.
 * (of course, it's only missing for one.)
 */
TEST_CASE("test_net3+24", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "16", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "16", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip.txt", Feature( "458", "192.168.0.55", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "458", "192.168.0.91", "struct ip R (dst) cksum-ok")),
        Check("ip.txt", Feature( "986", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip.txt", Feature( "986", "192.168.0.55", "struct ip R (dst) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=3", "192.168.0.91")),
        Check("ip_histogram.txt", Feature( "n=3", "192.168.0.55"))
    };
    validate("ntlm3.pcap", ex2, false, 24);
}

TEST_CASE("test_net80", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "40", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=80", "192.168.0.91"))
    };
    validate("ntlm80.pcap", ex2);
}

TEST_CASE("test_winpe", "[phase1]") {
    std::vector<Check> ex2 {
        Check("winpe.txt", Feature( "0",
                                    "074b9b371de190a96fb0cb987326cd238142e9d1",
                                    "<PE><FileHeader Machine=\"IMAGE_FILE_MACHINE_I386*"))
    };
    validate("hello_win64_exe", ex2);
}

/****************************************************************
 ** test sbufs (which is this here?
 */

sbuf_t *make_sbuf()
{
    auto sbuf = new sbuf_t("Hello World!");
    return sbuf;
}

/* Test that sbuf data  are not copied when moved to a child.*/
std::atomic<int> counter{0};
const uint8_t *sbuf_buf_loc = nullptr;
void test_process_sbuf(sbuf_t *sbuf)
{
    if (sbuf_buf_loc != nullptr) {
        REQUIRE( sbuf_buf_loc == sbuf->get_buf() );
    }
    delete sbuf;
}

TEST_CASE("sbuf_no_copy", "[threads]") {
    for(int i=0;i<100;i++){
        auto sbuf = make_sbuf();
        sbuf_buf_loc = sbuf->get_buf();
        test_process_sbuf(sbuf);
    }
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
    delete p;
}

/****************************************************************
 ** Test the path printer
 **/
TEST_CASE("path_printer", "[path_printer]") {
    scanner_config sc;
    sc.input_fname = test_dir() / "test_hello.512b.gz";
    enable_all_scanners(sc);
    sc.allow_recurse = true;

    scanner_set ss(sc, feature_recorder_set::flags_disabled(), nullptr);
    ss.add_scanners(scanners_builtin);
    ss.apply_scanner_commands();

    auto reader = image_process::open( sc.input_fname, false, 65536, 65536 );
    std::stringstream str;
    class path_printer pp(&ss, reader, str);
    pp.process_path("512-GZIP-0/h");    // create a hex dump

    REQUIRE(str.str() == "0000: 6865 6c6c 6f40 776f 726c 642e 636f 6d0a hello@world.com.\n");
    str.str("");

    pp.process_path("512-GZIP-2/r");    // create a hex dump with a different path and the /r
    REQUIRE( str.str() == "14\r\nllo@world.com\n" );
}
