#define CATCH_CONFIG_MAIN

#include "config.h"
#include "be20_api/catch.hpp"
#include "be20_api/dfxml_cpp/src/cpuid.h"
#include "be20_api/dfxml_cpp/src/dfxml_writer.h"
#include "be20_api/dfxml_cpp/src/hash_t.h"

#include <chrono>
#include <filesystem>

TEST_CASE("dfxml hashes", "[dfxml]") {
    const uint8_t empty[1] = {0};
    REQUIRE(dfxml::md5_generator::hash_buf(empty, 0).hexdigest() ==
            "d41d8cd98f00b204e9800998ecf8427e");
    REQUIRE(dfxml::sha1_generator::hash_buf(empty, 0).hexdigest() ==
            "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    REQUIRE(dfxml::sha256_generator::hash_buf(empty, 0).hexdigest() ==
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_CASE("dfxml writer", "[dfxml]") {
    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output = std::filesystem::temp_directory_path() /
                        ("bulk_extractor-dfxml-" + std::to_string(suffix) + ".xml");
    char program[] = "test_dfxml";
    char *argv[] = {program};

    dfxml_writer writer(output, false);
    writer.add_DFXML_creator("test_dfxml", VERSION, GIT_COMMIT, 1, argv);
    writer.comment("writer integration test");
    writer.close();

    REQUIRE(std::filesystem::file_size(output) > 0);
    std::filesystem::remove(output);
}

TEST_CASE("cpuid availability", "[dfxml]") {
#if defined(_WIN32) || defined(HAVE_ASM_CPUID)
    REQUIRE(CPUID::vendor().size() == 12);
#else
    REQUIRE(CPUID::vendor().empty());
#endif
}
