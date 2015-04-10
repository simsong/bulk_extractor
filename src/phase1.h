#ifndef PHASE1_H
#define PHASE1_H

#include "be13_api/aftimer.h"
#include "threadpool.h"
#include "image_process.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"


/****************************************************************
 *** Phase 1 BUFFER PROCESSING
 *** For every page of the iterator, schedule work.
 ****************************************************************/

class BulkExtractor_Phase1 {
public:
    typedef std::set<uint64_t> blocklist_t; // a list of blocks
    static void make_sorted_random_blocklist(blocklist_t *blocklist,uint64_t max_blocks,float frac);

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
    };
private:

    /** Sleep for a minimum of msec */
    static void msleep(uint32_t msec);       // sleep for a specified number of msec
    bool sampling(){return config.sampling_fraction<1.0;} // are we random sampling?
    static std::string minsec(time_t tsec);                      // return "5 min 10 sec" string

    class threadpool *tp;
    void print_tp_status();


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
    md5_generator *md5g;

    /* Get the sbuf from current image iterator location, with retries */
    sbuf_t *get_sbuf(image_process::iterator &it);

    /* Notify user about current state of phase1 */
    void notify_user(image_process::iterator &it){
	if(notify_ctr++ >= config.opt_notify_rate){
	    time_t t = time(0);
	    struct tm tm;
	    localtime_r(&t,&tm);
	    printf("%2d:%02d:%02d %s ",tm.tm_hour,tm.tm_min,tm.tm_sec,it.str().c_str());

	    /* not sure how to do the rest if sampling */
	    if(!sampling()){
		printf("(%4.2f%%) Done in %s at %s",
		       it.fraction_done()*100.0,
		       timer.eta_text(it.fraction_done()).c_str(),
		       timer.eta_time(it.fraction_done()).c_str());
	    }
	    printf("\n");
	    fflush(stdout);
	    notify_ctr = 0;
	}
    }

public:
    static void set_sampling_parameters(Config &c,std::string &p){
	std::vector<std::string> params = split(p,':');
	if(params.size()!=1 && params.size()!=2){
	    errx(1,"error: sampling parameters must be fraction[:passes]");
	}
	c.sampling_fraction = atof(params.at(0).c_str());
	if(c.sampling_fraction<=0 || c.sampling_fraction>=1){
	    errx(1,"error: sampling fraction f must be 0<f<=1; you provided '%s'",params.at(0).c_str());
	}
	if(params.size()==2){
	    c.sampling_passes = atoi(params.at(1).c_str());
	    if(c.sampling_passes==0){
		errx(1,"error: sampling passes must be >=1; you provided '%s'",params.at(1).c_str());
	    }
	}
    }

#ifndef HAVE_RANDOM
#define random(x) rand(x)
#endif

    BulkExtractor_Phase1(dfxml_writer &xreport_,aftimer &timer_,Config &config_):
        tp(),xreport(xreport_),timer(timer_),config(config_),notify_ctr(0),total_bytes(0),md5g(){}

    void run(image_process &p,feature_recorder_set &fs, seen_page_ids_t &seen_page_ids);
    void wait_for_workers(image_process &p,std::string *md5_string);
};

#endif
