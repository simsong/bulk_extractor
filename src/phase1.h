#ifndef PHASE1_H
#define PHASE1_H

#include <thread>
#include <atomic>

#include "be13_api/scanner_set.h"
#include "be13_api/dfxml_cpp/src/dfxml_writer.h"
#include "be13_api/dfxml_cpp/src/hash_t.h"

#include "image_process.h"

/**
 * bulk_extractor:
 * phase 0 - Load every scanner, create the feature recorder set, create the feature recorders
 * phase 1 - process every buffer with every scanner.
 *           BE1.0 - Each sbuf is loaded by the producer. Each worker runs all scanners.
 *           BE2.0 - Each sbuf is loaded by the producer. Each worker runs a single scanner.
 * phase 2 - histograms are made.
 *
 * This file implements Phase 1 - reads some or all of the sbufs and asks the scanner set to process them.
 * A regular scanner set will process them sequentially in a single-thread. The multithreaded_scanner_set will
 * create a work queue, queue each sbuf to be processed, and process them in parallel.
 */


/****************************************************************
 *** Phase 1 BUFFER PROCESSING
 *** For every page of the iterator, schedule work.
 ****************************************************************/

class Phase1 {
    Phase1(const Phase1 &that) = delete; // no copy constructor
    Phase1 &operator=(const Phase1 &that) = delete; // no assignment
    bool sampling(){                    // are we random sampling?
        return config.sampling_fraction<1.0;
    }

<<<<<<< HEAD
public:
    // because seen_page_ids are added in order, we want to use an unordered set.
    typedef std::unordered_set<std::string> seen_page_ids_t;
    static inline std::string REPORT_FILENAME {"report.xml"};
    /* Configuration Control */
    struct Config {
        static const auto MiB = 1024*1024;
        Config(const Config &that) = default; // copy constructor - default
        Config &operator=(const Config &that) = delete;        // assignment constructor - delete

        Config() { }
        uint64_t  debug {false};                 // debug
        size_t    opt_pagesize {16 * MiB};
        size_t    opt_marginsize { 4 * MiB};
        uint32_t  max_bad_alloc_errors {3}; // by default, 3 retries
        bool      opt_info {false};
        uint32_t  opt_notify_rate {1};		// by default, notify every second
        uint64_t  opt_page_start {0};
        uint64_t  opt_scan_start {0};   // byte where we should start scanning, if not 0
        uint64_t  opt_scan_end {0}; // byte where we should end scanning, if not 0
        time_t    max_wait_time {3600};  // after an hour, terminate a scanner
        int       opt_quiet {false};                  // -1 = no output
        int       retry_seconds {60};
        u_int     num_threads  { std::thread::hardware_concurrency() }; // default to # of cores; 0 for no threads
        double    sampling_fraction {1.0};       // for random sampling
        u_int     sampling_passes {1};
        bool      opt_report_read_errors {true};
        bool      opt_recurse {false};  // -r flag
        void      set_sampling_parameters(std::string p);
        std::atomic<double>    *fraction_done {nullptr};
        bool      opt_legacy {false};
        bool      opt_notification {true}; // run notification thread
        seen_page_ids_t seen_page_ids {};               // pages that were already seen
=======
    /* configuration for phase1 */
    class Config {
    Config &operator=(const Config &);  // not implemented
    Config(const Config &);             // not implemented
    public:
        Config():
            debug(0),
            opt_pagesize(1024*1024*16),
            opt_marginsize(1024*1024*4),
            max_bad_alloc_errors(60),
            opt_info(false),
            opt_notify_rate(4),
            opt_page_start(0),
            opt_offset_start(0),
            opt_offset_end(0),
            max_wait_time(3600),
            opt_quiet(0),
            retry_seconds(60),
            num_threads(1),             //
            sampling_fraction(1.0),
            sampling_passes(1),
            opt_report_read_errors(true) {}

        uint64_t debug;                 // debug
        size_t   opt_pagesize;
        size_t   opt_marginsize;
        uint32_t max_bad_alloc_errors;
        bool     opt_info;
        uint32_t opt_notify_rate;		// by default, notify every 4 pages
        uint64_t opt_page_start;
        int64_t  opt_offset_start;
        int64_t  opt_offset_end;
        time_t   max_wait_time;
        int      opt_quiet;                  // -1 = no output
        int      retry_seconds;
        u_int    num_threads;
        double   sampling_fraction;       // for random sampling
        u_int    sampling_passes;
        bool     opt_report_read_errors;

        void validate(){
#if 0
            if(opt_offset_start % opt_pagesize != 0){
                std::cerr << "WARNING: start offset must be a multiple of the page size.\n";
                opt_offset_start = (opt_offset_start / opt_pagesize) * opt_pagesize;
                std::cerr << "         adjusted to " << opt_offset_start << "\n";
            }
            if(opt_offset_end % opt_pagesize != 0) errx(1,"ERROR: end offset must be a multiple of the page size\n");
#endif
        };
>>>>>>> origin/master
    };

    typedef std::set<uint64_t> blocklist_t; // a list of blocks (for random sampling)
    static std::string minsec(time_t tsec);    // return "5 min 10 sec" string
    static void make_sorted_random_blocklist(blocklist_t *blocklist,uint64_t max_blocks,float frac);

    /* These instance variables reference variables in main.cpp */
    Config        &config;              // phase1 config passed in. Writable so seen can be updated.
    image_process &p;                   // image being processed
    scanner_set   &ss;                  // our scanner set

    u_int         notify_ctr  {0};      // for random sampling
    uint64_t      total_bytes {0};      // processed
    dfxml::sha1_generator *sha1g {nullptr};        // the SHA1 of the image. Set to 0 if a gap is encountered
    uint64_t      hash_next {0};        // next byte to hash, to detect gaps

<<<<<<< HEAD
    std::string image_hash {};          // when hashed, the image hash
    dfxml_writer &xreport;              // we always write out the DFXML. Allows restart to be handled in phase1
=======
public:
    typedef std::set<std::string> seen_page_ids_t;
    /**
     * print the status of a threadpool
     */
    /* Instance variables */
    dfxml_writer &xreport;
    aftimer &timer;
    Config &config;
    u_int   notify_ctr;    /* for random sampling */
    uint64_t total_bytes;               //
    dfxml::md5_generator *md5g;
>>>>>>> origin/master

    /* Get the sbuf from current image iterator location, with retries */
    sbuf_t *get_sbuf(image_process::iterator &it);


    Phase1(Config &config_, image_process &p_, scanner_set &ss_);
    void dfxml_write_create(int argc, char * const *argv); // create the DFXML header
    void dfxml_write_source();                             // create the DFXML <source> block
    void read_process_sbufs(); // read and process the sbufs
    void phase1_run();         // run phase1
};

#endif
