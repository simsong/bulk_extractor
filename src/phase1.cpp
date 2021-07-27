#include <random>
#include <chrono>
#include <thread>
#include <chrono>

#include "config.h"
#include "phase1.h"
#include "be13_api/utils.h"             // needs config.h


/**
 * Implementation of the bulk_extractor Phase 1.
 *
 * bulk_extractor 1.0:
 * - run() creates worker threads.
 *   -  main thread stuffs the work queue with sbufs to process.
 *   -  workers remove each sbuf and process with each scanner.
 *   -  recursive work is processed within each thread.
 *
 * bulk_extractor 2.0:
 * - implements 1.0 mechanism.
 * - implements 2.0 mechanism, which uses a work unit for each sbuf/scanner combination.
 */

using namespace std::chrono_literals;

Phase1::Phase1(Config config_, image_process &p_, scanner_set &ss_):
    config(config_), p(p_), ss(ss_), xreport(*ss_.get_dfxml_writer())
{
}

/**
 * convert tsec into a string.
 */

std::string Phase1::minsec(time_t tsec)
{
    time_t min = tsec / 60;
    time_t sec = tsec % 60;
    std::stringstream ss;
    if (min>0) ss << min << " min";
    if (sec>0) ss << sec << " sec";
    return ss.str();
}

void Phase1::Config::set_sampling_parameters(std::string param)
{
    std::vector<std::string> params = split(param,':');
    if (params.size()!=1 && params.size()!=2){
        throw std::runtime_error("sampling parameters must be fraction[:passes]");
    }
    sampling_fraction = atof(params.at(0).c_str());
    if (sampling_fraction<=0 || sampling_fraction>=1){
        throw std::runtime_error("error: sampling fraction f must be 0<f<=1");
    }
    if (params.size()==2){
        sampling_passes = atoi(params.at(1).c_str());
        if (sampling_passes==0){
            throw std::runtime_error("error: sampling passes must be >=1");
        }
    }
}



/**
 * attempt to get an sbuf. If we can't get it, we may be in a
 * low-memory situation.  wait for 30 seconds.
 *
 * TODO: do not get an sbuf if more than N tasks in workqueue.
 */

sbuf_t *Phase1::get_sbuf(image_process::iterator &it)
{
    assert(config.max_bad_alloc_errors>0);
    for(u_int retry_count=0;retry_count<config.max_bad_alloc_errors;retry_count++){
        try {
            return p.sbuf_alloc(it); // may throw exception
        }
        catch (const std::bad_alloc &e) {
            // Low memory could come from a bad sbuf alloc or another low memory condition.
            // wait for a while and then try again...
            std::cerr << "Low Memory (bad_alloc) exception: " << e.what()
                      << " reading " << it.get_pos0()
                      << " (retry_count=" << retry_count
                      << " of " << config.max_bad_alloc_errors << ")\n";

            std::stringstream str;
            str << "name='bad_alloc' " << "pos0='" << it.get_pos0() << "' " << "retry_count='"     << retry_count << "' ";
            xreport.xmlout("debug:exception", e.what(), str.str(), true);
        }
        if (retry_count < config.max_bad_alloc_errors+1){
            std::cerr << "will wait for " << config.retry_seconds << " seconds and try again...\n";
            std::this_thread::sleep_for(config.retry_seconds * 1000ms);
        }
    }
    std::cerr << "Too many errors encountered in a row. Diagnose and restart.\n";
    throw std::runtime_error("too many sbuf allocation errors");
}


/**
 * Create a list sorted list of random blocks.
 */
void Phase1::make_sorted_random_blocklist(blocklist_t *blocklist,uint64_t max_blocks,float frac)
{
    if (frac>0.2){
        std::cerr << "A better random sampler is needed for this many random blocks. \n"
                  << "We should have an iterator that decides if to include or not to include based on frac.\n";
        throw std::runtime_error("Specify frac<0.2");
    }

    std::default_random_engine generator;
    std::uniform_int_distribution<uint64_t> distribution(0,max_blocks);
    while(blocklist->size() < max_blocks * frac){
        blocklist->insert( distribution(generator) ); // will be added even if already present
    }
}

