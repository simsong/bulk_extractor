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
    if(min>0) ss << min << " min ";
    if(sec>0) ss << sec << " sec";
    return ss.str();
}

void BulkExtractor_Phase1::print_tp_status(threadpool &tp)
{
    std::stringstream ss;
    for(u_int i=0;i<config.num_threads;i++){
        std::string status = tp.get_thread_status(i);
        if(status.size() && status!="Free"){
            ss << "Thread " << i << ": " << status << "\n";
        }
    }
    std::cout << ss.str() << "\n";
}

