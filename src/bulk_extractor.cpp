/*
 * bulk_extractor.cpp:
 * Feature Extraction Framework...
 *
 */

#include "bulk_extractor.h"
#include "aftimer.h"
#include "image_process.h"
#include "threadpool.h"
#include "xml.h"

#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <set>
#include <setjmp.h>
#include <vector>
#include <queue>
#include <unistd.h>
#include <ctype.h>

#ifdef HAVE_MCHECK
#include <mcheck.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

using namespace std;

/****************************************************************
 *** COMMAND LINE OPTIONS
 ****************************************************************/

const char *progname=0;

size_t opt_pagesize=1024*1024*16;	// 
size_t opt_margin = 1024*1024*4;
u_int opt_notify_rate = 4;		// by default, notify every 4 pages
int word_min = 6;
int word_max = 14;
int64_t opt_offset_start = 0;
int64_t opt_offset_end   = 0;
int   min_uncompr_size = 6;	// don't bother with objects smaller than this
int   max_uncompr_size = 256*1024*1024; // don't decompress objects larger than this
int   debug=0;
int   opt_silent= 0;
int   max_bad_alloc_errors = 60;
uint64_t opt_page_start = 0;
uint32_t   opt_last_year = 2020;
const char *image_fname = 0;
const time_t max_wait_time=3600;

std::string HTTP_EOL = "\r\n";		// stdout is in binary form

/* global alert_list and stop_list
 * These should probably become static class variables
 */
word_and_context_list alert_list;		/* shold be flagged */
word_and_context_list stop_list;		/* should be ignored */



/* Obtain subversion keywords
 * http://svnbook.red-bean.com/en/1.4/svn.advanced.props.special.keywords.html
 * http://stackoverflow.com/questions/1449935/getting-svn-revision-number-into-a-program-automatically
 * remember to set the property 'keywords=Revision' on bulk_extractor.cpp
 */
std::string svn_date("$Date: 2012-11-29 17:00:33 -0500 (Thu, 29 Nov 2012) $");
std::string svn_revision("$Rev: 10886 $");
std::string svn_author("$Author: jon@lightboxtechnologies.com $");
std::string svn_headurl("$HeadURL: https://domex.nps.edu/domex/svn/src/bulk_extractor/trunk/src/bulk_extractor.cpp $");
std::string svn_id("$Id: bulk_extractor.cpp 10886 2012-11-29 22:00:33Z jon@lightboxtechnologies.com $");
std::string svn_revision_clean()
{
    string svn_r;
    for(string::const_iterator it = svn_revision.begin();it!=svn_revision.end();it++){
	if(*it>='0' && *it<='9') svn_r.push_back(*it);
    }
    return svn_r;
}


/**
 * Singletons:
 * histograms_t histograms - the set of histograms to make
 * feature_recorder_set fs - the collection of feature recorders.
 * xml xreport             - the DFXML output.
 * image_process p         - the image being processed.
 * 
 * Note that all of the singletons are passed to the phase1() function.
 */

#ifdef WIN32 
// Allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more 
int _CRT_fmode = _O_BINARY;
#endif

/************************
 *** SCANNER PLUG-INS ***
 ************************/

/* scanner_def is the class that is used internally to track all of the plug-in scanners.
 * Some scanners are compiled-in; others can be loaded at run-time.
 * Scanners can have multiple feature recorders. Multiple scanners can record to the
 * same feature recorder.
 */

/* An array of the built-in scanners */
scanner_t *scanners_builtin[] = {
    scan_accts,
    scan_base16,
    scan_base64,
    scan_kml,
    scan_email,
    //    scan_httpheader,
    scan_gps,
    scan_net,
    scan_find,
    scan_wordlist,
    scan_aes,
    scan_json,
#ifdef HAVE_LIBLIGHTGREP
    scan_lightgrep,
#endif
    //scan_lift,  // not ready for prime time
    //scan_extx,  // not ready for prime time
#ifdef HAVE_EXIV2
    scan_exiv2,
#endif
    scan_elf,
    scan_exif,
    scan_zip,
    scan_rar,
    scan_gzip,
    scan_pdf,
    scan_winpe,
    scan_hiberfile,
    scan_winprefetch,
    scan_windirs,
    scan_vcard,
    scan_bulk,
    scan_xor,
    0};

/***************************************************************************************
 *** PATH PRINTER - Used by bulk_extractor for printing pages associated with a path ***
 ***************************************************************************************/

/* Get the next token from the path. Tokens are separated by dashes.*/
static string get_and_remove_token(string &path)
{
    while(path[0]=='-'){
	path = path.substr(1); // remove any leading dashes.
    }
    size_t dash = path.find('-');	// find next dash
    if(dash==string::npos){		// no string; return the rest
	string prefix = path;
	path = "";
	return prefix;
    }
    string prefix = path.substr(0,dash);
    path = path.substr(dash+1);
    return prefix;
}

/**
 * upperstr - Turns an ASCII string into upper case (should be UTF-8)
 */

std::string upperstr(const std::string &str)
{
    std::string ret;
    for(std::string::const_iterator i=str.begin();i!=str.end();i++){
	ret.push_back(toupper(*i));
    }
    return ret;
}