void Phase1::read_process_sbufs()
{
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

    if (config.opt_offset_start){
        std::cout << "offset set to " << config.opt_offset_start << "\n";
        it.set_raw_offset(config.opt_offset_start);
    }

    if (sampling()){
        /* Create a list of blocks to sample */
        std::cerr << "sampling\n";
        make_sorted_random_blocklist(&blocks_to_sample,it.max_blocks(),config.sampling_fraction);
        si = blocks_to_sample.begin();    // get the new beginning
    } else {
        /* Not sampling */
        sha1g = new dfxml::sha1_generator();
    }
    /* Loop over the blocks to sample */
    while(it != p.end()) {
        if (sampling()){                // if sampling, seek the iterator
            if (si==blocks_to_sample.end()) break;
            it.seek_block(*si);
        }
        /* If we have gone to far, break */
        if (config.opt_offset_end!=0 && config.opt_offset_end <= it.raw_offset ){
            break;                      // passed the offset
        }

        /* If there are too many in the queue, wait... */
        std::cerr << "sbufs_in_queue=" << ss.sbufs_in_queue << "\n";
        if (ss.depth0_sbufs_in_queue > ss.get_thread_count()) {
            using namespace std::chrono_literals;
            std::cerr << "too many depth0 sbufs in queue! waiting for 1 second\n";
            std::this_thread::sleep_for(2000ms);
            continue;
        }

        if (config.opt_page_start<=it.page_number && config.opt_offset_start<=it.raw_offset){
            // Make sure we haven't done this page yet. This should never happen
            if (seen_page_ids.find(it.get_pos0().str()) != seen_page_ids.end()){
                std::cerr << "Phase 1 error: " << it.get_pos0().str() << " provided twice\n";
                throw std::runtime_error("phase1 error - page provided twice");
            }
            try {
                sbuf_t *sbufp = get_sbuf(it);

                std::stringstream attribute;
                attribute << "t='" << time(0) << "'";
                xreport.xmlout("sbuf_read",sbufp->pos0.str(), attribute.str(), false);

                /* compute the sha1 hash */
                if (sha1g){
                    if (sbufp->pos0.offset==sha1_next){
                        // next byte follows logically, so continue to compute hash
                        sha1g->update(sbufp->get_buf(), sbufp->pagesize);
                        sha1_next += sbufp->pagesize;
                    } else {
                        delete sha1g; // we had a logical gap; stop hashing
                        sha1g = 0;
                    }
                }
                total_bytes += sbufp->pagesize;
                ss.schedule_sbuf(sbufp); // processes the sbuf, then deletes it
            }
            catch (const std::exception &e) {
                // report uncaught exceptions to both user and XML file
                std::stringstream sstr;
                sstr << "phase=1 name='" << e.what() << "' " << "pos0='" << it.get_pos0() << "' ";

                if (config.opt_report_read_errors) {
                    std::cerr << "Phase 1 Exception " << e.what() << " skipping " << it.get_pos0() << "\n";
                }
                xreport.xmlout("debug:exception", e.what(), sstr.str(), true);
            }
        }

        /* If we are random sampling, move to the next random sample. */
        if (sampling()){
            ++si;
        }
        ++it;
    }

    if (!config.opt_quiet){
        std::cout << "All data are read; waiting for threads to finish...\n";
    }
}

#if 0
TODO: Turn this into a real-time status thread.
    /* Now wait for all of the threads to be free */
    time_t wait_start = time(0);
    for(int32_t counter = 0;;counter++){
        //int num_remaining = config.num_threads - tp->get_free_count();
        //if (num_remaining==0) break;

        std::this_thread::sleep_for(100ms);
        time_t time_waiting   = time(0) - wait_start;
        time_t time_remaining = config.max_wait_time - time_waiting;

        if (counter%60==0){
            std::stringstream sstr;
            sstr << "Time elapsed waiting for "
                // << num_remaining
                // << " thread" << (num_remaining>1 ? "s" : "")
               << " to finish:\n    " << minsec(time_waiting)
               << " (timeout in "     << minsec(time_remaining) << ".)\n";
            if (config.opt_quiet==0){
                std::cout << sstr.str();
                if (counter>0) print_tp_status();
            }
            xreport.comment(sstr.str());
        }
        if (time_waiting>config.max_wait_time){
            std::cout << "\n\n";
            std::cout << " ... this shouldn't take more than an hour. Exiting ... \n";
            std::cout << " ... Please report to the bulk_extractor maintainer ... \n";
            break;
        }
    }
    if (config.opt_quiet==0) std::cout << "All Threads Finished!\n";
