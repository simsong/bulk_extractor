#ifndef NOTIFY_THREAD_H
#define NOTIFY_THREAD_H

#include "config.h"

#include <string>
#include "be13_api/aftimer.h"
#include "be13_api/scanner_set.h"
#include "phase1.h"

struct notify_thread {
    struct notify_opts {
        notify_opts(const Phase1::Config &cfg_) : cfg(cfg_) {};
        scanner_set *ssp {};
        aftimer *master_timer {};
        std::atomic<double> *fraction_done {};
        const Phase1::Config &cfg;
    };

    static inline const std::string FRACTION_READ {"fraction_read"};
    static inline const std::string ESTIMATED_TIME_REMAINING {"estimated_time_remaining"};
    static inline const std::string ESTIMATED_DATE_COMPLETION {"estimated_date_completion"};

    static int terminal_width( int default_width );

    [[noreturn]] static void notifier( struct notify_opts *o );
    static void launch_notify_thread( struct notify_opts *o);
};

#endif