void process_path_printer(const scanner_params &sp)
{
    /* 1. Get next token 
     * 2. if prefix part is a number, skip forward that much in sbuf and repeat.
     *    if the prefix is PRINT, print the buffer
     *    if next part is a string, strip it and run that decoder.
     *    if next part is a |, print
     */

    if(debug & DEBUG_PRINT_STEPS) cerr << "process_path_printer " << sp.sbuf.pos0.path << "\n";
    string new_path = sp.sbuf.pos0.path;
    string prefix = get_and_remove_token(new_path);

    /* Time to print ?*/
    if(prefix.size()==0 || prefix=="PRINT"){

	uint64_t print_start = 0;
	uint64_t print_len = 4096;
    
	/* Check for options */
	scanner_params::PrintOptions::iterator it;

	it = sp.print_options.find("Content-Length");
	if(it!=sp.print_options.end()){
	    print_len = stoi64(it->second);
	}

	it = sp.print_options.find("Range");
	if(it!=sp.print_options.end()){
	    if(it->second[5]=='='){
		size_t dash = it->second.find('-');
		string v1 = it->second.substr(6,dash-6);
		string v2 = it->second.substr(dash+1);
		print_start = stoi64(v1);
		print_len = stoi64(v2)-print_start+1;
	    }
	}

	if(print_start>sp.sbuf.bufsize){
	    print_len = 0;			// can't print anything
	}

	if(print_len>0 && print_start+print_len>sp.sbuf.bufsize){
	    print_len = sp.sbuf.bufsize-print_start;
	}

	switch(scanner_params::getPrintMode(sp.print_options)){
	case scanner_params::MODE_HTTP:
	    std::cout << "Content-Length: "		<< print_len  << HTTP_EOL;
	    std::cout << "Content-Range: bytes "	<< print_start << "-" << print_start+print_len-1 << HTTP_EOL;
	    std::cout << "X-Range-Available: bytes " << 0 << "-" << sp.sbuf.bufsize-1 << HTTP_EOL;
	    std::cout << HTTP_EOL;
	    sp.sbuf.raw_dump(std::cout,print_start,print_len); // send to stdout as binary
	    break;
	case scanner_params::MODE_RAW:
	    std::cout << print_len << HTTP_EOL;
	    std::cout.flush();
	    sp.sbuf.raw_dump(std::cout,print_start,print_len); // send to stdout as binary
	    break;
	case scanner_params::MODE_HEX:
	    sp.sbuf.hex_dump(std::cout,print_start,print_len);
	    break;
	case scanner_params::MODE_NONE:
	    break;
	}
	return;			// our job is done
    }
    /* If we are in an offset block, process recursively with the offset */
    if(isdigit(prefix[0])){
	uint64_t offset = stoi64(prefix);
	if(offset>sp.sbuf.bufsize){
	    printf("Error: %s only has %u bytes; can't offset to %u\n",
		   new_path.c_str(),(unsigned int)sp.sbuf.bufsize,(unsigned int)offset);
	    return;
	}
	process_path_printer(scanner_params(scanner_params::PHASE_SCAN,
					    sbuf_t(new_path,sp.sbuf+offset),
					    sp.fs,sp.print_options));
	return;
    }
    /* Find the scanner and use it */
    for(scanner_vector::const_iterator it = current_scanners.begin();it!=current_scanners.end();it++){
	const string &name = upperstr((*it)->info.name);
	if(name==prefix){
	    ((*it)->scanner)(scanner_params(scanner_params::PHASE_SCAN,
					    sbuf_t(new_path,sp.sbuf),
					    sp.fs,sp.print_options),
			     recursion_control_block(process_path_printer,name,true));
	    return;
	}
    }
    cerr << "Unknown name in path: " << prefix << "\n";
}

/**
 * process_path uses the scanners to decode the path for the purpose of
 * decoding the image data and extracting the information.
 */


static void process_path(const image_process &p,string path,scanner_params::PrintOptions &po)
{
    /* Check for "/r" in path which means print raw */
    if(path.size()>2 && path.substr(path.size()-2,2)=="/r"){
	path = path.substr(0,path.size()-2);
    }

    string prefix = get_and_remove_token(path);
    int64_t offset = stoi64(prefix);

    /* Get the offset. process */
    u_char *buf = (u_char *)calloc(opt_pagesize,1);
    if(!buf) errx(1,"Cannot allocate buffer");
    int count = p.pread(buf,opt_pagesize,offset);
    if(count<0){
	cerr << p.image_fname() << ": " << strerror(errno) << " (Read Error)\n";
	return;
    }

    /* make up a bogus feature recorder set and feature recorder */
    feature_recorder_set fs(feature_recorder_set::DISABLED);

    pos0_t pos0(path+"-PRINT"); // insert the PRINT token
    sbuf_t sbuf(pos0,buf,count,count,true); // sbuf system will free
    scanner_params sp(scanner_params::PHASE_SCAN,sbuf,fs,po);
    process_path_printer(sp);
}

/**
 * process a path for a given filename.
 * Opens the image and calls the function above.
 * Also implements HTTP server with "-http" option.
 */
