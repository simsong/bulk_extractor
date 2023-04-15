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

#include "be20_api/catch.hpp"

#ifdef HAVE_MACH_O_DYLD_H
#include "mach-o/dyld.h"         // Needed for _NSGetExecutablePath
#endif

#include "dfxml_cpp/src/dfxml_writer.h"
#include "be20_api/path_printer.h"
#include "be20_api/scanner_set.h"
#include "be20_api/utils.h"             // needs config.h

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

const std::string JSON1 {"[{\"1\": \"one@company.com\"}, {\"2\": \"two@company.com\"}, {\"3\": \"two@company.com\"}]"};
const std::string JSON2 {"[{\"1\": \"one@base64.com\"}, {\"2\": \"two@base64.com\"}, {\"3\": \"three@base64.com\"}]\n"};

bool debug = false;

bool has(std::string line, std::string substr)
{
    return line.find(substr) != std::string::npos;
}

/* return --notify_async or --notify_main_thread depending on if DEBUG_THREAD_SANITIZER is set or not */
const char *notify()
{
    if (getenv("DEBUG_THREAD_SANITIZER")){
        return "--notify_main_thread";
    } else {
        return "--notify_async";
    }
}

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

std::filesystem::path check;
bool check_set = false;

bool check_test_dir(std::filesystem::path test)
{
    std::cout << "check " << test << std::endl;
    if (std::filesystem::exists( test ) &&
        std::filesystem::exists( test / "CFReDS001.E01")) {
        std::cout << "Test directory: " << test << std::endl;
        check = test;
        check_set = true;
        return true;
    }
    return false;
}

/* We assume that the tests are being run out of bulk_extractor/src/.
 * This returns the directory of the test subdirectory.
 */
std::filesystem::path test_dir()
{
    if (check_set) return check;
    // We will set check to a variety of things and then test to see if the subdirectory jpegs exists.
    // If it does, we found the right location

    // if srcdir is set, use that, otherwise use the directory of the executable
    // srcdir is set when we run under autoconf 'make distcheck'
    const char *srcdir = getenv("srcdir");
    if (srcdir) {
        check = std::filesystem::path(srcdir) / "tests";
        if (check_test_dir(check)) return check;
    }
    check = my_executable().parent_path() / "tests";
    if (check_test_dir(check)) return check;

    check = my_executable().parent_path().parent_path().parent_path() / "tests";
    if (check_test_dir(check)) return check;

    check = my_executable().parent_path().parent_path().parent_path() / "src" / "tests";
    if (check_test_dir(check)) return check;

    std::cerr << "Cannot find tests directory.  my_executable:" << my_executable() << std::endl;
    exit(1);
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
        if ( has(it, feature)) return true;
    }
    std::cerr << "feature not found: " << feature << "\nfeatures found (perhaps one of these is the feature you are looking for?):\n";
    for (const auto &it : lines) {
        std::cerr << "  " << it << std::endl;
    }
    return false;
}

