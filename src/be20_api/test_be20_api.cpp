/*
 * Most be20_api test cases are in this file.
 * The goal is to have complete test coverage of the v2 API
 *
 * Todo list:
 * - test an individual feature recorder (they can exist without scanners)
 * - test a feature recorder set with two feature recorders.
 * - create a generic scanner that counts the number of empty pages.
 * - test the code for creating a scanner set that registering and enabling the scanner.
 * - test code for running the single scanner set.
 */

// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h" //
#include "catch.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <csignal>

#include "stdlib.h"

//#include "astream.h"
#include "atomic_unicode_histogram.h"
#include "packet_info.h"
#include "sbuf.h"
#include "sbuf_stream.h"
#include "utils.h"
#include "dfxml_cpp/src/hash_t.h"
#include "dfxml_cpp/src/dfxml_writer.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/****************************************************************
 *** Support code
 ****************************************************************/

// https://inversepalindrome.com/blog/how-to-create-a-random-string-in-cpp
std::string random_string(std::size_t length) {
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i) { random_string += CHARACTERS[distribution(generator)]; }

    return random_string;
}

// A single tempdir for all of our testing
std::filesystem::path get_tempdir() {
    static std::filesystem::path tempdir("/");

    if (tempdir == std::filesystem::path("/")) {
        tempdir = std::filesystem::temp_directory_path() / random_string(8);
        std::filesystem::create_directory(tempdir);
        std::cerr << "Test results in: " << tempdir << std::endl;
    }
    return tempdir;
}

#ifdef HAVE_MACH_O_DYLD_H
#include <mach-o/dyld.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_LIMITS_LIMITS_H
#include <limits.h>
#endif

std::string get_exe() {
    char rpath[PATH_MAX];
#ifdef HAVE_MACH_O_DYLD_H
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        realpath(path, rpath);
    } else {
        throw std::runtime_error("bufsize too small");
    }
#elif defined(_WIN32)
    GetModuleFileNameA(nullptr, rpath, PATH_MAX);
#else
    ssize_t ret = readlink("/proc/self/exe", rpath, sizeof(rpath));
    if (ret < 0) { throw std::runtime_error("readlink failed"); }
    if (ret == sizeof(rpath)) { throw std::runtime_error("rpath too small"); }
    rpath[ret] = 0; // terminate the string
#endif
    return std::string(rpath);
}

std::filesystem::path tests_dir() {
    /* The be20 tests are built by the parent bulk_extractor tree. */
    const char* srcdir = getenv("srcdir");
    if (srcdir) { return std::filesystem::path(srcdir) / "be20_api" / "tests"; }

    /* Otherwise, return relative to this executable */
    std::filesystem::path p(get_exe());
    return p.parent_path() / "be20_api" / "tests";
}

const char* hello8 = "Hello world!";
const char* hello_sha1 = "d3486ae9136e7856bc42212385ea797094475802";

const uint8_t hello16[] = {"H\000e\000l\000l\000o\000 \000w\000o\000r\000l\000d\000!\000"};
const sbuf_t* hello16_sbuf() {
    pos0_t p0("hello16");
    return sbuf_t::sbuf_new(p0, hello16, sizeof(hello16), sizeof(hello16));
}

/*************************
 *** UNIT TESTS FOLLOW ***
 *************************/

/****************************************************************
 * aftimer.h
 */
#include "aftimer.h"
TEST_CASE("aftimer", "[utils]") {
    aftimer t;
    REQUIRE(int(t.elapsed_seconds()) == 0);
}

/****************************************************************
 * atomic_set_map.h
 */
#include "atomic_set.h"
TEST_CASE("atomic_set", "[atomic]") {
    atomic_set<std::string> as;
    REQUIRE(as.size() == 0);
    as.insert("one");
    as.insert("two");
    as.insert("three");
    REQUIRE(as.contains("one") == true);
    REQUIRE(as.contains("four") == false);
    REQUIRE(as.size() == 3);
}

#include "atomic_map.h"
int *new_int()
{
    return new int(0);
}

struct ab_t {
    int a {0};
    int b {0};
} ;


TEST_CASE("atomic_map", "[atomic]") {
    atomic_map<std::string, ab_t> am;
    am["one"].a = 10;
#if 0
    am["one"].b = 10;
    am["two"].a = 5;
    am["three"].b = 10;
    am["three"].b += 10;
    am["three"].b += 10;
    REQUIRE(am["one"].a == 10);
    REQUIRE(am["one"].b == 10);
    REQUIRE(am["two"].a == 5);
    REQUIRE(am["two"].b == 0);
    REQUIRE(am["three"].b == 30);
#endif
}

/****************************************************************
 * packet_info.h
 */
