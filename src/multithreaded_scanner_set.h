/*
 * subclasses the scanner_set to make it multi-threaded.
 */

#ifndef MULTITHREADED_SCANNER_SET_H
#define MULTITHREADED_SCANNER_SET_H
#include "threadpool.hpp"
#include "be13_api/scanner_set.h"
class multithreaded_scanner_set: public scanner_set {
    static const inline size_t SAME_THREAD_SBUF_SIZE = 8192; // sbufs smaller than this run in the same thread.
    multithreaded_scanner_set(const scanner_set& s) = delete;
    multithreaded_scanner_set& operator=(const scanner_set& s) = delete;
    class threadpool *tp {nullptr};     // nullptr means we are not threading

    /* The thread pool contains a queue of std::function<void()>, which are calls to work_unit::process() */
    struct  work_unit {
        work_unit(multithreaded_scanner_set &ss_,sbuf_t *sbuf_):ss(ss_),sbuf(sbuf_){}
        multithreaded_scanner_set &ss;
        sbuf_t *sbuf {};
        void process() const;
    };
    std::atomic<int> sbuf_depth0 {0};
    [[noreturn]] void notify_thread();               // notify what's going on
    std::thread *nt {nullptr};

public:
    multithreaded_scanner_set(const scanner_config& sc, const feature_recorder_set::flags_t& f, class dfxml_writer* writer);
    void launch_workers(int count);
    virtual void process_sbuf(sbuf_t* sbuf) override; // process the sbuf, then delete it.
    virtual void schedule_sbuf(sbuf_t* sbuf) override; // process the sbuf, then delete it.
    virtual void delete_sbuf(sbuf_t* sbuf) override; // process the sbuf, then delete it.
    virtual void set_status(const std::string &status) override; // set the status for this thread
    virtual void print_tp_status();                  // print the status of each thread
    void join();                        // join the threads
    void run_notify_thread();               // notify what's going on
};
#endif