static void process_path(const char *fn,string path)
{
    image_process *pp = image_process_open(fn,0);
    if(pp==0){
	if(path=="-http"){
	    std::cout << "HTTP/1.1 502 Filename " << fn << " is invalid" << HTTP_EOL << HTTP_EOL;
	} else {
	    cerr << "Filename " << fn << " is invalid\n";
	}
	exit(1);
    }

    if(path=="-"){
	/* process path interactively */
	printf("Path Interactive Mode:\n");
	if(pp==0){
	    cerr << "Invalid file name: " << fn << "\n";
	    exit(1);
	}
	do {
	    getline(cin,path);
	    if(path==".") break;
	    scanner_params::PrintOptions po;
	    scanner_params::setPrintMode(po,scanner_params::MODE_HEX);
	    process_path(*pp,path,po);
	} while(true);
	return;
    }
    if(path=="-http"){
	do {
	    /* get the HTTP query */
	    string line;		// the specific query
	    scanner_params::PrintOptions po;
	    scanner_params::setPrintMode(po,scanner_params::MODE_HTTP);	// options for this query
	    
	    getline(cin,line);
	    truncate_at(line,'\r');
	    if(line.substr(0,4)!="GET "){
		std::cout << "HTTP/1.1 501 Method not implemented" << HTTP_EOL << HTTP_EOL;
		return;
	    }
	    size_t space = line.find(" HTTP/1.1");
	    if(space==string::npos){
		std::cout << "HTTP/1.1 501 Only HTTP/1.1 is implemented" << HTTP_EOL << HTTP_EOL;
		return;
	    }
	    string p2 = line.substr(4,space-4);

	    /* Get the additional header options */
	    do {
		getline(cin,line);
		truncate_at(line,'\r');
		if(line.size()==0) break; // double new-line
		size_t colon = line.find(":");
		if(colon==string::npos){
		    std::cout << "HTTP/1.1 502 Malformed HTTP request" << HTTP_EOL;
		    return;
		}
		string name = line.substr(0,colon);
		string val  = line.substr(colon+1);
		while(val.size()>0 && (val[0]==' '||val[0]=='\t')) val = val.substr(1);
		po[name]=val;
	    } while(true);
	    /* Process some specific URLs */
	    if(p2=="/info"){
		std::cout << "X-Image-Size: " << pp->image_size() << HTTP_EOL;
		std::cout << "X-Image-Filename: " << pp->image_fname() << HTTP_EOL;
		std::cout << "Content-Length: 0" << HTTP_EOL;
		std::cout << HTTP_EOL;
		continue;
	    }

	    /* Ready to go with path and options */
	    process_path(*pp,p2,po);
	} while(true);
	return;
    }
    scanner_params::PrintOptions po;
    scanner_params::print_mode_t mode = scanner_params::MODE_HEX;
    if(path.size()>2 && path.substr(path.size()-2,2)=="/r"){
	path = path.substr(0,path.size()-2);
	mode = scanner_params::MODE_RAW;
    }
    if(path.size()>2 && path.substr(path.size()-2,2)=="/h"){
	path = path.substr(0,path.size()-2);
	mode = scanner_params::MODE_HEX;
    }
    scanner_params::setPrintMode(po,mode);
    process_path(*pp,path,po);
}

/****************************************************************
 *** Phase 1 BUFFER PROCESSING
 *** For every page of the iterator, schedule work.
 ****************************************************************/

class BulkExtractor_Phase1 {
    /** Sleep for a minimum of msec */
    static void msleep(int msec) {
#if _WIN32
	Sleep(msec);
#else
#  ifdef HAVE_USLEEP
	usleep(msec*1000);
#  else
	int sec = msec/1000;
	if(sec<1) sec=1;
	sleep(sec);			// posix
#  endif
#endif
    }

    std::string minsec(time_t tsec) {
        time_t min = tsec / 60;
        time_t sec = tsec % 60;
        std::stringstream ss;
        if(min>0) ss << min << " min ";
        if(sec>0) ss << sec << " sec";
        return ss.str();
	
    }
    void print_tp_status(threadpool &tp) {
        std::stringstream ss;
        for(u_int i=0;i<num_threads;i++){
            std::string status = tp.get_thread_status(i);
            if(status.size() && status!="Free"){
                ss << "Thread " << i << ": " << status << "\n";
            }
        }
        std::cout << ss.str() << "\n";
    }

    /* Instance variables */
    xml &xreport;
    aftimer &timer;
    u_int num_threads;
    int opt_quiet;
    static const int retry_seconds = 60;

    /* for random sampling */
    double sampling_fraction;
    u_int  sampling_passes;
    u_int   notify_ctr;

    bool sampling(){return sampling_fraction<1.0;}

    /* Get the sbuf from current image iterator location, with retries */
    sbuf_t *get_sbuf(image_process::iterator &it) {
	for(int retry_count=0;retry_count<max_bad_alloc_errors;retry_count++){
	    try {
		return it.sbuf_alloc(); // may throw exception
	    }
	    catch (const std::bad_alloc &e) {
		// Low memory could come from a bad sbuf alloc or another low memory condition.
		// wait for a while and then try again...
		std::cerr << "Low Memory (bad_alloc) exception: " << e.what()
			  << " reading " << it.get_pos0()
			  << " (retry_count=" << retry_count
			  << " of " << max_bad_alloc_errors << ")\n";
		
		std::stringstream ss;
		ss << "name='bad_alloc' " << "pos0='" << it.get_pos0() << "' "
		   << "retry_count='"     << retry_count << "' ";
		xreport.xmlout("debug:exception", e.what(), ss.str(), true);
	    }
	    if(retry_count<max_bad_alloc_errors+1){
		std::cerr << "will wait for " << retry_seconds << " seconds and try again...\n";
		msleep(retry_seconds*1000);
	    }
	}
	std::cerr << "Too many errors encountered in a row. Diagnose and restart.\n";
	exit(1);
    }

