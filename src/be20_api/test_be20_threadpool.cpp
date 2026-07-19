/*
 * be20_api threadpool test is in this file.
 * The goal is to have complete test coverage of the v2 API
 *
 */

// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"
#include "catch.hpp"

#include <filesystem>

#include "scanner_set.h"
#include "scan_sha1_test.h"

std::filesystem::path get_tempdir();

TEST_CASE("scanner_set_mt", "[thread_pool]") {
    scanner_config sc;
    sc.outdir = get_tempdir() / "threadpool";
    std::filesystem::create_directory(sc.outdir);
    sc.push_scanner_command("sha1_test", scanner_config::scanner_command::ENABLE);

    feature_recorder_set::flags_t f;
    scanner_set ss(sc, f, nullptr);
    ss.add_scanner(scan_sha1_test);
    ss.apply_scanner_commands();
    feature_recorder& recorder = ss.named_feature_recorder("sha1_bufs");

    ss.phase_scan();
    ss.launch_workers(4);
    for (const char *text : {"alpha", "bravo", "charlie", "delta", "echo", "foxtrot"}) {
        ss.schedule_sbuf(new sbuf_t(text));
    }
    ss.join();
    REQUIRE(ss.get_worker_count() == 0);
    REQUIRE(ss.get_tasks_queued() == 0);
    REQUIRE(recorder.features_written == 6);
    ss.shutdown();
}

TEST_CASE("idle_workers_shutdown", "[thread_pool]") {
    scanner_config sc;
    sc.outdir = get_tempdir() / "idle_workers_shutdown";
    std::filesystem::create_directory(sc.outdir);

    feature_recorder_set::flags_t f;
    scanner_set ss(sc, f, nullptr);
    ss.apply_scanner_commands();
    ss.phase_scan();
    ss.launch_workers(32);
    ss.join();

    REQUIRE(ss.get_worker_count() == 0);
    REQUIRE(ss.get_tasks_queued() == 0);
    ss.shutdown();
}
