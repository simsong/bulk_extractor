#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CONSOLE_WIDTH 120

#include "config.h"
#include "catch.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>

#include "scanner_set.h"
#include "utils.h"

namespace {
std::mutex scanner_mutex;
std::condition_variable scanner_cv;
bool scanner_started {false};
bool release_scanner {false};

void blocking_scanner(scanner_params &sp)
{
    if (sp.phase == scanner_params::PHASE_INIT) {
        sp.info->set_name("thread_safety_blocking");
        sp.info->min_sbuf_size = 1;
        return;
    }
    if (sp.phase == scanner_params::PHASE_SCAN) {
        std::unique_lock<std::mutex> lock(scanner_mutex);
        scanner_started = true;
        scanner_cv.notify_all();
        scanner_cv.wait(lock, [] { return release_scanner; });
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

    {
        std::lock_guard<std::mutex> lock(scanner_mutex);
        scanner_started = false;
        release_scanner = false;
    }
    ss.schedule_sbuf(new sbuf_t("first"));
    {
        std::unique_lock<std::mutex> lock(scanner_mutex);
        REQUIRE(scanner_cv.wait_for(lock, std::chrono::seconds(5), [] { return scanner_started; }));
    }

    std::atomic<bool> read_snapshots {true};
    std::atomic<bool> observed_wait {false};
    std::thread reader([&] {
        while (read_snapshots.load()) {
            observed_wait.store(observed_wait.load() || ss.producer_wait_ns() > 0);
        }
    });
    std::thread producer([&] { ss.main_thread_wait(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    {
        std::lock_guard<std::mutex> lock(scanner_mutex);
        release_scanner = true;
    }
    scanner_cv.notify_all();
    producer.join();
    read_snapshots = false;
    reader.join();

    ss.join();
    REQUIRE(observed_wait);
    REQUIRE(ss.producer_wait_ns() > 0);
    REQUIRE_FALSE(ss.producer_wait_text().empty());
    ss.shutdown();
}
