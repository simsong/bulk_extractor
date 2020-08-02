#ifndef PHASE1_H
#define PHASE1_H

#include "be13_api/utils.h"             // split()
#include "be13_api/aftimer.h"
#include "image_process.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"
#include "threadpool.hpp"               // new threadpool!

/**
 * bulk_extractor:
 * phase 0 - Load every scanner, create the feature recorder set, create the feature recorders
 * phase 1 - process every buffer with every scanner.
 *           BE1.0 - Each sbuf is loaded by the producer. Each worker runs all scanners.
 *           BE2.0 - Each sbuf is loaded by the producer. Each worker runs a single scanner.
 * phase 2 - histograms are made.
 *
 */


/****************************************************************
 *** Phase 1 BUFFER PROCESSING
 *** For every page of the iterator, schedule work.
 ****************************************************************/

class BulkExtractor_Phase1 {
public:
    /* Configuration Control */
    class Config {
        static const auto MB = 1024*1024;
        Config &operator=(const Config &);  // not implemented
        Config(const Config &);             // not implemented
    public:
        Config() {}
        uint64_t debug;                 // debug
        size_t   opt_pagesize {16 * MB};
        size_t   opt_marginsize { 4 * MB};
        uint32_t max_bad_alloc_errors {0};
        bool     opt_info {false};
        uint32_t opt_notify_rate {4};		// by default, notify every 4 pages
        uint64_t opt_page_start {0};
        int64_t  opt_offset_start {0};
        int64_t  opt_offset_end {0};
        time_t   max_wait_time {3600};  // after an hour, terminate a scanner
        int      opt_quiet {false};                  // -1 = no output
        int      retry_seconds {60};
        u_int    num_threads {1};
        double   sampling_fraction {1.0};       // for random sampling
        u_int    sampling_passes {1};
        bool     opt_report_read_errors {true};
    };

    typedef std::set<uint64_t> blocklist_t; // a list of blocks
    static void msleep(uint32_t msec); // sleep for a specified number of msec
    static std::string minsec(time_t tsec);    // return "5 min 10 sec" string
    static void make_sorted_random_blocklist(blocklist_t *blocklist,uint64_t max_blocks,float frac);
    static void set_sampling_parameters(Config &c,std::string &p);

private:
    /** Sleep for a minimum of msec */
    bool sampling(){                    // are we random sampling?
        return config.sampling_fraction<1.0;
    }

    class threadpool *tp;
    void print_tp_status();

public:
    typedef std::set<std::string> seen_page_ids_t;
    /**
     * print the status of a threadpool
     */
    /* Instance variables */
    dfxml_writer  &xreport;
    aftimer       &timer;
    Config        &config;
    u_int         notify_ctr  {0};    /* for random sampling */
    uint64_t      total_bytes {0};               //
    image_process &p;
    feature_recorder_set &fs;
    seen_page_ids_t &seen_page_ids;
    dfxml::md5_generator *md5g {nullptr};        // the MD5 of the image. Set to 0 if a gap is encountered

    BulkExtractor_Phase1(dfxml_writer &xreport_,aftimer &timer_,Config &config_,
                         image_process &p_, feature_recorder_set &fs_, seen_page_ids_t &seen_page_ids_):
        xreport(xreport_),timer(timer_),config(config_),
        p(p_), fs(fs_), seen_page_ids(seen_page_ids_)  {}

    /* Get the sbuf from current image iterator location, with retries */
    sbuf_t *get_sbuf(image_process::iterator &it);

    /* Notify user about current state of phase1 */
    void notify_user(image_process::iterator &it);

    void load_workers();
    void wait_for_workers();
    void run();
};

#endif