#endif


void Phase1::dfxml_write_create(int argc, char * const *argv)
{
    xreport.push("dfxml","xmloutputversion='1.0'");
    xreport.push("metadata",
		 "\n  xmlns='http://afflib.org/bulk_extractor/' "
		 "\n  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
		 "\n  xmlns:dc='http://purl.org/dc/elements/1.1/'" );
    xreport.xmlout("dc:type","Feature Extraction","",false);
    xreport.pop();
    if (argc && argv){
        xreport.add_DFXML_creator(PACKAGE_NAME, PACKAGE_VERSION, "", argc, argv);
    }
    xreport.push("configuration");
    xreport.xmlout("threads",config.num_threads);
    xreport.xmlout("pagesize",config.opt_pagesize);
    xreport.xmlout("marginsize",config.opt_marginsize);
    xreport.push("scanners");

    /* Generate a list of the scanners in use */
    auto ev = ss.get_enabled_scanners();
    for (const auto &it : ev) {
        xreport.xmlout("scanner",it);
    }
    xreport.pop("scanners");		// scanners
    xreport.pop("configuration");	// configuration
    xreport.flush();                    // get it to the disk
}

void Phase1::dfxml_write_source()
{
    /* We can write out the source info now, since we (might) know the hash */
    xreport.push("source");
    xreport.xmlout("image_filename",p.image_fname());
    xreport.xmlout("image_size",p.image_size());
    if (sha1g){
        dfxml::sha1_t sha1 = sha1g->digest();
        xreport.xmlout("hashdigest",sha1.hexdigest(),"type='SHA1'",false);
        delete sha1g;
    }
    xreport.pop("source");			// source
    xreport.flush();

    //if (config.opt_quiet==0) std::cout << "Producer time spent waiting: " << tp->waiting.elapsed_seconds() << " sec.\n";

    //xreport.xmlout("thread_wait",dtos(tp->waiting.elapsed_seconds()),"thread='0'",false);
    double worker_wait_average = 0;
    if (config.opt_quiet==0) {
        std::cout << "Average consumer time spent waiting: " << worker_wait_average << " sec.\n";
    }
#if 0
    if (worker_wait_average > tp->waiting.elapsed_seconds()*2
       && worker_wait_average>10 && config.opt_quiet==0){
        std::cout << "*******************************************\n";
        std::cout << "** bulk_extractor is probably I/O bound. **\n";
        std::cout << "**        Run with a faster drive        **\n";
        std::cout << "**      to get better performance.       **\n";
        std::cout << "*******************************************\n";
    }
    if (tp->waiting.elapsed_seconds() > worker_wait_average * 2
       && tp->waiting.elapsed_seconds()>10 && config.opt_quiet==0){
        std::cout << "*******************************************\n";
        std::cout << "** bulk_extractor is probably CPU bound. **\n";
        std::cout << "**    Run on a computer with more cores  **\n";
        std::cout << "**      to get better performance.       **\n";
        std::cout << "*******************************************\n";
    }
#endif
    /* end of phase 1 */
}


void Phase1::phase1_run()
{
    assert(ss.get_current_phase() == scanner_params::PHASE_SCAN);
    //ss.run_notify_thread();
    /* Create the threadpool and launch the workers */
    //p.set_report_read_errors(config.opt_report_read_errors);
    xreport.push("runtime","xmlns:debug=\"http://www.github.com/simsong/bulk_extractor/issues\"");
    read_process_sbufs();
    ss.join();
    xreport.pop("runtime");
    dfxml_write_source();               // written here so it may also include hash
}