TEST_CASE("packet_info parses unaligned Ethernet IP frames", "[packet_info]") {
    std::array<uint8_t, 14 + 24 + 20 + 1> ipv4_storage {};
    uint8_t* const ipv4 = ipv4_storage.data() + 1; // Deliberately unaligned.
    const uint8_t ether_dst[] {0, 1, 2, 3, 4, 5};
    const uint8_t ether_src[] {6, 7, 8, 9, 10, 11};
    const uint8_t ip4_src[] {192, 0, 2, 1};
    const uint8_t ip4_dst[] {198, 51, 100, 2};
    std::memcpy(ipv4, ether_dst, sizeof(ether_dst));
    std::memcpy(ipv4 + 6, ether_src, sizeof(ether_src));
    ipv4[12] = 0x08;
    ipv4[13] = 0x00;
    uint8_t* const ip4 = ipv4 + 14;
    ip4[0] = 0x46; // Four bytes of IPv4 options precede the TCP header.
    ip4[9] = IPPROTO_TCP;
    std::memcpy(ip4 + 12, ip4_src, sizeof(ip4_src));
    std::memcpy(ip4 + 16, ip4_dst, sizeof(ip4_dst));
    ip4[24] = 0x12;
    ip4[25] = 0x34;
    ip4[26] = 0x56;
    ip4[27] = 0x78;

    struct pcap_pkthdr ip4_hdr {};
    ip4_hdr.caplen = ip4_hdr.len = ipv4_storage.size() - 1;
    be20::packet_info ip4_packet(DLT_EN10MB, &ip4_hdr, ipv4, ip4_hdr.ts, ip4, ip4_hdr.caplen - 14);
    REQUIRE(ip4_packet.ether_type() == ETHERTYPE_IP);
    REQUIRE(std::memcmp(ip4_packet.get_ether_dhost(), ether_dst, sizeof(ether_dst)) == 0);
    REQUIRE(std::memcmp(ip4_packet.get_ether_shost(), ether_src, sizeof(ether_src)) == 0);
    REQUIRE(ip4_packet.is_ip4());
    REQUIRE(ip4_packet.is_ip4_tcp());
    REQUIRE(ip4_packet.get_ip4_src() == ip4 + 12);
    REQUIRE(ip4_packet.get_ip4_dst() == ip4 + 16);
    REQUIRE(std::memcmp(ip4_packet.get_ip4_src(), ip4_src, sizeof(ip4_src)) == 0);
    REQUIRE(std::memcmp(ip4_packet.get_ip4_dst(), ip4_dst, sizeof(ip4_dst)) == 0);
    REQUIRE(ip4_packet.get_ip4_tcp_sport() == 0x1234);
    REQUIRE(ip4_packet.get_ip4_tcp_dport() == 0x5678);

    std::array<uint8_t, 14 + 40 + 20 + 1> ipv6_storage {};
    uint8_t* const ipv6 = ipv6_storage.data() + 1; // Deliberately unaligned.
    ipv6[12] = 0x86;
    ipv6[13] = 0xdd;
    uint8_t* const ip6 = ipv6 + 14;
    const uint8_t ip6_src[] {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    const uint8_t ip6_dst[] {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
    ip6[0] = 0x60;
    ip6[4] = 0;
    ip6[5] = 20;
    ip6[6] = IPPROTO_TCP;
    std::memcpy(ip6 + 8, ip6_src, sizeof(ip6_src));
    std::memcpy(ip6 + 24, ip6_dst, sizeof(ip6_dst));

    struct pcap_pkthdr ip6_hdr {};
    ip6_hdr.caplen = ip6_hdr.len = ipv6_storage.size() - 1;
    be20::packet_info ip6_packet(DLT_EN10MB, &ip6_hdr, ipv6, ip6_hdr.ts, ip6, ip6_hdr.caplen - 14);
    REQUIRE(ip6_packet.ether_type() == ETHERTYPE_IPV6);
    REQUIRE(ip6_packet.is_ip6());
    REQUIRE(ip6_packet.is_ip6_tcp());
    REQUIRE(ip6_packet.get_ip6_src() == ip6 + 8);
    REQUIRE(ip6_packet.get_ip6_dst() == ip6 + 24);
    REQUIRE(std::memcmp(ip6_packet.get_ip6_src(), ip6_src, sizeof(ip6_src)) == 0);
    REQUIRE(std::memcmp(ip6_packet.get_ip6_dst(), ip6_dst, sizeof(ip6_dst)) == 0);

    ip4_hdr.caplen = sizeof(be20::ether_addr);
    REQUIRE(ip4_packet.ether_type() == 0);
    REQUIRE_THROWS_AS(ip4_packet.get_ether_dhost(), be20::packet_info::frame_too_short);
}

/****************************************************************
 * utils.h
 */

TEST_CASE("scale_stoi64","[utils]") {
    REQUIRE(scaled_stoi64("10") == 10);
    REQUIRE(scaled_stoi64("2k") == 2048);
    REQUIRE(scaled_stoi64("4m") == 4194304);
    REQUIRE(scaled_stoi64("1g") == 1073741824);
}

/*** BE-specific tests continue ***/

/****************************************************************
 * histogram_def.h
 */
#include "histogram_def.h"
TEST_CASE("comparision_functions", "[histogram_def]") {
    histogram_def h1("name1", "feature1", "pattern1", "", "suffix1", histogram_def::flags_t());
    histogram_def h2("name2", "feature2", "pattern2", "", "suffix2", histogram_def::flags_t());

    REQUIRE(h1 == h1);
    REQUIRE(h1 != h2);
    REQUIRE(h1 < h2);
}

TEST_CASE("histogram_def-8", "[histogram_def]") {
    histogram_def d0("numbers", "numbers", "([0-9]+)", "", "s0", histogram_def::flags_t());
    REQUIRE(d0.match("123", nullptr, "") == true);
    REQUIRE(d0.match("abc", nullptr, "") == false);

    std::string s1;
    REQUIRE(d0.match("abc123def", &s1, "") == true);
    REQUIRE(s1 == "123");

    histogram_def d1("extraction", "extraction", "^(.....)", "", "", histogram_def::flags_t());
    REQUIRE(d1.match("abcdefghijklmnop", &s1, "") == true);
    REQUIRE(s1 == "abcde");
};

/****************************************************************
 * atomic_unicode_histogram.h
 */
#include "atomic_unicode_histogram.h"
#include "histogram_def.h"
TEST_CASE("AtomicUnicodeHistogram_1", "[atomic][regex]") {
    /* Histogram that matches everything */
    histogram_def d1("name", "feature_file", "(.*)", "", "suffix1", histogram_def::flags_t());
    AtomicUnicodeHistogram h(d1);
    h.add_feature_context("foo", "");
    h.add_feature_context("foo", "bar");
    h.add_feature_context("foo", "ignore");
    h.add_feature_context("bar", "me");

    /* Now make sure things were added with the right counts */
    AtomicUnicodeHistogram::FrequencyReportVector f = h.makeReport();
    REQUIRE(f.size() == 2);
    REQUIRE(f.at(0).key == "foo");
    REQUIRE(f.at(0).value->count == 3);
    REQUIRE(f.at(0).value->count16 == 0);

    REQUIRE(f.at(1).key == "bar");
    REQUIRE(f.at(1).value->count == 1);
    REQUIRE(f.at(1).value->count16 == 0);

    f = h.makeReport(1);

    REQUIRE(f.size() == 1);
    REQUIRE(f.at(0).key == "foo");
    REQUIRE(f.at(0).value->count == 3);
    REQUIRE(f.at(0).value->count16 == 0);
}

TEST_CASE("AtomicUnicodeHistogram_2", "[atomic][regex]") {
    /* Histogram that matches everything */
    histogram_def d1("extraction", "extraction", "^(.....)", "", "", histogram_def::flags_t());
    AtomicUnicodeHistogram h(d1);
    h.add_feature_context("abcdefghijklmnop", "");

    /* Now make sure things were added with the right counts */
    AtomicUnicodeHistogram::FrequencyReportVector f = h.makeReport();
    REQUIRE(f.at(0).key == "abcde");
    REQUIRE(f.at(0).value->count == 1);
    REQUIRE(f.at(0).value->count16 == 0);
}

TEST_CASE("AtomicUnicodeHistogram_3", "[histogram]") {
    /* Make sure that the histogram elements work */
    AtomicUnicodeHistogram::HistogramTally v1;
    AtomicUnicodeHistogram::HistogramTally v2;
    AtomicUnicodeHistogram::auh_t::item e1("hello", &v1);
    AtomicUnicodeHistogram::auh_t::item e2("world", &v2);

    REQUIRE(e1 == e1);
    REQUIRE(e1 != e2);
    REQUIRE(e1 < e2);

    /* Try a simple histogram with just numbers */
    histogram_def::flags_t flags;
    flags.numeric = true;
    histogram_def h1("phones", "p", "([0-9]+)", "", "phones", flags);
    AtomicUnicodeHistogram hm(h1);

    hm.add_feature_context("100", "");
    hm.add_feature_context("200", "1");
    hm.add_feature_context("300", "2");
    hm.add_feature_context("200", "3");
    hm.add_feature_context("300", "4");
    hm.add_feature_context("foo 300 bar", "");

    std::vector<AtomicUnicodeHistogram::auh_t::item> r = hm.makeReport(0);
    REQUIRE(r.size() == 3);

    /* Make sure that the tallies were correct. */
    REQUIRE(r.at(0).key == "300");
    REQUIRE(r.at(0).value->count == 3);
    REQUIRE(r.at(0).value->count16 == 0);

    REQUIRE(r.at(1).key == "200");
    REQUIRE(r.at(1).value->count == 2);
    REQUIRE(r.at(1).value->count16 == 0);

    REQUIRE(r.at(2).key == "100");
    REQUIRE(r.at(2).value->count == 1);
    REQUIRE(r.at(2).value->count16 == 0);

    /* Add a UTF-16 300 */
    char buf300[6]{'\000', '3', '\000', '0', '\000', '0'};
    std::string b300(buf300, 6);
    hm.add_feature_context(b300, "");
    r = hm.makeReport(0);
    REQUIRE(r.at(0).key == "300");
    REQUIRE(r.at(0).value->count == 4);
    REQUIRE(r.at(0).value->count16 == 1);

    /* write the histogram to a file */
    std::string tempdir = get_tempdir().string();
    std::string fname = tempdir + "/histogram1.txt";
    {
        std::ofstream out(fname.c_str());
        out << r;
        out.close();
    }

    /* And verify the histogram that was written */
    {
        std::vector<std::string> lines{getLines(fname)};
        REQUIRE(lines.size() == 3);
        REQUIRE(lines[0] == "n=4\t300\t(utf16=1)");
        REQUIRE(lines[1] == "n=2\t200");
        REQUIRE(lines[2] == "n=1\t100");
    }
}

/****************************************************************
 * hash_t.h
 */
static std::string hash_name("sha1");
static std::string hash_func(const uint8_t* buf, size_t bufsize) {
    if (hash_name == "md5" || hash_name == "MD5") { return dfxml::md5_generator::hash_buf(buf, bufsize).hexdigest(); }
    if (hash_name == "sha1" || hash_name == "SHA1" || hash_name == "sha-1" || hash_name == "SHA-1") {
        return dfxml::sha1_generator::hash_buf(buf, bufsize).hexdigest();
    }
    if (hash_name == "sha256" || hash_name == "SHA256" || hash_name == "sha-256" || hash_name == "SHA-256") {
        return dfxml::sha256_generator::hash_buf(buf, bufsize).hexdigest();
    }
    std::cerr << "Invalid hash name: " << hash_name << "\n";
    std::cerr << "This version of bulk_extractor only supports MD5, SHA1, and SHA256\n";
    exit(1);
}

// Test the hash function without testing the SBUF, then test it with the sbuf
TEST_CASE("sha1", "[hash]") {
    REQUIRE(hash_func(reinterpret_cast<const uint8_t*>(hello8), strlen(hello8)) == hello_sha1);
}

/****************************************************************
 * feature_recorder.h
 */
#include "feature_recorder.h"
#include "feature_recorder_file.h"
#include "feature_recorder_set.h"
#include "scanner_config.h"
TEST_CASE("quote_if_necessary", "[feature_recorder]") {
    std::string f1("feature");
    std::string c1("context");

    feature_recorder_set::flags_t flags;
    flags.no_alert = true;
    feature_recorder_set frs(flags, scanner_config());
    frs.create_feature_recorder("test");
    feature_recorder& ft = frs.named_feature_recorder("test");
    ft.quote_if_necessary(f1, c1);
    REQUIRE(f1 == "feature");
    REQUIRE(c1 == "context");

    std::string f2("[{\"1\": \"one@company.com\"}, {\"2\": \"two@company.com\"}, {\"3\": \"two@company.com\"}]\");");
    std::string c2("context");

    std::string f2_quoted = f2;
    std::string c2_quoted = c2;
    ft.quote_if_necessary(f2_quoted, c2_quoted);
    REQUIRE(f2_quoted == f2);
    REQUIRE(c2_quoted == c2);
}

TEST_CASE("feature_recorder_tests", "[feature_recorder]") {
    feature_recorder_set::flags_t flags;
    flags.no_alert = true;
    flags.pedantic = true;
    scanner_config sc;
    sc.outdir = NamedTemporaryDirectory();
    feature_recorder_set frs(flags, sc);
    frs.create_feature_recorder("test");
    feature_recorder& fr = frs.named_feature_recorder("test");

    auto p = fr.fname_in_outdir("foo", feature_recorder::NO_COUNT);
    REQUIRE(p.filename() == "test_foo.txt");

    auto p1 = fr.fname_in_outdir("bar", feature_recorder::NEXT_COUNT);
    REQUIRE(p1.filename() == "test_bar.txt");

    auto p2 = fr.fname_in_outdir("bar", feature_recorder::NEXT_COUNT);
    REQUIRE(p2.filename() == "test_bar_1.txt");

    fr.carve_mode = feature_recorder_def::CARVE_ALL;

    /* Check basic writing */
    auto sbuf0 = sbuf_t("0123456789");
    fr.write_buf(sbuf0, 2, 5);
    fr.flush();

    /* Now read the feature file and see if it is there */
    auto lines = getLines(sc.outdir / "test.txt");
    auto lastLine = getLast(lines);
    REQUIRE(lastLine == "2\t23456\t0123456789");

    /* check carving */
    auto sbuf1 = sbuf_t("Hello World!\n");
    fr.carve(sbuf1, ".carve_data");

    /* Check record record carving */
    auto hbuf1 = sbuf_t("Header\n");
    auto sbuf2 = sbuf_t("[record 001][record 002]");
    fr.carve(hbuf1, sbuf2, ".carve_records");
    std::cerr << "carved data in " << sc.outdir << "\n";

    /* Check children */
    REQUIRE( sbuf1.asString() == std::string("Hello World!\n"));
    REQUIRE( sbuf1.children == 0 );
    {
        auto sbuf1c = sbuf1.new_slice(1);
        REQUIRE( sbuf1.children == 1 );
        REQUIRE( sbuf1c->asString() == std::string("ello World!\n"));
        REQUIRE( sbuf1c->children == 0 );
        delete sbuf1c;
    }
    REQUIRE( sbuf1.children == 0 );

}

/** feature_recorder_file functions */
TEST_CASE("file_support","[feature_recorder_file]") {
    std::string line {"one\ttwo\tthree\\133"};
    std::string feature;
    std::string context;
    REQUIRE( feature_recorder_file::extract_feature_context(line, feature, context) == true);
    REQUIRE( feature=="two");
    REQUIRE( context=="three[");

    REQUIRE( feature_recorder_file::extract_feature_context("nothing", feature, context) == false);

}


/** test the path printer
 */
#include "path_printer.h"
TEST_CASE("functions", "[path_printer]") {
    REQUIRE(path_printer::lowerstr("ZIP") == "zip");

    std::string path {"100-ZIP-200-GZIP-300"};
    std::string part = path_printer::get_and_remove_token(path);
    REQUIRE(part=="100");
    REQUIRE(path=="ZIP-200-GZIP-300");

    part = path_printer::get_and_remove_token(path);
    REQUIRE(part=="ZIP");
    REQUIRE(path=="200-GZIP-300");

    part = path_printer::get_and_remove_token(path);
    REQUIRE(part=="200");
    REQUIRE(path=="GZIP-300");

    part = path_printer::get_and_remove_token(path);
    REQUIRE(part=="GZIP");
    REQUIRE(path=="300");

    part = path_printer::get_and_remove_token(path);
    REQUIRE(part=="300");
    REQUIRE(path=="");

    path = "PRINT";
    part = path_printer::get_and_remove_token(path);
    REQUIRE(part=="PRINT");
    REQUIRE(path=="");

    PrintOptions po;
    po.add_rfc822_header(std::cerr, "Content-Length: 100");
    part = po.get("Content-Length","200");
    REQUIRE(part=="100");

    part = po.get("Content-Color","red");
    REQUIRE(part=="red");
}



/** test the sbuf_stream interface.
 */
TEST_CASE("find_ngram_size", "[sbuf]") {
    auto sbuf1 = sbuf_t("Hello World!\n");
    REQUIRE( sbuf1.find_ngram_size(100) == 0);

    auto sbuf2 = sbuf_t("abcabcabcabc");
    REQUIRE( sbuf2.find_ngram_size(10) == 3);

    auto sbuf3 = sbuf_t("abcabcabcabc");
    REQUIRE( sbuf3.find_ngram_size(2) == 0); // if we don't search with a large enough probe, we won't find it.
}

TEST_CASE("distinct_characters", "[sbuf]") {
    auto sbuf1 = sbuf_t("Hello World!\n");
    REQUIRE( sbuf1.get_distinct_character_count() == 10 );
    REQUIRE( sbuf1.distinct_characters(0,13) == 10 );
    REQUIRE( sbuf1.distinct_characters(1,12) == 9 );
    REQUIRE( sbuf1.distinct_characters(0,12) == 9 );

    REQUIRE(sbuf1.getUTF8(4) == "o World!\n");

}

TEST_CASE("range_exception", "[sbuf]") {
    auto sbuf1 = sbuf_t("Hello World!\n");

    REQUIRE( sbuf1[100] == 0 );         // [] is safe
    REQUIRE_THROWS_AS( sbuf1.get8i(100), sbuf_t::range_exception_t); // get is not
    REQUIRE_THROWS_AS( sbuf1.get8uBE(100), sbuf_t::range_exception_t); // get is not
    REQUIRE_THROWS_AS( sbuf1.get16uBE(100), sbuf_t::range_exception_t); // get is not
    REQUIRE_THROWS_AS( sbuf1.get32uBE(100), sbuf_t::range_exception_t); // get is not
    try {
        std::cerr << sbuf1.get8i(100);
    }
    catch (const sbuf_t::range_exception_t &e) {
        REQUIRE( std::string("[sbuf_t::range_exception_t: Read past end of sbuf off=100 len=1]") == e.what());
    }
}

TEST_CASE("sbuf boundary contract", "[sbuf]") {
    const std::array<uint8_t, 8> bytes{{0x80, 0x01, 0x02, 0xff, 0x04, 0x05, 0x06, 0x87}};
    const std::array<uint8_t, 8> same = bytes;
    auto first_diff = bytes;
    first_diff[0] = 0x81;

    std::unique_ptr<sbuf_t> sbuf(sbuf_t::sbuf_new(pos0_t("bounds"), bytes.data(), bytes.size(), 4));

    SECTION("memcmp checks byte zero and accepts valid empty ranges") {
        REQUIRE(sbuf->memcmp(same.data(), 0, same.size()) == 0);
        REQUIRE(sbuf->memcmp(first_diff.data(), 0, first_diff.size()) != 0);
        REQUIRE(sbuf->memcmp(same.data(), 0, 0) == 0);
        REQUIRE(sbuf->memcmp(same.data(), sbuf->size(), 0) == 0);
        REQUIRE_THROWS_AS(sbuf->memcmp(same.data(), sbuf->size() + 1, 0), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->memcmp(same.data(), std::numeric_limits<size_t>::max(), 0), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->memcmp(same.data(), sbuf->size() - 1, 2), sbuf_t::range_exception_t);
    }

    SECTION("integer readers reject overflowed and one-past-end ranges") {
        REQUIRE(sbuf->get16u(6) == 0x8706);
        REQUIRE(sbuf->get16uBE(6) == 0x0687);
        REQUIRE(sbuf->get32u(0) == 0xff020180);
        REQUIRE(sbuf->get32uBE(0) == 0x800102ff);
        REQUIRE(sbuf->get64u(0) == 0x87060504ff020180ULL);
        REQUIRE(sbuf->get64uBE(0) == 0x800102ff04050687ULL);

        REQUIRE_THROWS_AS(sbuf->get8u(sbuf->size()), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->get16u(sbuf->size() - 1), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->get32u(sbuf->size() - 3), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->get64u(1), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->get32u(std::numeric_limits<size_t>::max()), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->get32uBE(std::numeric_limits<size_t>::max()), sbuf_t::range_exception_t);
    }

    SECTION("slice page sizes distinguish primary bytes from margins") {
        auto primary = sbuf->slice(1, 5);
        REQUIRE(primary.bufsize == 5);
        REQUIRE(primary.pagesize == 3);
        REQUIRE(primary.get_buf() == bytes.data() + 1);

        auto margin = sbuf->slice(4, 3);
        REQUIRE(margin.bufsize == 3);
        REQUIRE(margin.pagesize == 0);
        REQUIRE(margin.get_buf() == bytes.data() + 4);

        auto end = sbuf->slice(sbuf->size(), 0);
        REQUIRE(end.bufsize == 0);
        REQUIRE(end.pagesize == 0);
        REQUIRE(end.get_buf() == bytes.data() + bytes.size());

        REQUIRE_THROWS_AS(sbuf->slice(sbuf->size() + 1, 0), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->slice(1, std::numeric_limits<size_t>::max()), sbuf_t::range_exception_t);
        REQUIRE_THROWS_AS(sbuf->slice(std::numeric_limits<size_t>::max(), 0), sbuf_t::range_exception_t);
    }

    SECTION("clamping constructors never advance beyond the source") {
        sbuf_t child(*sbuf, std::numeric_limits<size_t>::max());
        REQUIRE(child.bufsize == 0);
        REQUIRE(child.pagesize == 0);
        REQUIRE(child.get_buf() == bytes.data() + bytes.size());

        sbuf_t bounded(*sbuf, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
        REQUIRE(bounded.bufsize == 0);
        REQUIRE(bounded.pagesize == 0);
        REQUIRE(bounded.get_buf() == bytes.data() + bytes.size());
    }
}

TEST_CASE("sbuf writable boundary", "[sbuf]") {
    std::unique_ptr<sbuf_t> sbuf(sbuf_t::sbuf_malloc(pos0_t("writable"), 2, 2));
    sbuf->wbuf(0, 0x12);
    sbuf->wbuf(1, 0x34);
    REQUIRE(sbuf->get16u(0) == 0x3412);
    REQUIRE_THROWS_AS(sbuf->wbuf(2, 0x56), std::runtime_error);
    REQUIRE_THROWS_AS(sbuf->wbuf(std::numeric_limits<size_t>::max(), 0x56), std::runtime_error);
}

void validate_file(std::filesystem::path path, std::string contents)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    REQUIRE (in.is_open());
    auto size = in.tellg();
    auto memblock = new char [size];
    in.seekg (0, std::ios::beg);
    in.read (memblock, size);
    in.close();
    std::string str(memblock,size);
    REQUIRE(str == contents);
    delete[] memblock;
}

TEST_CASE("sbuf_write", "[sbuf]") {
    auto sbuf1 = sbuf_t("0123456789");
    std::filesystem::path td = get_tempdir().string();
    std::filesystem::path fname = td / "file.txt";
    int fd = open(fname.string().c_str(), O_RDWR | O_CREAT | O_BINARY, 0777);
    REQUIRE(sbuf1.write(fd, 1, 4) == 4);
    close(fd);
    validate_file(fname, "1234");
    std::filesystem::remove(fname);

    FILE *f = fopen(fname.string().c_str(), "wb");
    REQUIRE(sbuf1.write(f, 2, 4) == 4);
    fclose(f);
    validate_file(fname, "2345");
    std::filesystem::remove(fname);

    REQUIRE(sbuf1.write(fname) == 10);
    validate_file(fname, "0123456789");
    std::filesystem::remove(fname);

}

TEST_CASE("sbuf_stream", "[sbuf]") {
    auto sbuf1 = sbuf_t("\001\002\003\004\005\006\007\010");
    REQUIRE( sbuf1.bufsize == 8);
    auto sbs = sbuf_stream( sbuf1 );

    REQUIRE( sbs.tell() == 0 );
    REQUIRE( sbs.get8u() == 1);
    REQUIRE( sbs.get8u() == 2);
    REQUIRE( sbs.get8u() == 3);
    REQUIRE( sbs.get8u() == 4);
    REQUIRE( sbs.tell()  == 4 );

    REQUIRE( sbs.get16u() == 0x0605);   // ah, LE
    REQUIRE( sbs.get16u() == 0x0807);
    REQUIRE( sbs.tell() == 8 );

    sbs.seek(4);
    REQUIRE( sbs.get32u() == 0x08070605 );
    REQUIRE( sbs.tell() == 8 );

    sbs.seek(4);
    REQUIRE( sbs.get8uBE() == 0x05);
    REQUIRE( sbs.tell() == 5 );

    sbs.seek(4);
    REQUIRE( sbs.get16uBE() == 0x0506);
    REQUIRE( sbs.tell() == 6 );

    sbs.seek(4);
    REQUIRE( sbs.get32uBE() == 0x05060708);
    REQUIRE( sbs.tell() == 8 );

    sbs.seek(0);
    REQUIRE( sbs.get64u() == 0x0807060504030201L );
    REQUIRE( sbs.tell() == 8 );

    sbs.seek(0);
    REQUIRE( sbs.get64uBE() == 0x0102030405060708L );
    REQUIRE( sbs.tell() == 8 );

    sbs.seek(4);
    REQUIRE( sbs.get16iBE() == 0x0506);
    REQUIRE( sbs.tell() == 6 );

    REQUIRE( sbs.get16iBE() == 0x0708);
    REQUIRE( sbs.tell() == 8 );

    sbs.seek(4);
    REQUIRE( sbs.get32iBE() == 0x05060708 );
    REQUIRE( sbs.tell() == 8 );

    sbs.seek(0);
    REQUIRE( sbs.get64iBE() == 0x0102030405060708L );
    REQUIRE( sbs.tell() == 8 );

}


/****************************************************************
 * feature_recorder_set.h
 *
 * Create a simple set and write two features and make a histogram.
 */
TEST_CASE("feature_recorder_def", "[feature_recorder_set]") {
    feature_recorder_def d1("test1");
    feature_recorder_def d2("test1");
    feature_recorder_def d3("test1");
    d3.flags.xml = true;
    feature_recorder_def d4("test4");

    REQUIRE(d1.name == "test1");
    REQUIRE(d1 == d2);
    REQUIRE(d1.name == d3.name);
    REQUIRE(d1.flags != d3.flags);
    REQUIRE(d1 != d3);
}

TEST_CASE("write_features", "[feature_recorder_set]") {

    // Create a random directory for the output of the feature recorder
    std::filesystem::path tempdir = get_tempdir();
    {
        feature_recorder_set::flags_t flags;
        flags.no_alert = true;
        flags.debug = true;

        feature_recorder_set fs(flags, scanner_config());
        feature_recorder& fr = fs.create_feature_recorder("feature_file");

        /* Make sure requesting a valid name does not generate an exception and an invalid name generates an exception
         */
        REQUIRE_NOTHROW(fs.named_feature_recorder("feature_file"));
        REQUIRE_THROWS_AS(fs.named_feature_recorder("test_not"), feature_recorder_set::NoSuchFeatureRecorder);

        /* Ask the feature recorder to create a histogram */
        histogram_def h1("name", "feature_file", "([0-9]+)", "", "suffix1", histogram_def::flags_t());
        REQUIRE(fs.histogram_count() == 0);
        fs.histogram_add(h1);
        REQUIRE(fs.histogram_count() == 1);

        REQUIRE(pos0_t::calc_depth("0")==0);
        REQUIRE(pos0_t::calc_depth("0-OUTLOOK")==1);
        REQUIRE(pos0_t::calc_depth("0-OUTLOOK-0")==1);
        REQUIRE(pos0_t::calc_depth("0-OUTLOOK-0-XOR(255)")==2);

        pos0_t p;
        fr.write(p, "one", "context");
        fr.write(p + 5, "one", "context");
        fr.write(p + 10, "two", "context");

        const sbuf_t* sb16 = hello16_sbuf();
        REQUIRE(sb16->size()-1 == strlen(hello8) * 2); // -1 to remove the \000
        delete sb16;
    }
    std::vector<std::string> lines = getLines(tempdir / "histogram1.txt");
    REQUIRE( lines[0] == "n=4\t300\t(utf16=1)");
    REQUIRE( lines[1] == "n=2\t200");
    REQUIRE( lines[2] == "n=1\t100");
}

/****************************************************************
 * char_class.h
 */
#include "char_class.h"
TEST_CASE("char_class", "[char_class]") {
    CharClass c;
    c.add('a');
    c.add('0');
    REQUIRE(c.range_A_Fi == 1);
    REQUIRE(c.range_g_z == 0);
    REQUIRE(c.range_G_Z == 0);
    REQUIRE(c.range_0_9 == 1);

    c.add(reinterpret_cast<const uint8_t*>("ab"), 2);
    REQUIRE(c.range_A_Fi == 3);
    REQUIRE(c.range_g_z == 0);
    REQUIRE(c.range_G_Z == 0);
    REQUIRE(c.range_0_9 == 1);
}

/****************************************************************
 *
 * pos0.h:
 */

#include "pos0.h"
TEST_CASE("pos0_t", "[feature_recorder]") {
    REQUIRE(stoi64("12345") == 12345);

    pos0_t p0("10000-hello-200-bar", 300);
    pos0_t p1("10000-hello-200-bar", 310);
    pos0_t p2("10000-hello-200-bar", 310);
    REQUIRE(p0.path == "10000-hello-200-bar");
    REQUIRE(p0.offset == 300);
    REQUIRE(p0.isRecursive() == true);
    REQUIRE(p0.firstPart() == "10000");
    REQUIRE(p0.lastAddedPart() == "bar");
    REQUIRE(p0.alphaPart() == "hello/bar");
    REQUIRE(p0.imageOffset() == 10000);
    REQUIRE(p0 + 10 == p1);
    REQUIRE(p0 < p1);
    REQUIRE(p1 > p0);
    REQUIRE(p0 != p1);
    REQUIRE(p1 == p2);
}

/****************************************************************
 * regex_vector.h & regex_vector.cpp
 * Previously tested all three regex engines. Now just tests RE_ENGINE=RE2
 */
#include "regex_vector.h"
char disable[64];
TEST_CASE("test regex_vector", "[regex]") {
    REQUIRE(regex_vector::has_metachars("this[1234]foo") == true);
    REQUIRE(regex_vector::has_metachars("this(1234)foo") == true);
    REQUIRE(regex_vector::has_metachars("this[1234].*foo") == true);
    REQUIRE(regex_vector::has_metachars("this1234foo") == false);

    for(int pass=0;pass<1;pass++){
        if (pass==0) {
            strncpy(disable,"RE_ENGINE=RE2",sizeof(disable));
            putenv(disable);
        }

        regex_vector rv;
        std::cout << "Pass " << pass << " testing regex engine: " <<  rv.regex_engine() << std::endl;

        REQUIRE(rv.size() == 0);
        rv.push_back("this.*");
        REQUIRE(rv.size() == 1);
        rv.clear();			// test this functionality
        REQUIRE(rv.size() == 0);

        // Check a simple vector with three regular expressions
        rv.push_back("this.*");
        rv.push_back("check[1-9]");
        rv.push_back("thing");

        REQUIRE(rv.size() == 3);

        std::string found;
        REQUIRE(rv.search_all("hello1", &found) == false);
        REQUIRE(rv.search_all("check1", &found) == true);
        REQUIRE(found == "check1");

        REQUIRE(rv.search_all("before check2 after", &found) == true);
        REQUIRE(found == "check2");

        rv.clear();
        rv.push_back("[a-z]*@company.com");

        /* This tests to make sure searching for a potentially open-ended string doesn't
         * take a very long time. We do the search in a 30MiB array...
         */
        const size_t mbytes = 1;
        const size_t bytes = 1024*1024*mbytes;
        std::string bigstring = std::string(bytes,'a')
            + " user@company.com "
            + std::string(1024*1024*2,'b');
        found="";
        size_t offset = 0;
        size_t len = 0;
        aftimer t;
        t.start();
        std::thread([&] {
            std::this_thread::sleep_for(std::chrono::seconds(6000));
            std::cerr << "Timeout hit!" << std::endl;
            REQUIRE(false);  // Tell catch that we failed.
        }).detach();
        REQUIRE(rv.search_all(bigstring, &found, &offset, &len) == true);
#ifndef _WIN32
alarm(0);
#endif
        t.stop();
        std::cout << "time=" << t.elapsed_seconds() << "\n";
        REQUIRE(found == "user@company.com");
        REQUIRE(offset == bytes+1);
        REQUIRE(len == 16 );
    }
    strncpy(disable,"RE_ENGINE=RE2",sizeof(disable));
    putenv(disable);
}

/****************************************************************
 *
 * sbuf.h
 */

TEST_CASE("hello_sbuf", "[sbuf]") {
    sbuf_t sb1 = sbuf_t(hello8);
    REQUIRE(sb1.size() == strlen(hello8));
    REQUIRE(sb1.offset(reinterpret_cast<const uint8_t*>(hello8) + 2) == 2);
    REQUIRE(sb1.asString() == std::string("Hello world!"));
    REQUIRE(sb1.get8uBE(0) == 'H');
    REQUIRE(sb1.get8uBE(1) == 'e');
    REQUIRE(sb1.get8uBE(2) == 'l');
    REQUIRE(sb1.get8uBE(3) == 'l');
    REQUIRE(sb1.get8uBE(4) == 'o');
    REQUIRE(sb1.find('H', 0) == 0);
    REQUIRE(sb1.find('e', 0) == 1);
    REQUIRE(sb1.find('o', 0) == 4);
    REQUIRE(sb1.find("world") == 6);

    REQUIRE(sb1.has_hash()==false);
    REQUIRE(sb1.hash() != "");
    REQUIRE(sb1.hash() == sbuf_t(hello8).hash());
    REQUIRE(sb1.has_hash()==true);

    std::string s = sb1.getUTF8(6, 5);
    REQUIRE(s == "world");

    sbuf_t sb2("This\nis\na\ntest");
    size_t pos = 0;
    size_t line_start = 0;
    size_t line_len = 0;

    /* "This */
    REQUIRE(sb2.getline(pos, line_start, line_len) == true);
    REQUIRE(line_start == 0);
    REQUIRE(line_len == 4);

    /* "is" */
    REQUIRE(sb2.getline(pos, line_start, line_len) == true);
    REQUIRE(line_start == 5);
    REQUIRE(line_len == 2);

    /* "a" */
    REQUIRE(sb2.getline(pos, line_start, line_len) == true);
    REQUIRE(line_start == 8);
    REQUIRE(line_len == 1);

    /* "test" */
    REQUIRE(sb2.getline(pos, line_start, line_len) == true);
    REQUIRE(line_start == 10);
    REQUIRE(line_len == 4);

    REQUIRE(sb2.getline(pos, line_start, line_len) == false);

    sbuf_t* sb3 = sb1.new_slice(6, 5);
    REQUIRE(sb3->asString() == "world");
    REQUIRE(sb1.has_parent() == false);
    REQUIRE(sb3->has_parent() == true);
    REQUIRE(sb3->highest_parent() == &sb1);

    delete sb3;

    auto sb4 = sbuf_t(sb1,6);
    REQUIRE(sb4.asString() == "world!");
    auto sb5 = sbuf_t(sb1,100);
    REQUIRE(sb5.asString() == "");
    auto sb6 = sbuf_t(sb1,6,5);
    REQUIRE(sb6.asString() == "world");
    auto sb7 = sbuf_t(sb1,100,5);
    REQUIRE(sb7.asString() == "");
}

/* Make sure we can allocate writable space in a sbuf. */
TEST_CASE("sbuf_malloc", "[sbuf]") {
    const size_t BUFSIZE=256;
    sbuf_t *sb1 = sbuf_t::sbuf_malloc(pos0_t(), BUFSIZE, BUFSIZE);
    for(u_int i=0; i<BUFSIZE; i++){
        sb1->wbuf(i, 255-i);
    }
    REQUIRE_THROWS_AS( sb1->wbuf(600,0), std::runtime_error);

    REQUIRE((*sb1)[100]==155);
    REQUIRE((*sb1)[150]==105);

    std::stringstream ss;
    sb1->hex_dump(ss);
    REQUIRE(ss.str().find("00a0: 5f5e 5d5c") != std::string::npos);

    {
        sbuf_t sb1b = sb1->slice(100);
        REQUIRE(sb1->children==1);      // make sure child is registered
        REQUIRE(sb1b.children==0);
        REQUIRE(sb1b[0]==155);
        REQUIRE(sb1b[1]==154);
        REQUIRE_THROWS_AS( sb1b.wbuf(0,0), std::runtime_error); // slices are not writable
    }
    REQUIRE(sb1->children==0);          // make sure child is gone.

    sbuf_t *sb1c = sb1->new_slice_copy(100,5);
    REQUIRE(sb1c->bufsize==5);
    REQUIRE(sb1->children==0);
    delete sb1c;

    // Set 100..200 t be 55
    for(int i=100; i< 200; i++) {
        sb1->wbuf(i, 55);
    }
    REQUIRE( sb1->is_constant(0, 100, 55) == false );
    REQUIRE( sb1->is_constant(100, 100, 55) == true );
    REQUIRE( sb1->find(150, 0) == -1 );
    std::stringstream ss2;
    ss2 << *sb1;
    REQUIRE( ss2.str().find("children=0") != std::string::npos );
    delete sb1;


    std::string abc {"abcdefghijklmnopqrstuvwxyz"};
    sbuf_t *sb2 = sbuf_t::sbuf_malloc(pos0_t(), abc);
    REQUIRE((*sb2)[0]=='a');
    REQUIRE((*sb2)[9]=='j');
    REQUIRE((*sb2)[25]=='z');

    REQUIRE( sb2->substr(2,4) == "cdef" );

    sb2 = sb2->realloc(10);
    REQUIRE(sb2->bufsize==10);
    delete sb2;
}

TEST_CASE("sbuf_malloc2", "[sbuf]") {
    sbuf_t root("<b>this is bold</b><i>this is itallics</i>");
    std::stringstream ss;

    for(int i=0;i<100;i++){
        root.raw_dump(ss, 0, root.bufsize);
        ss << root;
    }
    std::string bufstr = ss.str();
    auto *dbuf = sbuf_t::sbuf_malloc(root.pos0+"MSXML", bufstr.size(), bufstr.size());
    memcpy(dbuf->malloc_buf(), bufstr.c_str(), bufstr.size());
    delete dbuf;
}

TEST_CASE("map_file", "[sbuf]") {
    std::filesystem::path tempdir = get_tempdir();
    std::ofstream os;
    std::filesystem::path fname = tempdir / "hello.txt";

    os.open(fname);
    REQUIRE(os.is_open());
    os << hello8;
    os.close();

    auto sb1p = sbuf_t::map_file(fname);
    auto& sb1 = *sb1p;
    REQUIRE(sb1.bufsize == strlen(hello8));
    REQUIRE(sb1.bufsize == sb1.pagesize);
    for (int i = 0; hello8[i]; i++) { REQUIRE(hello8[i] == sb1[i]); }
    REQUIRE(sb1[-1] == '\000');
    REQUIRE(sb1[1000] == '\000');
    delete sb1p;
}

/****************************************************************
 * scanner_config.h:
 * holds the name=value configurations for all scanners.
 * A scanner_config is required to make a scanner_set.
 * The scanner_set then loads the scanners and creates a feature recorder set, which
 * holds the feature recorders. THey are called with the default values.
 */
#include "scanner_config.h"
TEST_CASE("scanner_config", "[scanner]") {
    std::string help_expected {
        "   -S first-day=sunday    value for first-day (first-day)\n"
        "   -S age=0    age in years (age)\n"
    };
    scanner_config sc;
    /* Normally the set_configs would be called by main()
     * These two would be called by -S first-day=monday  -S age=5
     */
    sc.set_config("first-day", "monday");
    sc.set_config("age", "5");

    std::string val{"sunday"};
    sc.get_global_config("first-day", &val, "value for first-day");
    REQUIRE(val == "monday");

    uint64_t ival{0};
    sc.get_global_config("age", &ival, "age in years");
    REQUIRE(ival == 5);
    REQUIRE(sc.get_help() == help_expected);
    REQUIRE_THROWS_AS(sc.get_global_config<int>("invalid", nullptr, "invalid"), std::invalid_argument);

    sc.push_scanner_command("scanner1", scanner_config::scanner_command::ENABLE);
    sc.push_scanner_command("scanner2", scanner_config::scanner_command::DISABLE);
    REQUIRE(sc.get_scanner_commands().size() == 2);
}

/****************************************************************
 * scanner_params.h:
 * The interface used by scanners.
 */
#include "scanner_params.h"
TEST_CASE("scanner", "[scanner]") {
    scanner_config sc;
    scanner_params sp(sc, nullptr, nullptr, scanner_params::PHASE_INIT, nullptr);
    REQUIRE_THROWS_AS(sp.get_scanner_config<int>("invalid", nullptr, "invalid"), std::invalid_argument);
}

/****************************************************************
 * scanner_set.h:
 * Creates a place to hold all of the scanners.
 * The be20_api contains a single scanner for testing purposes:
 * scan_null, the null scanner, which writes metadata into a version.txt feature file.
 */
#include "scan_sha1_test.h"
#include "scanner_set.h"
#include "machine_stats.h"

#ifdef _WIN32
TEST_CASE("machine_stats", "[!mayfail][machine_stats]") {
    WARN("machine_stats not implemented on Windows (ps and /proc/ don't exist)");
}
#else
TEST_CASE("machine_stats", "[machine_stats]") {
    REQUIRE(machine_stats::get_available_memory() != 0);
    REQUIRE(machine_stats::get_cpu_percentage() >= 0);
    uint64_t virtual_size = 0;
    uint64_t resident_size = 0;
    machine_stats::get_memory(&virtual_size, &resident_size);
    std::cerr << "virtual_size: " << virtual_size << " resident_size: " << resident_size << std::endl;
    REQUIRE(virtual_size > 0);
    REQUIRE(resident_size > 0);
}
#endif


/* Just make sure that they can be created and deleted without error.
 * Previously we got errors before we moved the destructor to the .cpp file from the .h file.
 */
TEST_CASE("scanner_stats", "[scanner_set]") {
    scanner_config sc;
    feature_recorder_set::flags_t flags;

    feature_recorder_set fs(flags, sc);
    std::map<scanner_t*, const struct scanner_params::scanner_info*> scanner_info_db{};
    atomic_set<std::string> seen_set {}; // hex hash values of sbuf pages that have been seen
    auto ss = new scanner_set(sc, feature_recorder_set::flags_t(), nullptr);
    delete ss;
}

TEST_CASE("previously_processed", "[scanner_set]") {
    scanner_config sc;
    feature_recorder_set::flags_t f;
    scanner_set ss(sc, f, nullptr);
    sbuf_t slg("Simson");
    REQUIRE(ss.previously_processed_count(slg) == 0);
    REQUIRE(ss.previously_processed_count(slg) == 1);
    REQUIRE(ss.previously_processed_count(slg) == 2);
}

#if 0
TEST_CASE("mt_previously_processed", "[scanner_set]") {
    scanner_config sc;
    feature_recorder_set::flags_t f;
    scanner_set ss(sc, f, nullptr);
    sbuf_t slg("Simson");
    ss.info();
    REQUIRE(ss.previously_processed_count(slg) == 0);
    REQUIRE(ss.previously_processed_count(slg) == 1);
    REQUIRE(ss.previously_processed_count(slg) == 2);
}
#endif

TEST_CASE("enable/disable", "[scanner_set]") {
    scanner_config sc;
    sc.outdir = get_tempdir();

    /* Enable the scanner */
    const std::string SHA1_TEST = "sha1_test";

    {
        sc.enable_all_scanners();
        scanner_set ss(sc, feature_recorder_set::flags_t(), nullptr);
        ss.add_scanner(scan_sha1_test);
        ss.apply_scanner_commands(); // applied after all scanners are added

        /*  Make sure that the scanner was added  */
        REQUIRE_NOTHROW(ss.get_scanner_by_name("sha1_test"));
        REQUIRE(ss.get_scanner_by_name("sha1_test") == scan_sha1_test);
        REQUIRE_THROWS_AS(ss.get_scanner_by_name("no_such_scanner"), scanner_set::NoSuchScanner);

        /* Make sure that the sha1 scanner is enabled and only the sha1 scanner is enabled */
        REQUIRE(ss.is_scanner_enabled(SHA1_TEST) == true);
        REQUIRE(ss.is_find_scanner_enabled() == false); // only sha1 scanner is enabled

        /* Make sure that the scanner set has a two feature recorders:
         * the alert recorder and the sha1_bufs recorder
         */
        REQUIRE(ss.feature_recorder_count() == 2);

        /* Make sure it has a single histogram */
        REQUIRE(ss.histogram_count() == 1);
    }
    {
        /* Try it again, but this time turning on all of the commands */
        sc.enable_all_scanners();
        scanner_set ss(sc, feature_recorder_set::flags_t(), nullptr);
        ss.add_scanner(scan_sha1_test);
        ss.apply_scanner_commands(); // applied after all scanners are adde
        REQUIRE(ss.is_scanner_enabled(SHA1_TEST) == true);
    }
    {
        /* Try it again, but this time turning on the scanner, and then turning all off*/
        sc.push_scanner_command(SHA1_TEST, scanner_config::scanner_command::ENABLE);
        sc.disable_all_scanners();
        scanner_set ss(sc, feature_recorder_set::flags_t(), nullptr);
        ss.add_scanner(scan_sha1_test);
        ss.apply_scanner_commands(); // applied after all scanners are adde
        REQUIRE(ss.is_scanner_enabled(SHA1_TEST) == false);
    }
}

/* This test runs a scan on the hello_sbuf() with the sha1 scanner. */
TEST_CASE("run", "[scanner]") {
    sbuf_t::debug_leak = true;
    sbuf_t::debug_alloc = true;
    scanner_config sc;
    sc.outdir = get_tempdir();
    auto dfxml_file = sc.outdir / "report.xml";
    sc.push_scanner_command(std::string("sha1_test"), scanner_config::scanner_command::ENABLE); /* Turn it onn */

    scanner_set ss(sc, feature_recorder_set::flags_t(), nullptr);
    ss.add_scanner(scan_sha1_test);
    ss.apply_scanner_commands();
    REQUIRE(ss.get_scanner_name(scan_sha1_test) == "sha1_test");

    /* Make sure scanner is enabled */
    std::stringstream s2;
    ss.info_scanners(s2, true, true, 'e', 'x');
    auto s2str = s2.str();
    REQUIRE(s2str.find("disable scanner sha1") > 0);
    REQUIRE(s2str.find("enable scanner sha1") == std::string::npos);

    /* Make sure we got a sha1_test feature recorder */
    feature_recorder& fr = ss.named_feature_recorder("sha1_bufs");

    REQUIRE(fr.name == "sha1_bufs");

    /* Check to see if the histogram of the first 5 characters of the SHA1 code of each buffer was properly created. */
    /* There should be 1 histogram */
    REQUIRE(ss.histogram_count() == 1);
    REQUIRE(fr.histogram_count() == 1);
    /* It should be a regular expression that extracts the first 5 characters */
    /* Test the regular expression */
    /* And it should write to a feature file that has a suffix of "_first5" */

    feature_recorder_file *frp = reinterpret_cast<feature_recorder_file *>(&fr);

    REQUIRE(frp->histograms[0]->def.feature == "sha1_bufs");
    REQUIRE(frp->histograms[0]->def.pattern == "^(.....)");
    REQUIRE(frp->histograms[0]->def.suffix == "first5");
    REQUIRE(frp->histograms[0]->def.flags.lowercase == true);
    REQUIRE(frp->histograms[0]->def.flags.numeric == false);

    /* Test the hasher */
    sbuf_t* hello = new sbuf_t(hello8);
    std::string hashed = fr.hash(*hello);

    /* Set up a DFXML output */
    dfxml_writer w( dfxml_file, false);
    ss.set_dfxml_writer( &w );
    REQUIRE( ss.get_dfxml_writer() == &w);
    ss.dump_enabled_scanner_config();

    /* Perform a simulated scan */
    ss.phase_scan();         // start the scanner phase
    ss.schedule_sbuf(hello); // process a single sbuf, and delete it
    ss.shutdown();           // shutdown; this will write out the in-memory histogram.
    ss.dump_scanner_stats();

    auto enabled_scanners = ss.get_enabled_scanners();
    REQUIRE( enabled_scanners.size() == 1 );

    /* Make sure that the feature recorder output was created */
    std::vector<std::string> lines;
    std::filesystem::path fname_fr = get_tempdir() / "sha1_bufs.txt";

    lines = getLines(fname_fr);
    REQUIRE(lines[0] == "# BANNER FILE NOT PROVIDED (-b option)");
    REQUIRE(lines[1][0] == '#');
    REQUIRE(lines[2] == "# Feature-Recorder: sha1_bufs");
    REQUIRE(lines[3] == "# Filename: <NO-INPUT>");
    REQUIRE(lines[4] == "# Feature-File-Version: 1.1");
    REQUIRE(lines[5] == "0\td3486ae9136e7856bc42212385ea797094475802");
    REQUIRE(lines.size() == 6);

    /* The sha1 scanner makes a single histogram. Make sure we got it. */
    REQUIRE(ss.histogram_count() == 1);
    std::filesystem::path fname_hist = get_tempdir() / "sha1_bufs_first5.txt";
    lines = getLines(fname_hist);
    REQUIRE(lines.size() == 6);         // includes header!
    sbuf_t::debug_leak  = false;
    sbuf_t::debug_alloc = false;

}

/****************************************************************
 * test the path printer with a scanner set...
 */
#include "test_image_reader.h"
TEST_CASE("test_image_reader", "[path_printer]"){
    test_image_reader p;
    REQUIRE(p.image_size() == 256);
    char buf[1024];
    REQUIRE(p.pread(buf, sizeof(buf), 0) == 256);
    REQUIRE(p.pread(buf, sizeof(buf), 250) == 6);
    REQUIRE(p.pread(buf, sizeof(buf), 256) == 0);
    REQUIRE(p.pread(buf, sizeof(buf), 1000) == 0);

    REQUIRE(p.pread(buf, 10, 0) == 10);
    REQUIRE(p.pread(buf, 10, 250) == 6);
    REQUIRE(p.pread(buf, 10, 256) == 0);
    REQUIRE(p.pread(buf, 10, 1000) == 0);
}

std::string hexdump=
    "0000: 0001 0203 0405 0607 0809 0a0b 0c0d 0e0f 1011 1213 1415 1617 1819 1a1b 1c1d 1e1f ................................\n"
    "0020: 2021 2223 2425 2627 2829 2a2b 2c2d 2e2f 3031 3233 3435 3637 3839 3a3b 3c3d 3e3f  !\"#$%&'()*+,-./0123456789:;<=>?\n"
    "0040: 4041 4243 4445 4647 4849 4a4b 4c4d 4e4f 5051 5253 5455 5657 5859 5a5b 5c5d 5e5f @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n"
    "0060: 6061 6263 6465 6667 6869 6a6b 6c6d 6e6f 7071 7273 7475 7677 7879 7a7b 7c7d 7e7f `abcdefghijklmnopqrstuvwxyz{|}~.\n"
    "0080: 8081 8283 8485 8687 8889 8a8b 8c8d 8e8f 9091 9293 9495 9697 9899 9a9b 9c9d 9e9f ................................\n"
    "00a0: a0a1 a2a3 a4a5 a6a7 a8a9 aaab acad aeaf b0b1 b2b3 b4b5 b6b7 b8b9 babb bcbd bebf ................................\n"
    "00c0: c0c1 c2c3 c4c5 c6c7 c8c9 cacb cccd cecf d0d1 d2d3 d4d5 d6d7 d8d9 dadb dcdd dedf ................................\n"
    "00e0: e0e1 e2e3 e4e5 e6e7 e8e9 eaeb eced eeef f0f1 f2f3 f4f5 f6f7 f8f9 fafb fcfd feff ................................\n";

TEST_CASE("printer","[path_printer]"){
    scanner_config sc;
    sc.outdir = get_tempdir();
    scanner_set ss(sc, feature_recorder_set::flags_t(), nullptr);
    ss.apply_scanner_commands();
    std::stringstream mem_stream;

    // Make sure that the path_printer stops at the end of the image

    test_image_reader *p = new test_image_reader();
    path_printer pp( ss, p, mem_stream);
    pp.process_path( "0-PRINT/r" );

    REQUIRE( mem_stream.str().size() == std::string("256\r\n").size() + p->image_size());

    // Now try the other variants
    mem_stream.str("");
    pp.process_path( "0-PRINT/h" );
    REQUIRE( mem_stream.str() == hexdump );
    delete p;
}

/****************************************************************
 *  word_and_context_list.h
 */
#include "word_and_context_list.h"
TEST_CASE("word_and_context_list", "[feature_recorder]") {
    REQUIRE(word_and_context_list::rstrcmp("aaaa1", "bbbb0") < 0);
    REQUIRE(word_and_context_list::rstrcmp("aaaa1", "aaaa1") == 0);
    REQUIRE(word_and_context_list::rstrcmp("bbbb0", "aaaa1") > 0);

    auto tmpdir = NamedTemporaryDirectory();
    std::filesystem::path words_txt = tmpdir / "words.txt";
    std::filesystem::path bogus_txt = tmpdir / "bogus.txt";
    std::ofstream os;
    os.open(words_txt);
    REQUIRE(os.is_open());
    os << "# This is a comment\n";

    // Three features with no context
    os << "word1\n";
    os << "word2\n";
    os << "word3\n";

    // And a simple regular expression
    os << "letters?\n";

    os.close();
    word_and_context_list wcl;
    REQUIRE(wcl.readfile(bogus_txt) == -1);
    REQUIRE(wcl.readfile(words_txt) == 0);
    REQUIRE(wcl.size() == 4);

    std::filesystem::remove(words_txt);
    std::filesystem::remove_all(tmpdir);
}


/****************************************************************
 * unicode_escape.h
 * unicode_escape.cpp
 * Our test suite for the utf8 package and our uses of it.
 */
#include "unicode_escape.h"
TEST_CASE("unicode_escape", "[unicode]") {
    const char* U1F601 = "\xF0\x9F\x98\x81"; // UTF8 for Grinning face with smiling eyes
    REQUIRE(octal_escape('a') == "\\141");         // escape the escape!
    REQUIRE(utf8cont('a') == false);
    REQUIRE(utf8cont(U1F601[0]) == false); // the first character is not a continuation character
    REQUIRE(utf8cont(U1F601[1]) == true);  // the second is
    REQUIRE(utf8cont(U1F601[2]) == true);  // the third is
    REQUIRE(utf8cont(U1F601[3]) == true);  // the fourth is
    REQUIRE(valid_utf8codepoint(0x01) == true);
    REQUIRE(valid_utf8codepoint(0xffff) == false);

    std::string hellos("hello");
    /* Try all combinations with valid UTF8 */
    for (int a = 0; a < 2; a++) {
        for (int b = 0; b < 2; b++) {
            for (int c = 0; c < 2; c++) { REQUIRE(validateOrEscapeUTF8(hellos, a, b, c) == hellos); }
        }
    }
    // Below is confusing because \\ is turned into \ in C++
    REQUIRE(validateOrEscapeUTF8("backslash=\\", false, true, false) == "backslash=\\134");

    /* Try a round-trips */
    std::u32string u32s = U"我想玩";
    REQUIRE(convert_utf8_to_utf32(convert_utf32_to_utf8(u32s)) == u32s);

    // Handle embedded NULLs and Control-A
    std::string test1 {"aab"};
    test1[1]='\001';
    REQUIRE(validateOrEscapeUTF8( test1, true, false, false)  == "a\\001b");
    REQUIRE(validateOrEscapeUTF8( test1, true, true, false)   == "a\\001b");

    //REQUIRE_THROWS_AS( validateOrEscapeUTF8( test1, false, false, true), BadUnicode );
}

TEST_CASE("unicode_detection1", "[unicode]") {
    auto sb16p = sbuf_t::map_file(tests_dir() / "unilang.htm");
    auto& sb16 = *sb16p;

    bool little_endian = false;
    bool t16 = looks_like_utf16(sb16.asString(), little_endian);
    REQUIRE(t16 == true);
    REQUIRE(little_endian == true);
    delete sb16p;

    auto sb8p = sbuf_t::map_file(tests_dir() / "unilang8.htm");
    auto& sb8 = *sb8p;
    bool t8 = looks_like_utf16(sb8.asString(), little_endian);
    REQUIRE(t8 == false);
    delete sb8p;
}

TEST_CASE("unicode_detection2", "[unicode]") {
    char c[] {"h\000t\000t\000p\000:\000/\000/\000w\000w\000w\000.\000h\000h\000s\000.\000g\000o\000v\000/\000o\000"
            "c\000r\000/\000h\000i\000p\000a\000a\000/\000c\000o\000n\000s\000u\000m\000"
            "e\000r\000_\000r\000i\000g\000h\000t\000s\000.\000p\000d\000f\000"};
    std::string str(c,sizeof(c)-1);     // phantum null at end
    std::string utf8 {"http://www.hhs.gov/ocr/hipaa/consumer_rights.pdf"};

    REQUIRE( utf8.size()==48 );

#if 0
    for (size_t i = 0; i + 1 < str.size(); i += 2) {
        std::cerr << "str[" << i << "]= " << (int)str[i]
                  << " " << str[i] << "   str[" << i+1 << "]="
                  << (int)str[i+1] << " " << str[i+1] << "\n";
        /* TODO: Should we look for FFFE or FEFF and act accordingly ? */
    }
#endif

    REQUIRE(sizeof(c)==97);             // 90 bytes above
    REQUIRE(strlen(c)==1);              // but the second byte is a NULL
    REQUIRE(str.size()==96);

    /* validate the string */
    for(size_t i=0;i<utf8.size();i++){
        REQUIRE( utf8[i] == str[i*2] );
        REQUIRE( str[i*2+1] == 0);
    }

    bool little_endian = false;
    bool r = looks_like_utf16( str, little_endian );
    REQUIRE( r == true );
    REQUIRE( little_endian == true);
}


TEST_CASE("directory_support", "[utilities]") {
    auto tmpdir = NamedTemporaryDirectory();
    REQUIRE(std::filesystem::is_directory("no such directory") == false);
    REQUIRE(std::filesystem::is_directory(tmpdir) == true);
    REQUIRE(directory_empty(tmpdir) == true);
    std::filesystem::path foo_txt = tmpdir / "foo.txt";
    std::ofstream os;
    os.open(foo_txt);
    REQUIRE(os.is_open());
    os << "foo\n";
    os.close();
    REQUIRE(directory_empty(tmpdir) == false);
    std::filesystem::remove(foo_txt);
    std::filesystem::remove_all(tmpdir);
}

TEST_CASE("Show_output", "[end]") {
    std::string cmd = "ls -l " + get_tempdir().u8string();
    std::cout << cmd << "\n";
    int ret = system(cmd.c_str());
    if (ret != 0) { throw std::runtime_error("could not list tempdir???"); }
}
