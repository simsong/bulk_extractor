#ifndef NOTIFY_THREAD_H
#define NOTIFY_THREAD_H

#include "config.h"

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "be20_api/aftimer.h"
#include "be20_api/scanner_set.h"

#include "phase1.h"

class notify_thread {
    notify_thread() = delete;
    notify_thread(const notify_thread &that) = delete;
    notify_thread &operator=(const notify_thread &that) = delete;
    std::thread *the_notify_thread {nullptr};
    void *run();
public:
    notify_thread(std::ostream &os_, scanner_set &ss_, const Phase1::Config &cfg_, aftimer &master_timer_, std::atomic<double> *fraction_done_):
        os(os_), ss(ss_), cfg(cfg_), master_timer(master_timer_),fraction_done(fraction_done_) {};
    ~notify_thread();
    static int terminal_width( int default_width );
    std::ostream &os;
    scanner_set &ss;
    const Phase1::Config &cfg;
    aftimer &master_timer;
    std::atomic<double> *fraction_done {};
    std::atomic<int> phase {};
    std::mutex Mphase {};           // mutex for phase
    static inline const std::string FRACTION_READ {"fraction_read"};
    static inline const std::string ESTIMATED_TIME_REMAINING {"estimated_time_remaining"};
    static inline const std::string ESTIMATED_DATE_COMPLETION {"estimated_date_completion"};

    void start_notify_thread( );
    void join();
};

#endif
