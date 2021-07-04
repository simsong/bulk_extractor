#ifndef PHASE1_H
#define PHASE1_H

#include <thread>

#include "be13_api/aftimer.h"
#include "be13_api/scanner_set.h"
#include "be13_api/dfxml_cpp/src/dfxml_writer.h"
#include "be13_api/dfxml_cpp/src/hash_t.h"

#include "image_process.h"
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

class Phase1 {
    Phase1(const Phase1 &that) = delete; // no copy constructor
    Phase1 &operator=(const Phase1 &that) = delete; // no assignment
public:
    /* Configuration Control */
    struct Config {
        static const auto MiB = 1024*1024;
        Config(const Config &that) = default; // copy constructor - default
        Config &operator=(const Config &that) = delete;        // assignment constructor - delete
    public:

        Config() { }
        uint64_t debug {false};                 // debug
        size_t   opt_pagesize {16 * MiB};
        size_t   opt_marginsize { 4 * MiB};
        uint32_t max_bad_alloc_errors {3}; // by default, 3 retries
        bool     opt_info {false};
        uint32_t opt_notify_rate {4};		// by default, notify every 4 pages
        uint64_t opt_page_start {0};
        uint64_t  opt_offset_start {0};
        uint64_t  opt_offset_end {0};
        time_t   max_wait_time {3600};  // after an hour, terminate a scanner
        int      opt_quiet {false};                  // -1 = no output
        int      retry_seconds {60};
        u_int    num_threads  { std::thread::hardware_concurrency() }; // default to # of cores
        double   sampling_fraction {1.0};       // for random sampling
        u_int    sampling_passes {1};
        bool     opt_report_read_errors {true};
        void     set_sampling_parameters(std::string p);
    };

    typedef std::set<uint64_t> blocklist_t; // a list of blocks (for random sampling)
    static void msleep(uint32_t msec); // sleep for a specified number of msec
    static std::string minsec(time_t tsec);    // return "5 min 10 sec" string
    static void make_sorted_random_blocklist(blocklist_t *blocklist,uint64_t max_blocks,float frac);

private:
    bool sampling(){                    // are we random sampling?
        return config.sampling_fraction<1.0;
    }

    class threadpool *tp {nullptr};
    void print_tp_status();

public:
    typedef std::set<std::string> seen_page_ids_t;
    /**
     * print the status of a threadpool
     */
    /* Instance variables */
    dfxml_writer  &xreport;
    aftimer       timer {};
    const Config  config;
    u_int         notify_ctr  {0};    /* for random sampling */
    uint64_t      total_bytes {0};               //
    image_process &p;
    scanner_set   &ss;
    seen_page_ids_t seen_page_ids {};
    dfxml::sha1_generator *sha1g {nullptr};        // the SHA1 of the image. Set to 0 if a gap is encountered
    uint64_t      sha1_next {0};                   // next byte to hash

    std::string image_hash {};

    /* Get the sbuf from current image iterator location, with retries */
    sbuf_t *get_sbuf(image_process::iterator &it);

    /* Notify user about current state of phase1 */
    void notify_user(image_process::iterator &it);

    Phase1(dfxml_writer &xreport_,Config config_, image_process &p_, scanner_set &ss_);
    void dfxml_write_create(int argc, char * const *argv);
    void send_data_to_workers();        // launch all of the workers
    void dfxml_write_source();            //
    void run();                         // does the above
};

#endif
