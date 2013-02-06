#include "bulk_extractor.h"
#include "aftimer.h"
#include "image_process.h"
#include "aftimer.h"
#include "xml.h"
#include "threadpool.h"

#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <set>
#include <setjmp.h>
#include <vector>
#include <queue>
#include <unistd.h>


/* Return the number of CPUs we have on various architectures.
 * From http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
 */

int threadpool::numCPU()
{
    int numCPU=1;			// default
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    numCPU = sysinfo.dwNumberOfProcessors;
#endif
#if defined(HW_AVAILCPU) && defined(HW_NCPU)
    int mib[4];
    size_t len=sizeof(numCPU);

    /* set the mib for hw.ncpu */
    memset(mib,0,sizeof(mib));
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    /* get the number of CPUs from the system */
    if(sysctl(mib, 2, &numCPU, &len, NULL, 0)){
	perror("sysctl");
    }

    if( numCPU <= 1 ) {
	mib[1] = HW_NCPU;
	sysctl( mib, 2, &numCPU, &len, NULL, 0 );
	if( numCPU < 1 ) {
	    numCPU = 1;
	}
    }
#endif
#ifdef _SC_NPROCESSORS_ONLN
    numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    return numCPU;
}

#ifdef WIN32
/**
 * From the pthreads readme for mingw:
 * Define PTW32_STATIC_LIB when building your application. Also, your
 * application must call a two non-portable routines to initialise the
 * some state on startup and cleanup before exit. One other routine needs
 * to be called to cleanup after any Win32 threads have called POSIX API
 * routines. See README.NONPORTABLE or the html reference manual pages for
 * details on these routines:
 * 
 * BOOL pthread_win32_process_attach_np (void);
 * BOOL pthread_win32_process_detach_np (void);
 * BOOL pthread_win32_thread_attach_np (void); // Currently a no-op
 * BOOL pthread_win32_thread_detach_np (void);
 *
 */
void threadpool::win32_init()
{
    pthread_win32_process_attach_np();
    pthread_win32_thread_attach_np();
}
#endif

/**
 * Create the thread pool.
 * Each thread has its own feature_recorder_set.
 *
 */
threadpool::threadpool(int numthreads,feature_recorder_set &fs_,xml &xreport_):
    workers(),M(),TOMAIN(),TOWORKER(),freethreads(numthreads),work_queue(),
    fs(fs_),xreport(xreport_),thread_status(),waiting(),mode()
{
    if(pthread_mutex_init(&M,NULL)) errx(1,"pthread_mutex_init failed");
    if(pthread_cond_init(&TOMAIN,NULL)) errx(1,"pthread_cond_init #1 failed");
    if(pthread_cond_init(&TOWORKER,NULL)) errx(1,"pthread_cond_init #2 failed");

    // lock while I create the threads
    if(pthread_mutex_lock(&M)) errx(1,"pthread_mutex_lock failed");
    for(int i=0;i<numthreads;i++){
	class worker *w = new worker(*this,i);
	workers.push_back(w);
	thread_status.push_back(string());
	pthread_create(&w->thread,NULL,worker::start_worker,(void *)w);
    }
    pthread_mutex_unlock(&M);		// lock while I create the threads
}

threadpool::~threadpool()
{
    /* We previously sent the termination message to all of the sub-threads here.
     * However, their terminating caused wacky problems with the malloc library.
     * So we just leave them floating around now. Doesn't matter much, because
     * the main process will die soon enough.
     */
#if 0
    for(int i=0;i<num_threads;i++){
	this->schedule_work(0);
    }
#endif

    /* Release our resources */
    fs.close_all();
    pthread_mutex_destroy(&M);
    pthread_cond_destroy(&TOMAIN);
    pthread_cond_destroy(&TOWORKER);

#ifdef WIN32
    pthread_win32_process_detach_np();
    pthread_win32_thread_detach_np();
#endif
}

/** 
 * work is delivered in sbufs.
 * This blocks the caller if there are no free workers.
 */
