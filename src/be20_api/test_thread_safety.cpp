#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"
#include "catch.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <thread>

#include "scanner_set.h"
#include "utils.h"

namespace {
std::promise<void> scanner_started;
std::shared_future<void> release_scanner;

void blocking_scanner(scanner_params &sp)
{
    if (sp.phase == scanner_params::PHASE_INIT) {
        sp.info->set_name("thread_safety_blocking");
        sp.info->min_sbuf_size = 1;
        return;
    }
    if (sp.phase == scanner_params::PHASE_SCAN) {
        scanner_started.set_value();
        release_scanner.wait();
    }
}
}

TEST_CASE("producer wait snapshots are safe while workers run", "[thread_safety]")
{
    scanner_config sc;
    sc.outdir = NamedTemporaryDirectory();

    feature_recorder_set::flags_t flags;
    scanner_set ss(sc, flags, nullptr);
    ss.add_scanner(blocking_scanner);
    ss.apply_scanner_commands();
    ss.phase_scan();
    ss.launch_workers(1);

    auto scanner_started_future = scanner_started.get_future();
    std::promise<void> release;
    release_scanner = release.get_future().share();
    ss.schedule_sbuf(new sbuf_t("first"));
    REQUIRE(scanner_started_future.wait_for(std::chrono::seconds(5)) == std::future_status::ready);

    std::atomic<bool> read_snapshots {true};
    std::atomic<bool> observed_wait {false};
    std::thread reader([&] {
        while (read_snapshots.load()) {
            observed_wait.store(observed_wait.load() || ss.producer_wait_ns() > 0);
        }
    });
    std::thread producer([&] { ss.main_thread_wait(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    release.set_value();
    producer.join();
    read_snapshots = false;
    reader.join();

    ss.join();
    REQUIRE(observed_wait);
    REQUIRE(ss.producer_wait_ns() > 0);
    REQUIRE_FALSE(ss.producer_wait_text().empty());
    ss.shutdown();
}
