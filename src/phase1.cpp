#include "config.h"
#include "bulk_extractor.h"
#include "phase1.h"
#include "threadpool.h"

void BulkExtractor_Phase1::msleep(uint32_t msec)
{
#if _WIN32
	Sleep(msec);
#else
# ifdef HAVE_USLEEP
	usleep(msec*1000);
# else
	int sec = msec/1000;
	if(sec<1) sec=1;
	sleep(sec);			// posix
# endif
#endif
}

std::string BulkExtractor_Phase1::minsec(time_t tsec)
{
    time_t min = tsec / 60;
    time_t sec = tsec % 60;
    std::stringstream ss;
    if(min>0) ss << min << " min";
    if(sec>0) ss << sec << " sec";
    return ss.str();
}

void BulkExtractor_Phase1::print_tp_status()
{
    std::stringstream ss;
    for(u_int i=0;i<config.num_threads;i++){
        std::string status = tp->get_thread_status(i);
        if(status.size() && status!="Free"){
            ss << "Thread " << i << ": " << status << "\n";
        }
    }
    std::cout << ss.str() << "\n";
}

/**
 * attempt to get an sbuf. If we can't get it, we may be in a
 * low-memory situation.  wait for 30 seconds.
 */

sbuf_t *BulkExtractor_Phase1::get_sbuf(image_process::iterator &it)
{
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


void BulkExtractor_Phase1::make_sorted_random_blocklist(blocklist_t *blocklist,uint64_t max_blocks,float frac)
{
    while(blocklist->size() < max_blocks * frac){
        uint64_t blk_high = ((uint64_t)random()) << 32;
        uint64_t blk_low  = random();
        uint64_t blk      = (blk_high | blk_low) % max_blocks;
        blocklist->insert(blk); // will be added even if already present
    }
}


void BulkExtractor_Phase1::run(image_process &p,feature_recorder_set &fs,
                               seen_page_ids_t &seen_page_ids)
{
    p.set_report_read_errors(config.opt_report_read_errors);
    md5g = new md5_generator();		// keep track of MD5
    uint64_t md5_next = 0;              // next byte to hash

    if(config.debug & DEBUG_PRINT_STEPS) std::cout << "DEBUG: CREATING THREAD POOL\n";
    
    tp = new threadpool(config.num_threads,fs,xreport);	

    uint64_t page_ctr=0;
    xreport.push("runtime","xmlns:debug=\"http://www.github.com/simsong/bulk_extractor/issues\"");

    /* A single loop with two iterators.
     *
     * it -- the regular image_iterator; it knows how to read blocks.
     * 
     * si -- the sampling iterator. It is a iterator for an STL set.
     *
     * If sampling, si is used to ask for a specific page from it.
     */
    blocklist_t blocks_to_sample;
    blocklist_t::const_iterator si = blocks_to_sample.begin(); // sampling iterator
    image_process::iterator     it = p.begin(); // sequential iterator

    if(config.opt_offset_start){
        std::cout << "offset set to " << config.opt_offset_start << "\n";
        it.set_raw_offset(config.opt_offset_start);
    }

    if(sampling()){
        /* Create a list of blocks to sample */
        make_sorted_random_blocklist(&blocks_to_sample,it.max_blocks(),config.sampling_fraction);
        si = blocks_to_sample.begin();    // get the new beginning
    }
    /* Loop over the blocks to sample */
    while(true){
        if(sampling()){
            if(si==blocks_to_sample.end()) break;
            it.seek_block(*si);
        } else {
            /* Not sampling; no need to seek, it's the next one */
            if (it == p.end()){
                break;
            }
        }
                
        if(config.opt_offset_end!=0 && config.opt_offset_end <= it.raw_offset ){
            break;                      // passed the offset
        }
        if(config.opt_page_start<=page_ctr && config.opt_offset_start<=it.raw_offset){
            // Make sure we haven't done this page yet
            if(seen_page_ids.find(it.get_pos0().str()) == seen_page_ids.end()){
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
                        
                    tp->schedule_work(sbuf);	
                    if(!config.opt_quiet) notify_user(it);
                }
                catch (const std::exception &e) {
                    // report uncaught exceptions to both user and XML file
                    std::stringstream ss;
                    ss << "name='" << e.what() << "' " << "pos0='" << it.get_pos0() << "' ";

                    if (config.opt_report_read_errors) {
                        std::cerr << "Exception " << e.what() << " skipping " << it.get_pos0() << "\n";
                    }
                    xreport.xmlout("debug:exception", e.what(), ss.str(), true);
                }
            }
        } // end that we haven't seen it

        /* If we are random sampling, move to the next random sample.
         * Otherwise increment the it iterator.
         */
        if(sampling()){
            ++si;
        } else {
            ++it;
        }
        ++page_ctr;
    }
	    
    if(!config.opt_quiet){
        std::cout << "All data are read; waiting for threads to finish...\n";
    }
}

void BulkExtractor_Phase1::wait_for_workers(image_process &p,std::string *md5_string)
{
    /* Now wait for all of the threads to be free */
    tp->mode = 1;			// waiting for workers to finish
    time_t wait_start = time(0);
    for(int32_t counter = 0;;counter++){
        int num_remaining = config.num_threads - tp->get_free_count();
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
                if(counter>0) print_tp_status();
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
        if(md5_string) *md5_string = md5.hexdigest();
        xreport.xmlout("hashdigest",md5.hexdigest(),"type='MD5'",false);
        delete md5g;
    }
    xreport.pop();			// source

    /* Record the feature files and their counts in the output */
    tp->fs.dump_name_count_stats(xreport);

    if(config.opt_quiet==0) std::cout << "Producer time spent waiting: " << tp->waiting.elapsed_seconds() << " sec.\n";
    
    xreport.xmlout("thread_wait",dtos(tp->waiting.elapsed_seconds()),"thread='0'",false);
    double worker_wait_average = 0;
    for(threadpool::worker_vector::const_iterator ij=tp->workers.begin();ij!=tp->workers.end();ij++){
        worker_wait_average += (*ij)->waiting.elapsed_seconds() / config.num_threads;
        std::stringstream ss;
        ss << "thread='" << (*ij)->id << "'";
        xreport.xmlout("thread_wait",dtos((*ij)->waiting.elapsed_seconds()),ss.str(),false);
    }
    xreport.pop();
    xreport.flush();
    if(config.opt_quiet==0) std::cout << "Average consumer time spent waiting: " << worker_wait_average << " sec.\n";
    if(worker_wait_average > tp->waiting.elapsed_seconds()*2
       && worker_wait_average>10 && config.opt_quiet==0){
        std::cout << "*******************************************\n";
        std::cout << "** bulk_extractor is probably I/O bound. **\n";
        std::cout << "**        Run with a faster drive        **\n";
        std::cout << "**      to get better performance.       **\n";
        std::cout << "*******************************************\n";
    }
    if(tp->waiting.elapsed_seconds() > worker_wait_average * 2
       && tp->waiting.elapsed_seconds()>10 && config.opt_quiet==0){
        std::cout << "*******************************************\n";
        std::cout << "** bulk_extractor is probably CPU bound. **\n";
        std::cout << "**    Run on a computer with more cores  **\n";
        std::cout << "**      to get better performance.       **\n";
        std::cout << "*******************************************\n";
    }
    /* end of phase 1 */
}