    /* Notify user about current state of phase1 */
    void notify_user(image_process::iterator &it){
	if(notify_ctr++ >= opt_notify_rate){
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
    void set_sampling_parameters(const std::string &p){
	std::vector<std::string> params = split(p,':');
	if(params.size()!=1 && params.size()!=2){
	    errx(1,"error: sampling parameters must be fraction[:passes]");
	}
	sampling_fraction = atof(params.at(0).c_str());
	if(sampling_fraction<=0 || sampling_fraction>=1){
	    errx(1,"error: sampling fraction f must be 0<f<=1; you provided '%s'",params.at(0).c_str());
	}
	if(params.size()==2){
	    sampling_passes = atoi(params.at(1).c_str());
	    if(sampling_passes==0){
		errx(1,"error: sampling passes must be >=1; you provided '%s'",params.at(1).c_str());
	    }
	}
    }

    BulkExtractor_Phase1(xml &xreport_,aftimer &timer_,u_int num_threads_,int opt_quiet_):
        xreport(xreport_),timer(timer_),
	num_threads(num_threads_),opt_quiet(opt_quiet_),
	sampling_fraction(1),sampling_passes(1),notify_ctr(0){ }

    void run(image_process &p,feature_recorder_set &fs,
             int64_t &total_bytes, xml::tagid_set_t &seen_page_ids) {

        md5_generator *md5g = new md5_generator();		// keep track of MD5
        uint64_t md5_next = 0;					// next byte to hash
        threadpool tp(num_threads,fs,xreport);			// 
	uint64_t page_ctr=0;
        xreport.push("runtime","xmlns:debug=\"http://www.afflib.org/bulk_extractor/debug\"");

	std::set<uint64_t> blocks_to_sample;
	std::set<uint64_t>::const_iterator si = blocks_to_sample.begin(); // sampling iterator
	bool first = true;              
	for(image_process::iterator it = p.begin();it!=p.end();){
	    if(sampling()){
		if(first) {
		    first = false;
		    /* Create a list of blocks to sample */
		    uint64_t blocks = it.blocks();
		    while(blocks_to_sample.size() < blocks * sampling_fraction){
			uint64_t blk = ((random()<<32) | (random())) % blocks;
			blocks_to_sample.insert(blk); // will be added even if already present
		    }

		    //for(si = blocks_to_sample.begin();si!=blocks_to_sample.end();++si){
                    //std::cout << *si << " ";
                    //}
		    //std::cout << "\n";
		    si = blocks_to_sample.begin();
		    it.seek_block(*si);
		}
	    }
            
            if(opt_offset_end!=0 && opt_offset_end <= it.raw_offset ){
                break; // passed the offset
            }
            if(opt_page_start<=page_ctr && opt_offset_start<=it.raw_offset){
                if(seen_page_ids.find(it.get_pos0().str()) != seen_page_ids.end()){
                    // this page is in the XML file. We've seen it, so skip it (restart code)
                    goto loop;
                }
                
                // attempt to get an sbuf. If we can't get it, we may be in a low-memory situation.
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

                    tp.schedule_work(sbuf);	// schedule the work
		    if(!opt_quiet) notify_user(it);
                }
                catch (const std::exception &e) {
                    // report uncaught exceptions to both user and XML file
                    std::stringstream ss;
                    ss << "name='" << e.what() << "' " << "pos0='" << it.get_pos0() << "' ";
		    std::cerr << "Exception " << e.what() << " skipping " << it.get_pos0() << "\n";
                    xreport.xmlout("debug:exception", e.what(), ss.str(), true);
                }
            }
        loop:;
            /* If we are random sampling, move to the next random sample.
             * Otherwise increment the iterator
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
	    
        if(!opt_quiet){
            std::cout << "All Data is Read; waiting for threads to finish...\n";
        }

        /* Now wait for all of the threads to be free */
        tp.mode = 1;			// waiting for workers to finish
        time_t wait_start = time(0);
        for(int32_t counter = 0;;counter++){
            int num_remaining = num_threads - tp.get_free_count();
            if(num_remaining==0) break;

            msleep(100);
            time_t time_waiting   = time(0) - wait_start;
            time_t time_remaining = max_wait_time - time_waiting;

            if(counter%60==0){
                std::stringstream ss;
                ss << "Time elapsed waiting for " << num_remaining
                   << " thread" << (num_remaining>1 ? "s" : "") 
                   << " to finish:\n    " << minsec(time_waiting) 
                   << " (timeout in "     << minsec(time_remaining) << ".)\n";
                if(opt_quiet==0){
                    std::cout << ss.str();
                    if(counter>0) print_tp_status(tp);
                }
                xreport.comment(ss.str());
            }
            if(time_waiting>max_wait_time){
                std::cout << "\n\n";
                std::cout << " ... this shouldn't take more than an hour. Exiting ... \n";
                std::cout << " ... Please report to the bulk_extractor maintainer ... \n";
                break;
            }
        }
        if(opt_quiet==0) std::cout << "All Threads Finished!\n";
	
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
        for(feature_recorder_map::const_iterator it = tp.fs.frm.begin();
            it != tp.fs.frm.end(); it++){
            xreport.set_oneline(true);
            xreport.push("feature_file");
            xreport.xmlout("name",it->second->name);
            xreport.xmlout("count",it->second->count);
            xreport.pop();
            xreport.set_oneline(false);
        }
        std::cout << "Producer time spent waiting: " << tp.waiting.elapsed_seconds() << " sec.\n";
    
        xreport.xmlout("thread_wait",dtos(tp.waiting.elapsed_seconds()),"thread='0'",false);
        double worker_wait_average = 0;
        for(threadpool::worker_vector::const_iterator it=tp.workers.begin();it!=tp.workers.end();it++){
            worker_wait_average += (*it)->waiting.elapsed_seconds() / num_threads;
            std::stringstream ss;
            ss << "thread='" << (*it)->id << "'";
            xreport.xmlout("thread_wait",dtos((*it)->waiting.elapsed_seconds()),ss.str(),false);
        }
        xreport.pop();
        xreport.flush();
        std::cout << "Average consumer time spent waiting: " << worker_wait_average << " sec.\n";
        if(worker_wait_average > tp.waiting.elapsed_seconds()*2 && worker_wait_average>10){
            std::cout << "*******************************************\n";
            std::cout << "** bulk_extractor is probably I/O bound. **\n";
            std::cout << "**        Run with a faster drive        **\n";
            std::cout << "**      to get better performance.       **\n";
            std::cout << "*******************************************\n";
        }
        if(tp.waiting.elapsed_seconds() > worker_wait_average * 2 && tp.waiting.elapsed_seconds()>10){
            std::cout << "*******************************************\n";
            std::cout << "** bulk_extractor is probably CPU bound. **\n";
            std::cout << "**    Run on a computer with more cores  **\n";
            std::cout << "**      to get better performance.       **\n";
            std::cout << "*******************************************\n";
        }
        /* end of phase 1 */
    }
};

/****************************************************************
 *** Usage
 ****************************************************************/

static void usage() __attribute__ ((__noreturn__));
static void usage()
{
    std::cout << "bulk_extractor version " PACKAGE_VERSION " " << svn_revision << "\n";
    std::cout << "Usage: " << progname << " [options] imagefile\n";
    std::cout << "  runs bulk extractor and outputs to stdout a summary of what was found where\n";
    std::cout << "\n";
    std::cout << "Required parameters:\n";
    std::cout << "   imagefile     - the file to extract\n";
    std::cout << " or  -R filedir  - recurse through a directory of files\n";
#ifdef HAVE_LIBEWF
    std::cout << "                  HAS SUPPORT FOR E01 FILES\n";
#endif
#ifdef HAVE_LIBAFFLIB
    std::cout << "                  HAS SUPPORT FOR AFF FILES\n";
#endif    
#ifdef HAVE_EXIV2
    std::cout << "                  EXIV2 ENABLED\n";
#endif    
    std::cout << "   -o outdir    - specifies output directory. Must not exist.\n";
    std::cout << "                  bulk_extractor creates this directory.\n";
    std::cout << "Options:\n";
    std::cout << "   -b banner.txt- Add banner.txt contents to the top of every output file.\n";
    std::cout << "   -r alert_list.txt  - a file containing the alert list of features to alert\n";
    std::cout << "                       (can be a feature file or a list of globs)\n";
    std::cout << "                       (can be repeated.)\n";
    std::cout << "   -w stop_list.txt   - a file containing the stop list of features (white list\n";
    std::cout << "                       (can be a feature file or a list of globs)s\n";
    std::cout << "                       (can be repeated.)\n";
    std::cout << "   -F <rfile>   - Read a list of regular expressions from <rfile> to find\n";
    std::cout << "   -f <regex>   - find occurrences of <regex>; may be repeated.\n";
    std::cout << "                  results go into find.txt\n";
    std::cout << "   -q nn        - Quiet Rate; only print every nn status reports. Default 0; -1 for no status at all\n";
    std::cout << "   -S frac[:passes] - Set random sampling parameters\n";
    std::cout << "\nTuning parameters:\n";
    std::cout << "   -C NN        - specifies the size of the context window (default " << feature_recorder::context_window << ")\n";
    std::cout << "   -G NN        - specify the page size (default " << opt_pagesize << ")\n";
    std::cout << "   -g NN        - specify margin (default " <<opt_margin << ")\n";
    std::cout << "   -W n1:n2     - Specifies minimum and maximum word size\n";
    std::cout << "                 (default is -w" << word_min << ":" << word_max << ")\n";
    std::cout << "   -B NN        - Specify the blocksize for bulk data analysis (default " <<opt_scan_bulk_block_size<< ")\n";
    std::cout << "   -j NN        - Number of analysis threads to run (default " <<threadpool::numCPU() << ")\n";
    std::cout << "   -M nn        - sets max recursion depth (default " << scanner_def::max_depth << ")\n";
    std::cout << "\nPath Processing Mode:\n";
    std::cout << "   -p <path>/f  - print the value of <path> with a given format.\n";
    std::cout << "                  formats: r = raw; h = hex.\n";
    std::cout << "                  Specify -p - for interactive mode.\n";
    std::cout << "                  Specify -p -http for HTTP mode.\n";
    std::cout << "\nParallelizing:\n";
    std::cout << "   -Y <o1>      - Start processing at o1 (o1 may be 1, 1K, 1M or 1G)\n";
    std::cout << "   -Y <o1>-<o2> - Process o1-o2\n";
    std::cout << "   -A <off>     - Add <off> to all reported feature offsets\n";
    std::cout << "\nDebugging:\n";
    std::cout << "   -h           - print this message\n";
    std::cout << "   -H           - print detailed info on the scanners\n";
    std::cout << "   -V           - print version number\n";
    std::cout << "   -z nn        - start on page nn\n";
    std::cout << "   -dN          - debug mode (see source code\n";
    std::cout << "   -Z           - zap (erase) output directory\n";
    std::cout << "\nControl of Scanners:\n";
    std::cout << "   -P <dir>     - Specifies a plugin directory\n";
    std::cout << "   -E scanner   - turn off all scanners except scanner\n";
    std::cout << "   -m <max>     - maximum number of minutes to wait for memory starvation\n";
    std::cout << "                  default is " << max_bad_alloc_errors << "\n";
    std::cout << "   -s name=value - sets a bulk extractor option name to be value\n";
    info_scanners(false,scanners_builtin,'e','x');
    std::cout << "\n";
    exit(0);
}


/*
 * Make sure that the filename provided is sane.
 * That is, do not allow the analysis of a *.E02 file...
 */
void validate_fn(const string &fn)
{
    int r= access(fn.c_str(),R_OK);
    if(r!=0){
	cerr << "cannot open: " << fn << ": " << strerror(errno) << " (code " << r << ")\n";
	exit(1);
    }
    if(fn.size()>3){
	size_t e = fn.rfind('.');
	if(e!=string::npos){
	    string ext = fn.substr(e+1);
	    if(ext=="E02" || ext=="e02"){
		cerr << "Error: invalid file name\n";
		cerr << "Do not use bulk_extractor to process individual EnCase files.\n";
		cerr << "Instead, just run bulk_extractor with FILENAME.E01\n";
		cerr << "The other files in an EnCase multi-volume archive will be opened\n";
		cerr << "automatically.\n";
		exit(1);
	    }
	}
    }
}

/**
 * Create the dfxml output
 */

static void dfxml_create(xml &xreport,const string &command_line,int num_threads)
{
    xreport.push("dfxml","xmloutputversion='1.0'");
    xreport.push("metadata",
		 "\n  xmlns='http://afflib.org/bulk_extractor/' "
		 "\n  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
		 "\n  xmlns:dc='http://purl.org/dc/elements/1.1/'" );
    xreport.xmlout("dc:type","Feature Extraction","",false);
    xreport.pop();
    xreport.add_DFXML_creator(PACKAGE_NAME,PACKAGE_VERSION,svn_revision_clean(),command_line);
    xreport.push("configuration");
    xreport.xmlout("threads",num_threads);
    xreport.push("scanners");
    /* Generate a list of the scanners in use */

    for(scanner_vector::const_iterator it=current_scanners.begin();it!=current_scanners.end();it++){
	if((*it)->enabled){
	    xreport.xmlout("scanner",(*it)->info.name);
	}
    }
    xreport.pop();			// scanners
    xreport.pop();			// configuration
}

static bool directory_missing(const std::string &d)
{
    return access(d.c_str(),F_OK)<0;
}

static bool directory_empty(const std::string &d)
{
    if(directory_missing(d)==false){
	std::string reportfn = d + "/report.xml";
	if(access(reportfn.c_str(),R_OK)<0) return true;
    }
    return false;
}

/**
 * scaled_stoi:
 * Like a normal stoi, except it can handle modifies k, m, and g
 */
static uint64_t scaled_stoi(const std::string &str)
{
    std::stringstream ss(str);
    uint64_t val;
    ss >> val;
    if(str.find('k')!=string::npos  || str.find('K')!=string::npos) val *= 1024;
    if(str.find('m')!=string::npos  || str.find('m')!=string::npos) val *= 1024 * 1024;
    if(str.find('g')!=string::npos  || str.find('g')!=string::npos) val *= 1024 * 1024 * 1024;
    if(str.find('t')!=string::npos  || str.find('T')!=string::npos) val *= 1024 * 1024 * 1024 * 1024;
    return val;
}


int main(int argc,char **argv)
{
#ifdef HAVE_MCHECK
    mtrace();
#endif
    
    if(getenv("BULK_EXTRACTOR_DEBUG")){
	debug = atoi(getenv("BULK_EXTRACTOR_DEBUG"));
    }
    progname = argv[0];
    scanner_info::config_t be_config; // system configuration
    const char *opt_path = 0;
    int opt_recurse = 0;
    int opt_zap = 0;
    string opt_outdir;
    char *cc;
    setvbuf(stdout,0,_IONBF,0);		// don't buffer stdout
    std::string command_line = xml::make_command_line(argc,argv);
    u_int num_threads = threadpool::numCPU();
    int opt_quiet = 0;
    std::string opt_sampling_params;
    std::vector<std::string> scanner_dirs;

    /* Figure out the current time */
    time_t t = time(0);
    struct tm now;
    gmtime_r(&t,&now);
    opt_last_year = now.tm_year + 1900 + 5; // allow up to 5 years in the future
    


#ifdef WIN32
    setmode(1,O_BINARY);		// make stdout binary
    threadpool::win32_init();
#endif
    /* look for usage first */
    if(argc==1 || (strcmp(argv[1],"-h")==0)){
	usage();
	exit(1);
    }

    /* Process options */
    int ch;
    while ((ch = getopt(argc, argv, "A:B:b:C:d:E:e:F:f:G:g:Hhj:M:m:o:P:p:q:Rr:S:s:VW:w:x:Y:z:Z")) != -1) {
	switch (ch) {
	case 'A': feature_recorder::offset_add  = stoi64(optarg);break;
	case 'B': opt_scan_bulk_block_size = scaled_stoi(optarg);break;
	case 'b': feature_recorder::banner_file = optarg; break;
	case 'C': feature_recorder::context_window = atoi(optarg);break;
	case 'd':
	{
	    int d = atoi(optarg);
	    switch(d){
	    case DEBUG_ALLOCATE_512MiB: 
		if(calloc(1024*1024*512,1)){
		    cerr << "-d1002 -- Allocating 512MB of RAM; may be repeated\n";
		} else {
		    cerr << "-d1002 -- CANNOT ALLOCATE MORE RAM\n";
		}
		break;
	    default:
		debug  = d;
		break;
	    }
	}
	break;
	case 'E':
	    scanners_disable_all();
	    scanners_enable(optarg);
	    break;
	case 'e':
	    scanners_enable(optarg);
	    break;
	case 'F': process_find_file(optarg); break;
	case 'f': add_find_pattern(optarg); break;
	case 'G': opt_pagesize = scaled_stoi(optarg); break;
	case 'g': opt_margin = scaled_stoi(optarg); break;
	case 'j': num_threads = atoi(optarg); break;
	case 'M': scanner_def::max_depth = atoi(optarg); break;
	case 'm': max_bad_alloc_errors = atoi(optarg); break;
	case 'o': opt_outdir = optarg;break;
	case 'P': scanner_dirs.push_back(optarg);break;
	case 'p': opt_path = optarg; break;
        case 'q':
	    if(atoi(optarg)==-1) opt_quiet = 1;// -q -1 turns off notifications
	    else opt_notify_rate = atoi(optarg);
	    break;
	case 'r':
	    if(alert_list.readfile(optarg)){
		err(1,"Cannot read alert list %s",optarg);
	    }
	    break;
	case 'R': opt_recurse = 1; break;
	case 'S': opt_sampling_params = optarg; break;
	case 's':
	{
	    std::vector<std::string> params = split(optarg,'=');
	    if(params.size()!=2){
		std::cerr << "Invalid paramter: " << optarg << "\n";
		exit(1);
	    }
	    be_config[params[0]] = params[1];
	    continue;
	}
	case 'V': std::cout << "bulk_extractor " << PACKAGE_VERSION << "\n"; exit (1);
	case 'W':
	    cc = strchr(optarg,':');
	    if(!cc) err(1,"-W requires n1:n2");
	    cc[0] = 0;			// null terminate
	    word_min = atoi(optarg);
	    word_max = atoi(cc+1);
	    if(word_min>word_max) err(1,"word_min=%d word_max=%d\n",word_min,word_max);
	    break;
	case 'w': if(stop_list.readfile(optarg)){
		err(1,"Cannot read stop list %s",optarg);
	    }
	    break;
	case 'x':
	    scanners_disable(optarg);
	    break;
	case 'Y': {
	    string optargs = optarg;
	    size_t dash = optargs.find('-');
	    if(dash==string::npos){
		opt_offset_start = stoi64(optargs);
	    } else {
		opt_offset_start = scaled_stoi(optargs.substr(0,dash));
		opt_offset_end   = scaled_stoi(optargs.substr(dash+1));
	    }
	    break;
	}
	case 'z': opt_page_start = stoi64(optarg);break;
	case 'Z': opt_zap=true;break;
	case 'H':
	    info_scanners(true,scanners_builtin,'e','x');
	    exit(1);
	case 'h': case '?':default:
	    usage();
	    break;
	}
    }

    if(opt_offset_start % opt_pagesize != 0) errx(1,"ERROR: start offset must be a multiple of the page size\n");
    if(opt_offset_end % opt_pagesize != 0) errx(1,"ERROR: end offset must be a multiple of the page size\n");

    argc -= optind;
    argv += optind;


    if(be_config["work_start_work_end"]=="NO"){
	opt_work_start_work_end=false;
    }

    /* Load all the scanners and enable the ones we care about */
    load_scanner_directories(scanner_dirs,be_config);
    load_scanners(scanners_builtin,be_config); 
    scanners_process_commands();

    /* Give an error if a find list was specified
     * but no scanner that uses the find list is enabled.
     */

    if(find_list.size()>0){
        /* Look through the enabled scanners and make sure that
	 * at least one of them is a FIND scanner
	 */
        bool find_scanner_enabled = false;
        for(scanner_vector::const_iterator it = current_scanners.begin();
            it!=current_scanners.end() && find_scanner_enabled==false;
            it++){
            if( ((*it)->info.flags & scanner_info::SCANNER_FIND_SCANNER)
                && ((*it)->enabled)){
                find_scanner_enabled = true;
            }
        }
        
        if(find_scanner_enabled==false){
            errx(1,"find words are specified with -F but no find scanner is enabled.\n");
        }
    }

    if(opt_path){
	if(argc!=1) errx(1,"-p requires a single argument.");
	process_path(argv[0],opt_path);
	exit(0);
    }
    if(opt_outdir.size()==0) errx(1,"error: -o outdir must be specified");

    /* The zap option wipes the contents of a directory, useful for debugging */
    if(opt_zap){
	DIR *dirp = opendir(opt_outdir.c_str());
	if(dirp){
	    struct dirent *dp;
	    while ((dp = readdir(dirp)) != NULL){
		string name = dp->d_name;
		if(name=="." || name=="..") continue;
		string fname = opt_outdir + string("/") + name;
		unlink(fname.c_str());
		std::cout << "erasing " << fname << "\n";
	    }
	}
	if(rmdir(opt_outdir.c_str())){
            std::cout << "rmdir " << opt_outdir << "\n";
        }
    }

    /* Start the clock */
    aftimer timer;
    timer.start();

    /* If output directory does not exist, we are not restarting! */
    xml  *xreport=0;
    string reportfilename = opt_outdir + "/report.xml";

    xml::tagid_set_t seen_page_ids; // pages that do not need re-processing
    image_process *p = 0;
    image_fname = *argv;

    if(directory_missing(opt_outdir) || directory_empty(opt_outdir)){
	/* Create the new directory */
	if ( argc !=1 ) errx(1,"Disk image option not provided. Run with -h for help.");

	/* If disk image does not exist, we are in restart mode */
	validate_fn(image_fname);
	if(directory_missing(opt_outdir)) be_mkdir(opt_outdir);
	p = image_process_open(image_fname,opt_recurse);
	if(!p){
	    if(errno) err(1,"Cannot open %s: ",image_fname);
	}

	/* Store the configuration in the XML file */
	xreport = new xml(reportfilename,false);
	dfxml_create(*xreport,command_line,num_threads);
	
	xreport->xmlout("provided_filename",image_fname); // save this information
    } else {
	/* Restarting */
	if(access(reportfilename.c_str(),R_OK)){
	    cerr << opt_outdir << ": error\n";
	    cerr << "report.xml file is missing or unreadable.\n";
	    cerr << "Directory may not have been created by bulk_extractor.\n";
	    cerr << "Cannot continue.\n";
	    exit(1);
	}
	
	/* Specify the tags for which we are searching for */
	xml::tagmap_t tagmap;
	tagmap["provided_filename"] = "";
	string dws("debug:work_start");
	string attrib = "pos0";

	class xml::existing e;
	e.tagmap = &tagmap;
	e.tagid = &dws;
	e.attrib = &attrib;
	e.tagid_set = &seen_page_ids;

	xreport = new xml(reportfilename,e);

	if(tagmap["provided_filename"].size()==0){
	    cerr << "report.xml in output directory is incomplete.\n";
	    cerr << "cannot restart.\n";
	    exit(1);
	}
	
	if(tagmap["provided_filename"] != image_fname){
	    cerr << "report.xml in output directory is for " << tagmap["provided_filename"] << "\n";
	    cerr << "cannot restart with " << image_fname << "\n";
	    exit(1);
	}

	p = image_process_open(image_fname,opt_recurse);
	std::cout << "Restarting from " << opt_outdir << "\n";
    }

    if(p==0) err(1,"Cannot image_process_open(%s)",image_fname);
	    
    /* Determine the feature files that will be used */
    feature_file_names_t feature_file_names;
    enable_alert_recorder(feature_file_names);
    enable_feature_recorders(feature_file_names);
    feature_recorder_set fs(feature_file_names,image_fname,opt_outdir,stop_list.size()>0);

    /* Create the alert recorder */
    fs.create_name(feature_recorder_set::ALERT_RECORDER_NAME,false);
    feature_recorder_set::alert_recorder = fs.get_name(feature_recorder_set::ALERT_RECORDER_NAME);

    int64_t total_bytes = 0;

    /* provide documentation to the user; the DFXML information comes from elsewhere */
    if(!opt_quiet){
	std::cout << "bulk_extractor version: " << PACKAGE_VERSION << "\n";
#ifdef HAVE_GETHOSTNAME
	char hostname[1024];
	gethostname(hostname,sizeof(hostname));
	std::cout << "Hostname: " << hostname << "\n";
#endif
	std::cout << "Input file: " << image_fname << "\n";
	std::cout << "Output directory: " << opt_outdir << "\n";
	std::cout << "Disk Size: " << p->image_size() << "\n";
	std::cout << "Threads: " << num_threads << "\n";
    }

    /****************************************************************
     *** THIS IS IT! 
     ****************************************************************/

    BulkExtractor_Phase1 phase1(*xreport,timer,num_threads,opt_quiet);

    if(opt_sampling_params.size()>0) phase1.set_sampling_parameters(opt_sampling_params);

    phase1.run(*p,fs,total_bytes,seen_page_ids);

    if(opt_quiet==0) std::cout << "Phase 2. Shutting down scanners\n";
    phase_shutdown(fs,*xreport);

    if(opt_quiet==0) std::cout << "Phase 3. Creating Histograms\n";
    phase_histogram(fs,*xreport);

    /* report and then print final usage information */
    xreport->push("report");
    xreport->xmlout("total_bytes",total_bytes);
    xreport->xmlout("elapsed_seconds",timer.elapsed_seconds());
    xreport->pop();			// report
    xreport->flush();
    fs.dump_stats(*xreport);
    xreport->add_rusage();
    xreport->pop();			// bulk_extractor
    xreport->close();
    delete p;				// not strictly needed, but why not?
    if(!opt_quiet){
	float mb_per_sec = (total_bytes / 1000000.0) / timer.elapsed_seconds();

	std::cout.precision(4);
	std::cout << "Elapsed time: " << timer.elapsed_seconds() << " sec.\n";
	std::cout << "Overall performance: " << mb_per_sec << " MBytes/sec.\n";
        if (fs.has_name("email")) {
            feature_recorder *fr = fs.get_name("email");
            if(fr){
                std::cout << "Total " << fr->name << " features found: " << fr->count << "\n";
            }
        }
    }
#ifdef HAVE_MCHECK
    muntrace();
#endif
    exit(0);
}

