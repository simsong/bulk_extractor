#ifndef PHASE1_H
#define PHASE1_H

#include "aftimer.h"
#include "threadpool.h"
#include "image_process.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"


/****************************************************************
 *** Phase 1 BUFFER PROCESSING
 *** For every page of the iterator, schedule work.
 ****************************************************************/

class BulkExtractor_Phase1 {
    /** Sleep for a minimum of msec */
    static void msleep(uint32_t msec);       // sleep for a specified number of msec
    bool sampling(){return config.sampling_fraction<1.0;} // are we random sampling?
    static std::string minsec(time_t tsec);                      // return "5 min 10 sec" string
    void print_tp_status(class threadpool &tp);


public:
    typedef std::set<std::string> seen_page_ids_t;
    struct Config {
        Config():max_bad_alloc_errors(60),
                 opt_notify_rate(4),
                 opt_page_start(0),
                 opt_offset_start(0),
                 opt_offset_end(0),
                 max_wait_time(3600),
                 opt_quiet(0),
                 retry_seconds(60),
                 num_threads(threadpool::numCPU()),
                 sampling_fraction(1.0),
                 sampling_passes(1){}
                 
        uint32_t max_bad_alloc_errors;
        uint32_t opt_notify_rate;		// by default, notify every 4 pages
        uint64_t opt_page_start;
        int64_t opt_offset_start;
        int64_t opt_offset_end;
        time_t max_wait_time;
        int opt_quiet;                  // must be signed
        int retry_seconds;
        u_int num_threads;
        double sampling_fraction;       // for random sampling
        u_int  sampling_passes;

    };

    /**
     * print the status of a threadpool
     */
    /* Instance variables */
    dfxml_writer &xreport;
    aftimer &timer;
    Config &config;
    u_int   notify_ctr;    /* for random sampling */

    /* Get the sbuf from current image iterator location, with retries */
    sbuf_t *get_sbuf(image_process::iterator &it) {
	for(u_int retry_count=0;retry_count<config.max_bad_alloc_errors;retry_count++){
	    try {
		return it.sbuf_alloc(); // may throw exception
	    }
	    catch (const std::bad_alloc &e) {
		// Low memory could come from a bad sbuf alloc or another low memory condition.
		// wait for a while and then try again...
		std::cerr << "Low Memory (bad_alloc) exception: " << e.what()
			  << " reading " << it.get_pos0()
			  << " (retry_count=" << retry_count
			  << " of " << config.max_bad_alloc_errors << ")\n";
		
		std::stringstream ss;
		ss << "name='bad_alloc' " << "pos0='" << it.get_pos0() << "' "
		   << "retry_count='"     << retry_count << "' ";
		xreport.xmlout("debug:exception", e.what(), ss.str(), true);
	    }
	    if(retry_count < config.max_bad_alloc_errors+1){
		std::cerr << "will wait for " << config.retry_seconds << " seconds and try again...\n";
		msleep(config.retry_seconds*1000);
	    }
	}
	std::cerr << "Too many errors encountered in a row. Diagnose and restart.\n";
	exit(1);
    }

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
        xreport(xreport_),timer(timer_),config(config_),notify_ctr(0){}

