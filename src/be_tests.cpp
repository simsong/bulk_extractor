// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#include <cstring>
#include <iostream>
#include <memory>
#include <cstdio>
#include <stdexcept>
#include <unistd.h>
#include <string>
#include <string_view>

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"

#ifdef HAVE_MACH_O_DYLD_H
#include "mach-o/dyld.h"         // Needed for _NSGetExecutablePath
#endif

#include "dfxml_cpp/src/dfxml_writer.h"
#include "be13_api/catch.hpp"
#include "be13_api/scanner_set.h"
#include "be13_api/utils.h"             // needs config.h

#include "base64_forensic.h"
#include "bulk_extractor_scanners.h"
#include "exif_reader.h"
#include "image_process.h"
#include "jpeg_validator.h"
#include "phase1.h"
#include "sbuf_decompress.h"
#include "scan_base64.h"
#include "scan_email.h"
#include "scan_net.h"
#include "scan_msxml.h"
#include "scan_pdf.h"
#include "scan_vcard.h"
#include "scan_wordlist.h"

const std::string JSON1 {"[{\"1\": \"one@company.com\"}, {\"2\": \"two@company.com\"}, {\"3\": \"two@company.com\"}]"};
const std::string JSON2 {"[{\"1\": \"one@base64.com\"}, {\"2\": \"two@base64.com\"}, {\"3\": \"three@base64.com\"}]\n"};

std::filesystem::path test_dir()
{
#ifdef HAVE__NSGETEXECUTABLEPATH
    char path[4096];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0){
        return std::filesystem::path(path).parent_path() / "tests";
    }
    throw std::runtime_error("_NSGetExecutablePath failed???\n");
#else
    return std::filesystem::canonical("/proc/self/exe").parent_path() / "tests";
#endif
}

sbuf_t *map_file(std::filesystem::path p)
{
    sbuf_t::debug_range_exception = true;
    return sbuf_t::map_file( test_dir() / p );
}


/* Read all of the lines of a file and return them as a vector */
std::vector<std::string> getLines(const std::filesystem::path path)
{
    std::vector<std::string> lines;
    std::string line;
    std::ifstream inFile;
    inFile.open( path );
    if (!inFile.is_open()) {
        std::cerr << "getLines: Cannot open file: " << path << "\n";
        std::string cmd("ls -l " + path.parent_path().string());
        std::cerr << cmd << "\n";
        if (system( cmd.c_str())) {
            std::cerr << "error\n";
        }
        throw std::runtime_error("be_tests:getLines");
    }
    while (std::getline(inFile, line)){
        if (line.size()>0){
            lines.push_back(line);
        }
    }
    return lines;
}

/* Requires that a feature in a set of lines */
bool requireFeature(const std::vector<std::string> &lines, const std::string feature)
{
    for (const auto &it : lines) {
        if ( it.find(feature) != std::string::npos) return true;
    }
    std::cerr << "feature not found: " << feature << "\nfeatures found (perhaps one of these is the feature you are looking for?):\n";
    for (const auto &it : lines) {
        std::cerr << "  " << it << "\n";
    }
    return false;
}

/* Setup and run a scanner. Return the output directory */
std::vector<scanner_config::scanner_command> enable_all_scanners = {
    scanner_config::scanner_command(scanner_config::scanner_command::ALL_SCANNERS,
                                    scanner_config::scanner_command::ENABLE)
};


