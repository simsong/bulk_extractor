#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

/****************************************************************
 *** THREADING SUPPORT
 ****************************************************************/

/**
 * \addtogroup internal_interfaces
 * @{
 */


/**
 * \file
 * http://stackoverflow.com/questions/4264460/wait-for-one-of-several-threads-to-finish
 * Here is the algorithm to run the thread pool with a work queue:
 *
 * \verbatim
 * main:
 *     set freethreads to numthreads
 *     init mutex M, condvars TOMAIN and TOWORKER
 *     start N worker threads
 *     while true:
 *         wait for work item
 *         claim M
 *         while freethreads == 0:
 *             cond-wait TOMAIN, M
 *         put work item in queue
 *         decrement freethreads
 *         cond-signal TOWORKER
 *         release M
 * 
 * worker:
 *     init
 *     while true:
 *         claim M
 *         while no work in queue:
 *             cond-wait TOWORKER, M
 *         get work to local storage
 *         release M
 *         do work
 *         claim M
 *         increment freethreads
 *         cond-signal TOMAIN
 *         release M
 * \endverbatim
 */

#include <queue>
#include <pthread.h>
#include "be13_api/aftimer.h"
#include "dfxml/src/dfxml_writer.h"

// There is a single threadpool object
class threadpool {
 private:
    /*** neither copying nor assignment is implemented ***/
    threadpool(const threadpool &);
    threadpool &operator=(const threadpool &);
    
 public:
#ifdef WIN32
    static void win32_init();		// must be called on win32
#endif
    typedef std::vector<class worker *> worker_vector;
    worker_vector	workers;
    pthread_mutex_t	M;		// protects the following variables
    pthread_cond_t	TOMAIN;
    pthread_cond_t	TOWORKER;
    int			freethreads;
    std::queue<sbuf_t *> work_queue;	// work to be done
    feature_recorder_set &fs;		// one for all the threads; fs and fr are threadsafe
    dfxml_writer	&xreport;	// where the xml gets written; threadsafe
    std::vector<std::string> thread_status;	// for each thread, its status
    aftimer		waiting;	// time spend waiting
    int			mode;		// 0=running; 1 = waiting for workers to finish

    static u_int	numCPU();

    threadpool(int numthreads,feature_recorder_set &fs_,dfxml_writer &xreport);
    virtual ~threadpool();
    void		schedule_work(sbuf_t *sbuf);
    bool		all_free();
    int			get_free_count();
    std::string		get_thread_status(uint32_t id);
    void		set_thread_status(uint32_t id, const std::string &status );
};

// there is a worker object for each thread
class worker {
private:
    void do_work(sbuf_t *sbuf);		// do the work; does not delete sbuf
    class internal_error: public std::exception {
        virtual const char *what() const throw() {
            return "internal error.";
        }
    };
public:
    static bool opt_work_start_work_end; // report when work starts and when work ends
    static void * start_worker(void *arg){return ((worker *)arg)->run();};
    class threadpool &master;		// my master
    pthread_t thread;			// my thread; set when I am created
    uint32_t id;				// my number
    worker(class threadpool &master_,uint32_t id_): master(master_),thread(),id(id_),waiting(){}
    void *run();
    aftimer		waiting;	// time spend waiting
};

#endif