    void run(image_process &p,feature_recorder_set &fs,
             int64_t &total_bytes, seen_page_ids_t &seen_page_ids) {

        md5_generator *md5g = new md5_generator();		// keep track of MD5
        uint64_t md5_next = 0;					// next byte to hash
        threadpool tp(config.num_threads,fs,xreport);			// 
	uint64_t page_ctr=0;
        xreport.push("runtime","xmlns:debug=\"http://www.afflib.org/bulk_extractor/debug\"");

	std::set<uint64_t> blocks_to_sample;

        /* A single loop with two iterators.
         * it -- the regular image_iterator; it knows how to read blocks.
         * 
         * si -- the sampling iterator. It is a iterator for an STL set.
         * If sampling, si is used to ask for a specific page from it.
         */
        {
            std::set<uint64_t>::const_iterator si = blocks_to_sample.begin(); // sampling iterator
            image_process::iterator it = p.begin(); // sequential iterator
            bool first = true;              
            while(true){
                if(sampling()){
                    if(si==blocks_to_sample.end()) break;
                    if(first) {
                        first = false;
                        /* Create a list of blocks to sample */
                        uint64_t blocks = it.blocks();
                        while(blocks_to_sample.size() < blocks * config.sampling_fraction){
                            uint64_t blk_high = ((uint64_t)random()) << 32;
                            uint64_t blk_low  = random();
                            uint64_t blk =  (blk_high | blk_low) % blocks;
                            blocks_to_sample.insert(blk); // will be added even if already present
                        }
                        
                        si = blocks_to_sample.begin();
                        it.seek_block(*si);
                    }
                } else {
                    /* Not sampling */
                    while (p.begin() == p.end()) {std::cerr <<"BEGIN IS END\n";}
                    if (it == p.end()) break;
                }
                
                if(config.opt_offset_end!=0 && config.opt_offset_end <= it.raw_offset ){
                    break; // passed the offset
                }
                if(config.opt_page_start<=page_ctr && config.opt_offset_start<=it.raw_offset){
                    if(seen_page_ids.find(it.get_pos0().str()) != seen_page_ids.end()){
                        // this page is in the XML file. We've seen it, so skip it (restart code)
                        goto loop;
                    }
                    
                    // attempt to get an sbuf.
                    // If we can't get it, we may be in a low-memory situation.
                    // wait for 30 seconds.
                    
                    try {
                        sbuf_t *sbuf = get_sbuf(it);
                        if(sbuf==0) break;	// eof?
                        sbuf->page_number = page_ctr;
                        
                        /* compute the md5 hash */
                        if(md5g){
                            if(sbuf->pos0.offset==md5_next){ 
                                // next byte follows logically, so continue to compute hash
                                md5g->update(sbuf->buf,sbuf->pagesize);
                                md5_next += sbuf->pagesize;
                            } else {
                                delete md5g; // we had a logical gap; stop hashing
                                md5g = 0;
                            }
                        }
                        total_bytes += sbuf->pagesize;
                        
                        /***************************
                         **** SCHEDULE THE WORK ****
                         ***************************/
                        
                        tp.schedule_work(sbuf);	
                        if(!config.opt_quiet) notify_user(it);
                    }
                    catch (const std::exception &e) {
                        // report uncaught exceptions to both user and XML file
                        std::stringstream ss;
                        ss << "name='" << e.what() << "' " << "pos0='" << it.get_pos0() << "' ";
                        std::cerr << "Exception " << e.what()
                                  << " skipping " << it.get_pos0() << "\n";
                        xreport.xmlout("debug:exception", e.what(), ss.str(), true);
                    }
                }
            loop:;
                /* If we are random sampling, move to the next random sample.
                 * Otherwise increment the it iterator.
                 */
                if(sampling()){
                    ++si;
                    if(si==blocks_to_sample.end()) break;
                    it.seek_block(*si);
                } else {
                    ++it;
                }
                ++page_ctr;
            }
        }
	    
        if(!config.opt_quiet){
            std::cout << "All Data is Read; waiting for threads to finish...\n";
        }

        /* Now wait for all of the threads to be free */
        tp.mode = 1;			// waiting for workers to finish
        time_t wait_start = time(0);
        for(int32_t counter = 0;;counter++){
            int num_remaining = config.num_threads - tp.get_free_count();
            if(num_remaining==0) break;

            msleep(100);
            time_t time_waiting   = time(0) - wait_start;
            time_t time_remaining = config.max_wait_time - time_waiting;

            if(counter%60==0){
                std::stringstream ss;
                ss << "Time elapsed waiting for " << num_remaining
                   << " thread" << (num_remaining>1 ? "s" : "") 
                   << " to finish:\n    " << minsec(time_waiting) 
                   << " (timeout in "     << minsec(time_remaining) << ".)\n";
                if(config.opt_quiet==0){
                    std::cout << ss.str();
                    if(counter>0) print_tp_status(tp);
                }
                xreport.comment(ss.str());
            }
            if(time_waiting>config.max_wait_time){
                std::cout << "\n\n";
                std::cout << " ... this shouldn't take more than an hour. Exiting ... \n";
                std::cout << " ... Please report to the bulk_extractor maintainer ... \n";
                break;
            }
        }
        if(config.opt_quiet==0) std::cout << "All Threads Finished!\n";
	
        xreport.pop();			// pop runtime
        /* We can write out the source info now, since we (might) know the hash */
        xreport.push("source");
        xreport.xmlout("image_filename",p.image_fname());
        xreport.xmlout("image_size",p.image_size());  
        if(md5g){
            md5_t md5 = md5g->final();
            xreport.xmlout("hashdigest",md5.hexdigest(),"type='MD5'",false);
            delete md5g;
        }
        xreport.pop();			// source

        /* Record the feature files and their counts in the output */
        xreport.push("feature_files");
        for(feature_recorder_map::const_iterator ij = tp.fs.frm.begin();
            ij != tp.fs.frm.end(); ij++){
            xreport.set_oneline(true);
            xreport.push("feature_file");
            xreport.xmlout("name",ij->second->name);
            xreport.xmlout("count",ij->second->count);
            xreport.pop();
            xreport.set_oneline(false);
        }

        if(config.opt_quiet==0) std::cout << "Producer time spent waiting: " << tp.waiting.elapsed_seconds() << " sec.\n";
    
        xreport.xmlout("thread_wait",dtos(tp.waiting.elapsed_seconds()),"thread='0'",false);
        double worker_wait_average = 0;
        for(threadpool::worker_vector::const_iterator ij=tp.workers.begin();ij!=tp.workers.end();ij++){
            worker_wait_average += (*ij)->waiting.elapsed_seconds() / config.num_threads;
            std::stringstream ss;
            ss << "thread='" << (*ij)->id << "'";
            xreport.xmlout("thread_wait",dtos((*ij)->waiting.elapsed_seconds()),ss.str(),false);
        }
        xreport.pop();
        xreport.flush();
        if(config.opt_quiet==0) std::cout << "Average consumer time spent waiting: " << worker_wait_average << " sec.\n";
        if(worker_wait_average > tp.waiting.elapsed_seconds()*2 && worker_wait_average>10 && config.opt_quiet==0){
            std::cout << "*******************************************\n";
            std::cout << "** bulk_extractor is probably I/O bound. **\n";
            std::cout << "**        Run with a faster drive        **\n";
            std::cout << "**      to get better performance.       **\n";
            std::cout << "*******************************************\n";
        }
        if(tp.waiting.elapsed_seconds() > worker_wait_average * 2 && tp.waiting.elapsed_seconds()>10 && config.opt_quiet==0){
            std::cout << "*******************************************\n";
            std::cout << "** bulk_extractor is probably CPU bound. **\n";
            std::cout << "**    Run on a computer with more cores  **\n";
            std::cout << "**      to get better performance.       **\n";
            std::cout << "*******************************************\n";
        }
        /* end of phase 1 */
    }
};

#endif
