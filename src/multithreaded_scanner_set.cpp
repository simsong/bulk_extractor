#include "multithreaded_scanner_set.h"

/*
 * Called in the worker thread to process the sbuf.
 */
void multithreaded_scanner_set::work_unit::process() const
{
    ss.process_sbuf(sbuf);
}

multithreaded_scanner_set::multithreaded_scanner_set(const scanner_config& sc_,
                                                     const feature_recorder_set::flags_t& f_, class dfxml_writer* writer_):
    scanner_set(sc_, f_, writer_)
{
}


void multithreaded_scanner_set::schedule_sbuf(sbuf_t *sbufp)
{
    std::cerr << "processing sbuf in the same thread. TODO: just add it to the work queue\n";

    if (tp==nullptr) {
        scanner_set::process_sbuf(sbufp);
        return;
    }
    /***************************
     **** SCHEDULE THE WORK ****
     ***************************/

    struct work_unit wu(*this, sbufp);
    tp->push( [wu]{ wu.process(); } );
    //if (config.opt_quiet == false ){
    //notify_user(it);
}

void multithreaded_scanner_set::launch_workers(int count)
{
    tp = new threadpool(count);
}

void multithreaded_scanner_set::join()
{
    if (tp != nullptr) {
        tp->join();
    }
}


/*
 * Print the status of each thread in the threadpool.
 */
void multithreaded_scanner_set::print_tp_status()
{
#if 0
    for(u_int i=0;i<config.num_threads;i++){
        std::string status = tp->get_thread_status(i);
        if (status.size() && status!="Free"){
            std::cout << "Thread " << i << ": " << status << "\n";
        }
    }
    std::cout << "\n";
#endif
}
