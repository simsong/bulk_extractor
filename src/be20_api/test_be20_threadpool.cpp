/*
 * be20_api threadpool test is in this file.
 * The goal is to have complete test coverage of the v2 API
 *
 */

// https://github.com/catchorg/Catch2/blob/master/docs/tutorial.md#top

#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"
#include "catch.hpp"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>

#include "scanner_set.h"
#include "scan_sha1_test.h"

std::filesystem::path get_tempdir();

namespace {
std::mutex blocking_scanner_mutex;
std::condition_variable blocking_scanner_cv;
bool blocking_scanner_started {false};
bool release_blocking_scanner {false};

void scan_blocking_test(scanner_params& sp) {
    if (sp.phase == scanner_params::PHASE_INIT) {
        sp.info->set_name("blocking_test");
        sp.info->min_sbuf_size = 1;
        return;
    }
    if (sp.phase == scanner_params::PHASE_SCAN) {
        std::unique_lock<std::mutex> lock(blocking_scanner_mutex);
        blocking_scanner_started = true;
        blocking_scanner_cv.notify_all();
        blocking_scanner_cv.wait(lock, [] { return release_blocking_scanner; });
    }
}
}

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

TEST_CASE("timed_join_reports_timeout", "[thread_pool]") {
    scanner_config sc;
    sc.outdir = get_tempdir() / "timed_join_reports_timeout";
    std::filesystem::create_directory(sc.outdir);

    feature_recorder_set::flags_t f;
    scanner_set ss(sc, f, nullptr);
    ss.add_scanner(scan_blocking_test);
    ss.apply_scanner_commands();
    ss.phase_scan();
    ss.launch_workers(1);

    {
        std::lock_guard<std::mutex> lock(blocking_scanner_mutex);
        blocking_scanner_started = false;
        release_blocking_scanner = false;
    }
    ss.schedule_sbuf(new sbuf_t("blocked"));

    bool scanner_started;
    {
        std::unique_lock<std::mutex> lock(blocking_scanner_mutex);
        scanner_started = blocking_scanner_cv.wait_for(
            lock, std::chrono::seconds(5), [] { return blocking_scanner_started; });
    }
    const bool joined_before_deadline = ss.join(std::chrono::seconds(0));

    {
        std::lock_guard<std::mutex> lock(blocking_scanner_mutex);
        release_blocking_scanner = true;
    }
    blocking_scanner_cv.notify_all();
    const bool joined_after_release = ss.join(std::chrono::seconds(5));

    REQUIRE(scanner_started);
    REQUIRE_FALSE(joined_before_deadline);
    REQUIRE(joined_after_release);
    REQUIRE(ss.get_worker_count() == 0);
    REQUIRE(ss.get_tasks_queued() == 0);
    ss.shutdown();
}