std::filesystem::path test_scanners(const std::vector<scanner_t *> & scanners, sbuf_t *sbuf)
{
    debug = getenv_debug("DEBUG");

    REQUIRE(sbuf->children == 0);
    feature_recorder_set::flags_t frs_flags;
    frs_flags.pedantic = true;          // for testing
    scanner_config sc;
    sc.outdir           = NamedTemporaryDirectory();
    sc.enable_all_scanners();

    scanner_set ss(sc, frs_flags, nullptr);
    for (auto const &it : scanners ){
        ss.add_scanner( it );
    }
    ss.apply_scanner_commands();
    REQUIRE (ss.get_enabled_scanners().size() == scanners.size());

    if (ss.get_enabled_scanners().size()>0){
        std::cerr << "## output in " << sc.outdir << " for " << ss.get_enabled_scanners()[0] << std::endl;
    } else {
        std::cerr << "## output in " << sc.outdir << " but no enabled scanner! " << std::endl;
    }
    ss.phase_scan();
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

TEST_CASE("scan_exif0", "[scanners]") {
    auto *sbufp = map_file("1.jpg");
    REQUIRE( sbufp->bufsize == 7323 );
    auto res = jpeg_validator::validate_jpeg(*sbufp);
    REQUIRE( res.how == jpeg_validator::COMPLETE );
    delete sbufp;
}

TEST_CASE("scan_exif1", "[scanners]") {
    auto *sbufp = map_file("exif_demo1.jpg");
    auto outdir = test_scanner(scan_exif, sbufp); // deletes sbufp
    auto exif_txt = getLines( outdir / "exif.txt" );
    auto last     = getLast(exif_txt);
    REQUIRE( has(last, "<ifd0.tiff.Make>Apple</ifd0.tiff.Make>"));
    REQUIRE( has(last, "<ifd0.tiff.Model>iPhone SE (2nd generation)</ifd0.tiff.Model>"));
    REQUIRE( has(last, "<ifd0.tiff.Orientation>1</ifd0.tiff.Orientation>" ));
    REQUIRE( has(last, "<ifd0.tiff.XResolution>72/1</ifd0.tiff.XResolution>" ));
    REQUIRE( has(last, "<ifd0.tiff.YResolution>72/1</ifd0.tiff.YResolution>" ));
    REQUIRE( has(last, "<ifd0.tiff.ResolutionUnit>2</ifd0.tiff.ResolutionUnit>" ));
    REQUIRE( has(last, "<ifd0.tiff.Software>15.2</ifd0.tiff.Software>" ));
    REQUIRE( has(last, "<ifd0.tiff.DateTime>2021:12:19 12:04:36</ifd0.tiff.DateTime>" ));
    REQUIRE( has(last, "<ifd0.tiff.YCbCrPositioning>1</ifd0.tiff.YCbCrPositioning>" ));
    REQUIRE( has(last, "<ifd0.exif.ExposureTime>1/1000</ifd0.exif.ExposureTime>" ));
    REQUIRE( has(last, "<ifd0.exif.FNumber>9/5</ifd0.exif.FNumber>" ));
    REQUIRE( has(last, "<ifd0.exif.ExposureProgram>2</ifd0.exif.ExposureProgram>" ));
    REQUIRE( has(last, "<ifd0.exif.PhotographicSensitivity>20</ifd0.exif.PhotographicSensitivity>" ));
    REQUIRE( has(last, "<ifd0.exif.ExifVersion>0232</ifd0.exif.ExifVersion>" ));
    REQUIRE( has(last, "<ifd0.exif.DateTimeOriginal>2021:12:19 12:04:36</ifd0.exif.DateTimeOriginal>" ));
    REQUIRE( has(last, "<ifd0.exif.DateTimeDigitized>2021:12:19 12:04:36</ifd0.exif.DateTimeDigitized>" ));
    REQUIRE( has(last, "<ifd0.exif.ComponentsConfiguration>\\001\\002\\003%00</ifd0.exif.ComponentsConfiguration>" ));
    REQUIRE( has(last, "<ifd0.exif.ShutterSpeedValue>70777/7102</ifd0.exif.ShutterSpeedValue>" ));
    REQUIRE( has(last, "<ifd0.exif.ApertureValue>54823/32325</ifd0.exif.ApertureValue>" ));
    REQUIRE( has(last, "<ifd0.exif.BrightnessValue>38857/4398</ifd0.exif.BrightnessValue>" ));
    REQUIRE( has(last, "<ifd0.exif.ExposureBiasValue>0/1</ifd0.exif.ExposureBiasValue>" ));
    REQUIRE( has(last, "<ifd0.exif.MeteringMode>5</ifd0.exif.MeteringMode>" ));
    REQUIRE( has(last, "<ifd0.exif.Flash>16</ifd0.exif.Flash>" ));
    REQUIRE( has(last, "<ifd0.exif.FocalLength>399/100</ifd0.exif.FocalLength>" ));
    REQUIRE( has(last, "<ifd0.exif.SubjectArea>ߝקࢩԲ</ifd0.exif.SubjectArea>" ));
    REQUIRE( has(last, "<ifd0.exif.SubSecTime>912</ifd0.exif.SubSecTime>" ));
    REQUIRE( has(last, "<ifd0.exif.SubSecTimeOriginal>912</ifd0.exif.SubSecTimeOriginal>" ));
    REQUIRE( has(last, "<ifd0.exif.SubSecTimeDigitized>912</ifd0.exif.SubSecTimeDigitized>" ));
    REQUIRE( has(last, "<ifd0.exif.FlashpixVersion>0100</ifd0.exif.FlashpixVersion>" ));
    REQUIRE( has(last, "<ifd0.exif.ColorSpace>65535</ifd0.exif.ColorSpace>" ));
    REQUIRE( has(last, "<ifd0.exif.PixelXDimension>4032</ifd0.exif.PixelXDimension>" ));
    REQUIRE( has(last, "<ifd0.exif.PixelYDimension>3024</ifd0.exif.PixelYDimension>" ));
    REQUIRE( has(last, "<ifd0.exif.SensingMethod>2</ifd0.exif.SensingMethod>" ));
    REQUIRE( has(last, "<ifd0.exif.SceneType>1</ifd0.exif.SceneType>" ));
    REQUIRE( has(last, "<ifd0.exif.ExposureMode>0</ifd0.exif.ExposureMode>" ));
    REQUIRE( has(last, "<ifd0.exif.WhiteBalance>0</ifd0.exif.WhiteBalance>" ));
    REQUIRE( has(last, "<ifd0.exif.FocalLengthIn35mmFilm>28</ifd0.exif.FocalLengthIn35mmFilm>" ));
    REQUIRE( has(last, "<ifd0.exif.SceneCaptureType>0</ifd0.exif.SceneCaptureType>" ));
    REQUIRE( has(last, "<ifd0.exif.LensSpecification>4183519/1048501 4183519/1048501 9/5 9/5</ifd0.exif.LensSpecification>" ));
    REQUIRE( has(last, "<ifd0.exif.LensMake>Apple</ifd0.exif.LensMake>" ));
    REQUIRE( has(last, "<ifd0.exif.LensModel>iPhone SE (2nd generation) back camera 3.99mm f/1.8</ifd0.exif.LensModel>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSLatitudeRef>N</ifd0.gps.GPSLatitudeRef>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSLatitude>41/1 28/1 58/100</ifd0.gps.GPSLatitude>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSLongitudeRef>W</ifd0.gps.GPSLongitudeRef>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSLongitude>70/1 37/1 2168/100</ifd0.gps.GPSLongitude>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSAltitudeRef>0</ifd0.gps.GPSAltitudeRef>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSAltitude>11909/2217</ifd0.gps.GPSAltitude>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSSpeedRef>K</ifd0.gps.GPSSpeedRef>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSSpeed>18195/40397</ifd0.gps.GPSSpeed>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSImgDirectionRef>T</ifd0.gps.GPSImgDirectionRef>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSImgDirection>211471/740</ifd0.gps.GPSImgDirection>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSDestBearingRef>T</ifd0.gps.GPSDestBearingRef>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSDestBearing>211471/740</ifd0.gps.GPSDestBearing>" ));
    REQUIRE( has(last, "<ifd0.gps.GPSHPositioningError>36467/969</ifd0.gps.GPSHPositioningError>" ));
}

// exif_demo2.tiff from https://github.com/ianare/exif-samples.git
TEST_CASE("scan_exif2", "[scanners]") {
    auto *sbufp = map_file("exif_demo2.tiff");
    auto outdir = test_scanner(scan_exif, sbufp); // deletes sbufp
    auto exif_txt = getLines( outdir / "exif.txt" );
    auto last     = getLast(exif_txt);
    REQUIRE( has(last, "<ifd0.tiff.ImageWidth>199</ifd0.tiff.ImageWidth>"));
    REQUIRE( has(last, "<ifd0.tiff.Compression>5</ifd0.tiff.Compression>"));
    REQUIRE( has(last, "<ifd0.tiff.PhotometricInterpreation>2</ifd0.tiff.PhotometricInterpreation>"));
    REQUIRE( has(last, "<ifd0.tiff.StripOffsets>8</ifd0.tiff.StripOffsets>"));
    REQUIRE( has(last, "<ifd0.tiff.Orientation>1</ifd0.tiff.Orientation>"));
    REQUIRE( has(last, "<ifd0.tiff.SamplesPerPixel>4</ifd0.tiff.SamplesPerPixel>"));
    REQUIRE( has(last, "<ifd0.tiff.RowsPerStrip>47</ifd0.tiff.RowsPerStrip>"));
    REQUIRE( has(last, "<ifd0.tiff.StripByteCounts>6205</ifd0.tiff.StripByteCounts>"));
    REQUIRE( has(last, "<ifd0.tiff.XResolution>1207959552/16777216</ifd0.tiff.XResolution>"));
    REQUIRE( has(last, "<ifd0.tiff.YResolution>1207959552/16777216</ifd0.tiff.YResolution>"));
    REQUIRE( has(last, "<ifd0.tiff.PlanarConfiguration>1</ifd0.tiff.PlanarConfiguration>"));
    REQUIRE( has(last, "<ifd0.tiff.ResolutionUnit>2</ifd0.tiff.ResolutionUnit>"));
    REQUIRE( has(last, "<ifd0.tiff.Software>Mac OS X 10.5.8 (9L31a)</ifd0.tiff.Software>"));
    REQUIRE( has(last, "<ifd0.tiff.DateTime>2012:01:09 22:52:11</ifd0.tiff.DateTime>"));
    REQUIRE( has(last, "<ifd0.tiff.Artist>Jean Cornillon</ifd0.tiff.Artist>"));
}


// exif_demo2.tiff from https://github.com/ianare/exif-samples.git
TEST_CASE("scan_exif3", "[scanners]") {
    auto *sbufp = map_file("exif_demo3.psd");
    auto outdir = test_scanner(scan_exif, sbufp); // deletes sbufp
    auto exif_txt = getLines( outdir / "exif.txt" );
    auto last     = getLast(exif_txt);

    REQUIRE( has(last, "<ifd0.tiff.Orientation>1</ifd0.tiff.Orientation>"));
    REQUIRE( has(last, "<ifd0.tiff.XResolution>3000000/10000</ifd0.tiff.XResolution>"));
    REQUIRE( has(last, "<ifd0.tiff.YResolution>3000000/10000</ifd0.tiff.YResolution>"));
    REQUIRE( has(last, "<ifd0.tiff.ResolutionUnit>2</ifd0.tiff.ResolutionUnit>"));
    REQUIRE( has(last, "<ifd0.tiff.Software>Adobe Photoshop 23.1 (Macintosh)</ifd0.tiff.Software>"));
    REQUIRE( has(last, "<ifd0.tiff.DateTime>2021:12:19 17:32:57</ifd0.tiff.DateTime>"));
    REQUIRE( has(last, "<ifd0.exif.ColorSpace>1</ifd0.exif.ColorSpace>"));
    REQUIRE( has(last, "<ifd0.exif.PixelXDimension>300</ifd0.exif.PixelXDimension>"));
    REQUIRE( has(last, "<ifd0.exif.PixelYDimension>300</ifd0.exif.PixelYDimension>"));
    REQUIRE( has(last, "<ifd1.tiff.Compression>6</ifd1.tiff.Compression>"));
    REQUIRE( has(last, "<ifd1.tiff.XResolution>72/1</ifd1.tiff.XResolution>"));
    REQUIRE( has(last, "<ifd1.tiff.YResolution>72/1</ifd1.tiff.YResolution>"));
    REQUIRE( has(last, "<ifd1.tiff.ResolutionUnit>2</ifd1.tiff.ResolutionUnit>"));
    REQUIRE( has(last, "<ifd1.tiff.JPEGInterchangeFormat>306</ifd1.tiff.JPEGInterchangeFormat>"));
    REQUIRE( has(last, "<ifd1.tiff.JPEGInterchangeFormatLength>0</ifd1.tiff.JPEGInterchangeFormatLength>"));
    REQUIRE( has(last, "<ifd0.tiff.Orientation>1</ifd0.tiff.Orientation>"));
    REQUIRE( has(last, "<ifd0.tiff.XResolution>3000000/10000</ifd0.tiff.XResolution>"));
    REQUIRE( has(last, "<ifd0.tiff.YResolution>3000000/10000</ifd0.tiff.YResolution>"));
    REQUIRE( has(last, "<ifd0.tiff.ResolutionUnit>2</ifd0.tiff.ResolutionUnit>"));
    REQUIRE( has(last, "<ifd0.tiff.Software>Adobe Photoshop 23.1 (Macintosh)</ifd0.tiff.Software>"));
    REQUIRE( has(last, "<ifd0.tiff.DateTime>2021:12:19 17:32:57</ifd0.tiff.DateTime>"));
    REQUIRE( has(last, "<ifd0.exif.ColorSpace>1</ifd0.exif.ColorSpace>"));
    REQUIRE( has(last, "<ifd0.exif.PixelXDimension>300</ifd0.exif.PixelXDimension>"));
    REQUIRE( has(last, "<ifd0.exif.PixelYDimension>300</ifd0.exif.PixelYDimension>"));
    REQUIRE( has(last, "<ifd1.tiff.Compression>6</ifd1.tiff.Compression>"));
    REQUIRE( has(last, "<ifd1.tiff.XResolution>72/1</ifd1.tiff.XResolution>"));
    REQUIRE( has(last, "<ifd1.tiff.YResolution>72/1</ifd1.tiff.YResolution>"));
    REQUIRE( has(last, "<ifd1.tiff.ResolutionUnit>2</ifd1.tiff.ResolutionUnit>"));
    REQUIRE( has(last, "<ifd1.tiff.JPEGInterchangeFormat>306</ifd1.tiff.JPEGInterchangeFormat>"));
    REQUIRE( has(last, "<ifd1.tiff.JPEGInterchangeFormatLength>0</ifd1.tiff.JPEGInterchangeFormatLength>"));
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
    auto outdir = test_scanner(scan_json, sbufp); // deletes sbufp

    /* Read the output */
    auto json_txt = getLines( outdir / "json.txt" );
    auto last = getLast(json_txt);

    REQUIRE(last.size() > 40);
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
const uint8_t packet1[] = {
    0x00, 0x50, 0xe8, 0x04, // ethernet header
    0x77, 0x4b, 0x2c, 0xf0,
    0xa2, 0xf3, 0xa8, 0xee,
    0x08, 0x00,
    0x45, 0x00, 0x00, 0x40, // ipv4 header
    0x00, 0x00, 0x40, 0x00, 0x40, 0x06,
    0x3b, 0x8d, 0xac, 0x14,
    0x00, 0xb9, 0xac, 0xd9,
    0xa5, 0x84, 0xea, 0x06, 0x00, 0x50, 0xab, 0x8c,
    0x75, 0x56, 0x00, 0x00, 0x00, 0x00, 0xb0, 0xc2, 0xff, 0xff, 0x8e, 0xfd, 0x00, 0x00, 0x02, 0x04,
    0x05, 0xb4, 0x01, 0x03, 0x03, 0x06, 0x01, 0x01, 0x08, 0x0a, 0x72, 0x22, 0x2a, 0xb7, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x02, 0x00, 0x00
};

const uint16_t addr1[] = { 0x2603, 0x3003, 0x0127, 0x1000, 0x9440, 0x31dd, 0xdd50, 0xe403 };
const uint16_t addr2[] = { 0xffff, 0xffff, 0xffff, 0xffff, 0x6eee, 0xe608, 0x1111, 0x1187 };


/****************************************************************/
/*

    Internet Protocol Version 6
        0110 .... = Version: 6
        .... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
            .... 0000 00.. .... .... .... .... .... = Differentiated Services Codepoint: Default (0)
            .... .... ..00 .... .... .... .... .... = Explicit Congestion Notification: Not ECN-Capable Transport (0)
        .... .... .... 0000 0000 0000 0000 0000 = Flow Label: 0x00000
        Payload Length: 36
        Next Header: UDP (17)
        Hop Limit: 1
        Source Address: fe80::c46b:e4:d55:62b1
        Destination Address: ff02::1:3
    User Datagram Protocol
        Source Port: 63902
        Destination Port: 5355
        Length: 36
        Checksum: 0x21dc
        Checksum Status: Unverified
        Stream index: 0
        Timestamps
            Time since first frame: 0.000000000 seconds
            Time since previous frame: 0.000000000 seconds
        UDP payload (28 bytes)
    Link-local Multicast Name Resolution (query)
        Transaction ID: 0x7e2a
        Flags: 0x0000 Standard query
            0... .... .... .... = Response: Message is a query
            .000 0... .... .... = Opcode: Standard query (0)
            .... .0.. .... .... = Conflict: None
            .... ..0. .... .... = Truncated: Message is not truncated
            .... ...0 .... .... = Tentative: Not tentative
        Questions: 1
        Answer RRs: 0
        Authority RRs: 0
        Additional RRs: 0
        Queries
            xzignbnfvi: type A, class IN
                Name: xzignbnfvi
                Name Length: 10
                Label Count: 1
                Type: A (Host Address) (1)
                Class: IN (0x0001)
*/


/* DNS ipv6 packet, sans ethernet header */
const uint8_t packet2[] = {
    0x60, 0x00, 0x00, 0x00,        // version, traffic | flow label
    0x00, 0x24, 0x11, 0x01,        // payload length   | next header | hop limit
    0xfe, 0x80, 0x00, 0x00,        // source address
    0x00, 0x00, 0x00, 0x00,
    0xc4, 0x6b, 0x00, 0xe4,
    0x0d, 0x55, 0x62, 0xb1,
    0xff, 0x02, 0x00, 0x00,  // destination address
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x03,
    0xf9, 0x9e, 0x14, 0xeb, // udp source port | destination port
    0x00, 0x24, 0x21, 0xdc, // udp length | checksum
    0x7e, 0x2a, 0x00, 0x00, // udp data
    0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x0a, 0x78, 0x7a, 0x69,
    0x67, 0x6e, 0x62, 0x6e,
    0x66, 0x76, 0x69, 0x00,
    0x00, 0x01, 0x00, 0x01,
    0xa6, 0xca, 0xb2, 0x70,
    0xf8, 0x67, 0x2f, 0x3f
};


/****************************************************************/
/* First packet of a wget from google over IPv6 */
/*
(base) simsong@nimi ~ % tcpdump -r out1.pcap -x
reading from file out1.pcap, link-type EN10MB (Ethernet)
14:33:29.327826 IP6 2603:3003:127:1000:9440:31dd:dd50:e403.49478 > iad30s43-in-x04.1e100.net.http: Flags [S], seq 3310600832, win 65535, options [mss 1440,nop,wscale 6,nop,nop,TS val 1142909182 ecr 0,sackOK,eol], length 0
	0x0000:  6005 0500 002c 0640 2603 3003 0127 1000
	0x0010:  9440 31dd dd50 e403 2607 f8b0 4004 082f
	0x0020:  0000 0000 0000 2004 c146 0050 c553 c280
	0x0030:  0000 0000 b002 ffff 75c1 0000 0204 05a0
	0x0040:  0103 0306 0101 080a 441f 68fe 0000 0000
	0x0050:  0402 0000

Internet Protocol Version 6
0110 .... = Version: 6
.... 0000 0000 .... .... .... .... .... = Traffic Class: 0x00 (DSCP: CS0, ECN: Not-ECT)
.... .... .... 0101 0000 0101 0000 0000 = Flow Label: 0x50500
Payload Length: 44
Next Header: TCP (6)
Hop Limit: 64
Source Address: 2603:3003:127:1000:9440:31dd:dd50:e403
Destination Address: 2607:f8b0:4004:82f::2004
Transmission Control Protocol
Source Port: 49478
Destination Port: 80
Stream index: 0
TCP Segment Len: 0
Sequence Number: 0
Sequence Number (raw): 3310600832
Next Sequence Number: 1
Acknowledgment Number: 0
Acknowledgment number (raw): 0
1011 .... = Header Length: 44 bytes (11)
Flags: 0x002 (SYN)
Window: 65535
Calculated window size: 65535
Checksum: 0x75c1
Checksum Status: Unverified
Urgent Pointer: 0
Options: (24 bytes), Maximum segment size, No-Operation (NOP), Window scale, No-Operation (NOP), No-Operation (NOP), Timestamps, SACK permitted, End of Option List (EOL)
Timestamps
*/

TEST_CASE("scan_net1", "[scanners]") {
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

    scan_net_t::generic_iphdr_t h {} ;

    REQUIRE( scan_net_t::sanityCheckIP46Header( sbufip, 0 , &h, nullptr) == true );
    REQUIRE( h.checksum_valid == true );

    /* Now try with the offset */
    REQUIRE( scan_net_t::sanityCheckIP46Header( sbuf, frame_offset + ETHERNET_FRAME_SIZE, &h, nullptr) == true );
    REQUIRE( h.is_4() == true);
    REQUIRE( h.is_6() == false);
    REQUIRE( h.is_4or6() == true);
    REQUIRE( h.checksum_valid == true );

    /* Change the IP address and make sure that the header is valid but the checksum is not */
    buf[frame_offset + ETHERNET_FRAME_SIZE + 14]++; // increment destination address
    REQUIRE( scan_net_t::sanityCheckIP46Header( sbufip, 0 , &h, nullptr) == true );
    REQUIRE( h.checksum_valid == false );

    /* Break the port and make sure that the header is no longer valid */
    buf[frame_offset + ETHERNET_FRAME_SIZE] += 0x10; // increment header length
    REQUIRE( scan_net_t::sanityCheckIP46Header( sbufip, 0 , &h, nullptr) == false );

    /* Try some IP addresses to make sure they check or do not check */
    REQUIRE( sizeof(addr1) == 16 );
    REQUIRE( sizeof(addr2) == 16 );

    /* Put the addresses into network order and check */
    uint16_t addr[8];
    for (int i=0;i<8;i++){
      addr[i] = htons(addr1[i]);
    }
    REQUIRE( scan_net_t::invalidIP6(addr) == false );

    for (int i=0;i<8;i++){
      addr[i] = htons(addr2[i]);
    }
    REQUIRE( scan_net_t::invalidIP6(addr) == true );
}

#ifdef TEST_IPV6

/* Validate checksum computation from
 * https://stackoverflow.com/questions/30858973/udp-checksum-calculation-for-ipv6-packet
 */
const uint8_t packet6_udp[] = {
    0x60, 0x00, 0x00, 0x00, // version, traffic | flow label
    0x00, 0x34, 0x11, 0x01, // payload length   | next header | hop limit
    0x21, 0x00, 0x00, 0x00, // source
    0x00, 0x00, 0x00, 0x01,
    0xAB, 0xCD, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
    0xFD, 0x00, 0x00, 0x00, // destination address
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x60,
    0x26, 0x92, 0x26, 0x92, // udp source | udp destination
    0x00, 0x0C, 0x7E, 0xD5, // udp length | udp checksum
    0x12, 0x34, 0x56, 0x78  // udp data
};
TEST_CASE("ipv6_checksum_UDP", "[scanners]") {
    /* Try packet2 */
    sbuf_t sbuf2(pos0_t(), packet2, sizeof(packet2));
    std::cerr << "sbuf2: " << sbuf2 << std::endl;
    REQUIRE( scan_net_t::ip6_cksum_valid( sbuf2, 0) == true );


    /* Try packet6 */
    sbuf_t sbuf6(pos0_t(), packet6_udp, sizeof(packet6_udp));
    REQUIRE( sbuf6.bufsize==52 );
    REQUIRE( scan_net_t::ip6_cksum_valid( sbuf6, 0) == true );

    /* Make a 1 byte change to packet6 and make sure it fails */
    uint8_t buf[1024];
    memcpy(buf, packet6_udp, sizeof(packet6_udp));
    buf[16]++;
    sbuf_t sbuf6_bad(pos0_t(), buf, sizeof(packet6_udp));
    REQUIRE( sbuf6_bad.bufsize==52 );
    REQUIRE( scan_net_t::ip6_cksum_valid( sbuf6_bad, 0) == false );

};

TEST_CASE("ipv6_checksum_TCP", "[scanners]") {
    sbuf_t sbuf(pos0_t(), packet6_udp, sizeof(packet6_udp));
    REQUIRE( sbuf.bufsize==52 );
    REQUIRE( scan_net_t::ip6_cksum_valid( sbuf, 0) == true );

    /* Make a 1 byte change to the payload */
    uint8_t buf[1024];
    memcpy(buf, packet6_udp, sizeof(packet6_udp));
    buf[16]++;
    sbuf_t sbuf2(pos0_t(), buf, sizeof(packet6_udp));
    REQUIRE( sbuf2.bufsize==52 );
    REQUIRE( scan_net_t::ip6_cksum_valid( sbuf2, 0) == false );

};

TEST_CASE("scan_net2", "[scanners]") {
    /* This checks specifically for a valid IPv6 packet that we know is valid. */
    constexpr size_t frame_offset = 15;           // where we put the packet. Make sure that it is not byte-aligned!
    uint8_t buf[1024];
    scan_net_t::generic_iphdr_t h {} ;

    memset(buf,0xee,sizeof(buf));       // arbitrary data
    memcpy(buf + frame_offset, packet2, sizeof(packet1)); // copy it to an offset that is not byte-aligned
    sbuf_t sbuf(pos0_t(), buf, sizeof(buf));

    constexpr size_t packet2_ip_len = sizeof(packet2); // 14 bytes for ethernet header

    REQUIRE( packet2_ip_len == 84); // from above

    /*
    const struct ip6_hdr *ip6 = sbuf.get_struct_ptr_unsafe<struct ip6_hdr>( buf+frame_offset );
    REQUIRE( ip6->is_ipv6() == true);
    REQUIRE( buf[40+frame_offset] == 0x00);
    REQUIRE( buf[41+frame_offset] == 0x11);
    */

    REQUIRE( scan_net_t::sanityCheckIP46Header( sbuf, frame_offset , &h, nullptr) == true );
    REQUIRE( h.is_4() == false);
    REQUIRE( h.is_6() == true);
    REQUIRE( h.is_4or6() == true);
    REQUIRE( h.checksum_valid == true);
}
#endif

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
    auto *sbufp = map_file( "john_jakes.vcf" );
    auto outdir = test_scanner(scan_wordlist, sbufp); // deletes sbufp

    /* Read the output */
    auto wordlist_txt = getLines( outdir / "wordlist_dedup_1.txt");
    REQUIRE( wordlist_txt[0] == "States" );
    REQUIRE( wordlist_txt[1] == "America" );
    REQUIRE( wordlist_txt[2] == "Company" );
}

TEST_CASE("scan_winprefetch", "[scanners]") {
    auto *sbufp = map_file( "test_winprefetch.raw" );
    auto outdir = test_scanner(scan_winprefetch, sbufp); // deletes sbufp

    auto prefetch_txt = getLines( outdir / "winprefetch.txt");
    REQUIRE( requireFeature(prefetch_txt, "3584\tRUNDLL32.EXE" ));
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