void threadpool::schedule_work(sbuf_t *sbuf)
{
    pthread_mutex_lock(&M);
    while(freethreads==0){
	// wait until a thread is free (doesn't matter which)
	waiting.start();
	if(pthread_cond_wait(&TOMAIN,&M)){
	    err(1,"threadpool::schedule_work pthread_cond_wait failed");
	}
	waiting.stop();
    }
    work_queue.push(sbuf); 
    freethreads--;
    pthread_cond_signal(&TOWORKER);
    pthread_mutex_unlock(&M);
}

int threadpool::get_free_count()
{
    pthread_mutex_lock(&M);
    int ret = freethreads;
    pthread_mutex_unlock(&M);
    return ret;
}

/**
 * do the work. Record that the work was started and stopped in XML file.
 */
bool opt_work_start_work_end=true;
void worker::do_work(sbuf_t *sbuf)
{
    aftimer t;

    if(opt_work_start_work_end){
	std::stringstream ss;
	ss << "threadid='" << id << "'"
	   << " pos0='" << sbuf->pos0.str() << "'"
	   << " pagesize='" << sbuf->pagesize << "'"
	   << " bufsize='" << sbuf->bufsize << "'";
	master.xreport.xmlout("debug:work_start","",ss.str(),true);
    }
    t.start();
	
    /**
     * HERE IT IS!!!
     * Construct a scanner_params() object from the sbuf that was pulled
     * off the work queue and call process_extract().
     */

    process_sbuf(scanner_params(scanner_params::scan,*sbuf,master.fs)); 
    t.stop();
    if(opt_work_start_work_end){
	std::stringstream ss;
	ss << "threadid='" << id << "'"
	   << " pos0='" << sbuf->pos0.str() << "'"
	   << " time='" << t.elapsed_seconds() << "'";
	master.xreport.xmlout("debug:work_end","",ss.str(),true);
    }
    master.fs.flush_all();
}


void threadpool::set_thread_status(uint32_t id,const std::string &status)
{
    if(pthread_mutex_lock(&M)){
	errx(1,"threadpool::set_thread_status pthread_mutex_lock failed");
    }
    if(id < thread_status.size()){
	thread_status.at(id) = status;
    }
    pthread_mutex_unlock(&M);
}

std::string threadpool::get_thread_status(uint32_t id)
{
    if(pthread_mutex_lock(&M)){
	errx(1,"threadpool::set_thread_status pthread_mutex_lock failed");
    }
    std::string status;
    if(id < thread_status.size()){
	status = thread_status.at(id);
    }
    pthread_mutex_unlock(&M);
    return status;
}

/* Run the worker.
 * Note that some versions of GCC thinks that this should have a noreturn
 * attribute, but then when the attribute is given, GCC complains that it has
 * a return statement!
 */
void *worker::run() 
{
    while(true){
	/* Get the lock, then wait for the queue to be empty.
	 * If it is not empty, wait for the lock again.
	 */
	if(master.mode==0) waiting.start(); // only if we are not waiting for workers to finish
	if(pthread_mutex_lock(&master.M)){
            std::cerr << "worker::run: pthread_mutex_lock failed";
            throw new internal_error();
	}
	/* At this point the worker has the lock */
	while(master.work_queue.empty()){
	    /* I didn't get any work; go back to sleep */
	    if(pthread_cond_wait(&master.TOWORKER,&master.M)){
                std::cerr << "pthread_cond_wait error=%d" << errno << "\n";
                throw new internal_error();
	    }
	}
	waiting.stop();
	/* Worker still has the lock */
	sbuf_t *sbuf = master.work_queue.front(); // get the sbuf
	master.work_queue.pop();		   // pop from the list
	master.thread_status.at(id) = std::string("Processing ") + sbuf->pos0.str();

	/* release the lock */
	pthread_mutex_unlock(&master.M);	   // unlock
	if(sbuf==0) {
            return 0;
	}
	do_work(sbuf);
	delete sbuf;
	pthread_mutex_lock(&master.M);
	master.freethreads++;
	master.thread_status.at(id) = string("Free");
	pthread_cond_signal(&master.TOMAIN); // tell the master that we are free!
	pthread_mutex_unlock(&master.M);     // should wake up the master
    }
}