std::filesystem::path test_scanners(const std::vector<scanner_t *> & scanners, sbuf_t *sbuf)
{
    REQUIRE(sbuf->children == 0);

    const feature_recorder_set::flags_t frs_flags;
    scanner_config sc;
    sc.outdir           = NamedTemporaryDirectory();
    sc.scanner_commands = enable_all_scanners;

    scanner_set ss(sc, frs_flags, nullptr);
    for (auto const &it : scanners ){
        ss.add_scanner( it );
    }
    ss.apply_scanner_commands();

    REQUIRE (ss.get_enabled_scanners().size() == scanners.size()); // the one scanner
    std::cerr << "\n## output in " << sc.outdir << " for " << ss.get_enabled_scanners()[0] << "\n";
    REQUIRE(sbuf->children == 0);
    ss.phase_scan();
    REQUIRE(sbuf->children == 0);
    ss.schedule_sbuf(sbuf);
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
TEST_CASE("scan_email", "[support]") {
    {
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

    {
        /* This is text from a PDF, decompressed */
        auto *sbufp = new sbuf_t("q Q q 72 300 460 420 re W n /Gs1 gs /Cs1 cs 1 sc 72 300 460 420re f 0 sc./Gs2 gs q 1 0 0 -1 72720 cm BT 10 0 0 -10 5 10 Tm /F1.0 1 Tf (plain_text_pdf@textedit.com).Tj ET Q Q");
        auto outdir = test_scanner(scan_email, sbufp);
        auto email_txt = getLines( outdir / "email.txt" );
        REQUIRE( requireFeature(email_txt,"135\tplain_text_pdf@textedit.com"));
    }

    {
        auto *sbufp = new sbuf_t("plain_text_pdf@textedit.com");
        auto outdir = test_scanner(scan_email, sbufp);
        auto email_txt = getLines( outdir / "email.txt" );
        REQUIRE( requireFeature(email_txt,"0\tplain_text_pdf@textedit.com"));
    }

    {
        std::vector<scanner_t *>scanners = {scan_email, scan_pdf };
        auto *sbufp = map_file("nps-2010-emails.100k.raw");
        auto outdir = test_scanners(scanners, sbufp);
        auto email_txt = getLines( outdir / "email.txt" );
        REQUIRE( requireFeature(email_txt,"80896\tplain_text@textedit.com"));
        REQUIRE( requireFeature(email_txt,"70727-PDF-0\tplain_text_pdf@textedit.com\t"));
        REQUIRE( requireFeature(email_txt,"81991-PDF-0\trtf_text_pdf@textedit.com\t"));
        REQUIRE( requireFeature(email_txt,"92231-PDF-0\tplain_utf16_pdf@textedit.com\t"));
    }
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
    std::cerr << "IP PACKET:\n";
    sbufip.hex_dump(std::cerr);
    std::cerr << "=========\n";

    scan_net::generic_iphdr_t h;

    REQUIRE( scan_net::sanityCheckIP46Header( sbufip, 0 , &h) == true );
    REQUIRE( h.checksum_valid == true );

    /* Now try with the offset */
    REQUIRE( scan_net::sanityCheckIP46Header( sbuf, frame_offset + ETHERNET_FRAME_SIZE, &h) == true );
    REQUIRE( h.checksum_valid == true );

    /* Change the IP address and make sure that the header is valid but the checksum is not */
    buf[frame_offset + ETHERNET_FRAME_SIZE + 14]++; // increment destination address
    REQUIRE( scan_net::sanityCheckIP46Header( sbufip, 0 , &h) == true );
    REQUIRE( h.checksum_valid == false );

    /* Break the port and make sure that the header is no longer valid */
    buf[frame_offset + ETHERNET_FRAME_SIZE] += 0x10; // increment header length
    REQUIRE( scan_net::sanityCheckIP46Header( sbufip, 0 , &h) == false );


}

TEST_CASE("scan_vcard", "[scanners]") {
    /* Make a scanner set with a single scanner and a single command to enable all the scanners.
     */
    auto *sbufp = map_file( "john_jakes.vcf" );
    auto outdir = test_scanner(scan_vcard, sbufp); // deletes sbuf2

    /* Read the output */
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
    REQUIRE( requireFeature(email_txt,"1771-ZIP-402\tuser_docx@microsoftword.com"));
    REQUIRE( requireFeature(email_txt,"2396-ZIP-1012\tuser_docx@microsoftword.com"));
}


struct Check {
    Check(std::string fname_, Feature feature_):
        fname(fname_),
        feature(feature_) {};
    std::string fname;
    Feature feature;                    // defined in be13_api/feature_recorder.h
};

TEST_CASE("test_validate", "[phase1]" ) {
    scanner_config sc;

    sc.outdir = NamedTemporaryDirectory();
    sc.scanner_commands = enable_all_scanners;
    const feature_recorder_set::flags_t frs_flags;

    auto *xreport = new dfxml_writer(sc.outdir / "report.xml", false);

    scanner_set ss(sc, frs_flags, xreport);
    ss.add_scanners(scanners_builtin);
    ss.apply_scanner_commands();
    ss.phase_scan();
    ss.shutdown();
    delete xreport;
}



/*
 * Run all of the built-in scanners on a specific image, look for the given features, and return the directory.
 */
std::filesystem::path validate(std::string image_fname, std::vector<Check> &expected, bool recurse=true)
{
    sbuf_t::debug_range_exception = true;
    std::cerr << "================ validate  " << image_fname << " ================\n";
    scanner_config sc;

    sc.outdir = NamedTemporaryDirectory();
    sc.scanner_commands = enable_all_scanners;
    sc.input_fname      = image_fname;
    sc.allow_recurse    = recurse;

    const feature_recorder_set::flags_t frs_flags;
    auto *xreport = new dfxml_writer(sc.outdir / "report.xml", false);
    scanner_set ss(sc, frs_flags, xreport);
    ss.add_scanners(scanners_builtin);
    ss.apply_scanner_commands();

    if (image_fname != "" ) {
        try {
            auto p = image_process::open( test_dir() / image_fname, false, 65536, 65536);
            Phase1::Config cfg;  // config for the image_processing system
            Phase1 phase1(cfg, *p, ss);
            phase1.dfxml_write_create( 0, nullptr);

            ss.phase_scan();
            phase1.phase1_run();
            delete p;
        } catch (image_process::NoSuchFile &e) {
            std::cerr << "no such file: " << e.what() << "\n";
            bool file_found=false;
            REQUIRE(file_found);
        }
    }
    ss.shutdown();

    xreport->pop("dfxml");
    xreport->close();
    delete xreport;


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
                std::string pos = expected[i].feature.pos.str();
                if (ends_with(pos,"-0")) {
                    pos = pos.substr(0,pos.size()-2);
                }
                if (ends_with(pos,"|0")) {
                    pos = pos.substr(0,pos.size()-2);
                }
                if (words.size()>=2 &&
                    (words[0]==expected[i].feature.pos) &&
                    (words[1]==expected[i].feature.feature) &&
                    (words.size()==2 ||
                     (words[2]==expected[i].feature.context || expected[i].feature.context.size()==0))) {
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


TEST_CASE("test_json", "[phase1]") {
    std::vector<Check> ex1 {
        Check("json.txt",
              Feature( "0",
                       JSON1,
                       "ef2b5d7ee21e14eeebb5623784f73724218ee5dd")),
    };
    validate("test_json.txt", ex1);
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

TEST_CASE("test_hello", "[phase1]") {
    std::vector<Check> ex3 {
        Check("email.txt",
              Feature( "0-GZIP-0",
                       "hello@world.com",
                       "hello@world.com\\x0A"))

    };
    validate("test_hello.gz", ex3);
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
        Check("ip.txt", Feature( "54", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=1", "192.168.0.91"))
    };
    auto outdir = validate("ntlm1.pcap", ex2, false);
    /* The output file should equal the input file */
    std::filesystem::path fn0 = test_dir() / "ntlm1.pcap";
    std::filesystem::path fn1 = outdir / "packets.pcap";
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
                std::cerr << "file 0 " << fn0 << "\n";
                std::cerr << "file 1 " << fn1 << "\n";
            }
            std::cerr << "i=" << i << "  ch0=" << static_cast<u_int>(ch0) << " ch1=" << static_cast<u_int>(ch1) << "\n";
            errors += 1;
        }
        if (in0.eof() || in1.eof()) break;
    }
    REQUIRE(errors == 0);
}

TEST_CASE("test_net2", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "54", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=1", "192.168.0.91"))
    };
    validate("ntlm2.pcap", ex2);
}

TEST_CASE("test_net3", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "54", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=2", "192.168.0.91"))
    };
    validate("ntlm3.pcap", ex2);
}

TEST_CASE("test_net80", "[phase1]") {
    std::vector<Check> ex2 {
        Check("ip.txt", Feature( "54", "192.168.0.91", "struct ip L (src) cksum-ok")),
        Check("ip_histogram.txt", Feature( "n=80", "192.168.0.91"))
    };
    validate("ntlm80.pcap", ex2);
}

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
