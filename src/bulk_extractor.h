/*
 *
 */

#ifndef _BULK_EXTRACTOR_H_
#define _BULK_EXTRACTOR_H_

#include "be13_api/scanner_set.h"
#include "be13_api/aftimer.h"

struct notify_thread {
    struct notify_opts {
        scanner_set *ssp;
        aftimer *master_timer;
        std::atomic<double> *fraction_done;
        bool opt_legacy;
    };

    static inline const std::string FRACTION_READ {"fraction_read"};
    static inline const std::string ESTIMATED_TIME_REMAINING {"estimated_time_remaining"};
    static inline const std::string ESTIMATED_DATE_COMPLETION {"estimated_date_completion"};


    [[noreturn]] static void notifier( struct notify_opts *o );
    static void launch_notify_thread( struct notify_opts *o);
};

[[noreturn]] void debug_help();
//void usage(const char *progname, scanner_set &ss);
void validate_path(const std::filesystem::path fn);
int bulk_extractor_main(int argc,char * const *argv);

#endif
