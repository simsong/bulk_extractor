#include <random>
#include <chrono>
#include <thread>
#include <chrono>

#include "config.h"
#include "phase1.h"
#include "be13_api/utils.h"             // needs config.h
#include "be13_api/aftimer.h"             // needs config.h
#include "be13_api/dfxml_cpp/src/dfxml_writer.h"


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

#include "phase1.h"

using namespace std::chrono_literals;

Phase1::Phase1(Config &config_, image_process &p_, scanner_set &ss_, std::ostream &cout_):
    config(config_), p(p_), ss(ss_), cout(cout_), xreport(*ss_.get_dfxml_writer())
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
        std::cerr << "params.at(0): " << params.at(0) << std::endl;
        std::cerr << "sampling_fraction: " << sampling_fraction << std::endl;
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
            str << "name='bad_alloc' " << "pos0='" << dfxml_writer::xmlescape(it.get_pos0().str()) << "' " << "retry_count='"     << retry_count << "' ";
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

    if (config.opt_scan_start){
        std::cout << "offset set to " << config.opt_scan_start << "\n";
        it.set_raw_offset(config.opt_scan_start);
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
        /* If there is a disk write error, shut down */
        if (ss.disk_write_errors > 0 ){
            for(int i=0;i<5;i++){
                std::cerr << std::endl;
            }
            std::cerr << "*** DISK WRITE ERRORS (" << ss.disk_write_errors << ") ***" << std::endl;
            std::cerr << "Disk is likely full. Clear space and restart (press up arrow) " << std::endl;
	    std::cerr << "Check alerts.txt for further info" << std::endl;
            exit(1);
        }

        if (sampling()){                // if sampling, seek the iterator
            if (si==blocks_to_sample.end()) break;
            it.seek_block(*si);
        }
        /* If we have gone to far, break */
        if (config.opt_scan_end!=0 && config.opt_scan_end <= it.raw_offset ){
            break;                      // passed the offset
        }

        /* If there are too many in the queue, wait... */
        if (ss.depth0_sbufs_in_queue > ss.get_worker_count()) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2000ms);
            continue;
        }

        if (config.opt_page_start<=it.page_number && config.opt_scan_start<=it.raw_offset){
            // Only process pages we haven't seen before
            if (config.seen_page_ids.find(it.get_pos0().str()) == config.seen_page_ids.end()){
                try {
                    sbuf_t *sbufp = get_sbuf(it);

                    /* compute the sha1 hash */
                    if (sha1g){
                        if (sbufp->pos0.offset==hash_next){
                            // next byte follows logically, so continue to compute hash
                            sha1g->update(sbufp->get_buf(), sbufp->pagesize);
                            hash_next += sbufp->pagesize;

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
        }


        /* If we are random sampling, move to the next random sample. */
        if (sampling()){
            ++si;
        }

        /* Report back the fraction done if requested */
        if (config.fraction_done) *config.fraction_done = p.fraction_done(it);
        ++it;
    }

    if (config.fraction_done) *config.fraction_done = 1.0;
}

void Phase1::dfxml_write_create(int argc, char * const *argv)
{
    xreport.push("dfxml","xmloutputversion='1.0' xmlns:debug='http://afflib.org/bulk_extractor/debug'");
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
    ss.dump_enabled_scanner_config();
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
    if (!config.opt_quiet && worker_wait_average>0) {
        std::cout << "Average consumer time spent waiting: " << worker_wait_average << " sec." << std::endl;
    }
    /* end of phase 1 */
}


void Phase1::phase1_run()
{
    assert(ss.get_current_phase() == scanner_params::PHASE_SCAN);
    // save all of the pages we have seen in the DFXML file
    for (const auto &it : config.seen_page_ids) {
        ss.record_work_start_pos0str( it );
    }
    xreport.push("runtime","xmlns:debug=\"http://www.github.com/simsong/bulk_extractor/issues\"");
    read_process_sbufs();
    if (!config.opt_quiet) cout << "All data read; waiting for threads to finish..." << std::endl;
    ss.join();
    xreport.pop("runtime");
    dfxml_write_source();               // written here so it may also include hash
}
