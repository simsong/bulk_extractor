#ifndef THREADPOOL_H

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
#include "aftimer.h"

// There is a single threadpool object
class threadpool {
 private:
    /*** neither copying nor assignment is implemented ***
     *** We do this by making them private constructors that throw exceptions. ***/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying feature_recorder objects is not implemented.";
	}
    };
 threadpool(const threadpool &t):workers(),M(),TOMAIN(),TOWORKER(),freethreads(),
    work_queue(),fs(t.fs),xreport(t.xreport),thread_status(),waiting(),mode(){
    throw new not_impl();
  }
  const threadpool &operator=(const threadpool &t){throw new not_impl(); }
    
 public:
#ifdef WIN32
    static void win32_init();		// must be called on win32
#endif
    typedef vector<class worker *> worker_vector;
    worker_vector	workers;
    pthread_mutex_t	M;			// protects the following variables
    pthread_cond_t	TOMAIN;
    pthread_cond_t	TOWORKER;
    int			freethreads;
    queue<sbuf_t *>	work_queue;	// work to be done
    feature_recorder_set &fs;		// one for all the threads; fs and fr are threadsafe
    xml			&xreport;	// where the xml gets written; threadsafe
    vector<string>	thread_status;	// for each thread, its status
    aftimer		waiting;	// time spend waiting
    int			mode;		// 0=running; 1 = waiting for workers to finish

    static int		numCPU();

    threadpool(int numthreads,feature_recorder_set &fs_,xml &xreport);
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
    class internal_error: public exception {
        virtual const char *what() const throw() {
            return "internal error.";
        }
    };
public:
    static void * start_worker(void *arg){return ((worker *)arg)->run();};
    class threadpool &master;		// my master
    pthread_t thread;			// my thread; set when I am created
    uint32_t id;				// my number
    worker(class threadpool &master_,uint32_t id_): master(master_),thread(),id(id_),waiting(){}
    void *run();
    aftimer		waiting;	// time spend waiting
};

#endif
