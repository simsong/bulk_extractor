/*
 * bulk_extractor.cpp:
 * Feature Extraction Framework...
 *
 */

#include "bulk_extractor.h"
#include "image_process.h"
#include "threadpool.h"
#include "be13_api/aftimer.h"
#include "histogram.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"

#include "phase1.h"

#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <set>
#include <setjmp.h>
#include <vector>
#include <queue>
#include <unistd.h>
#include <ctype.h>

#ifdef HAVE_EXPAT_H
#include <expat.h>
#endif

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

// Global options that can be set without using the option system

uint32_t   debug=0;
int   opt_silent= 0;
uint32_t   opt_last_year = 2020;

std::string HTTP_EOL = "\r\n";		// stdout is in binary form

/* global alert_list and stop_list
 * These should probably become static class variables
 */
word_and_context_list alert_list;		/* shold be flagged */
word_and_context_list stop_list;		/* should be ignored */


static void truncate_at(string &line,char ch)
{
    size_t pos = line.find(ch);
    if(pos!=string::npos) line.erase(pos);
}


#if 0
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
#else
std::string svn_revision_clean()
{
    return std::string("");
}
#endif


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
#ifdef USE_LIFT
    scan_lift,  // not ready for prime time
#endif
    //scan_extx,  // not ready for prime time
#ifdef HAVE_EXIV2
    scan_exiv2,
#endif
#ifdef HAVE_HASHID
    scan_hashid,
#endif
    scan_elf,
    scan_exif,
    scan_zip,
#ifdef USE_RAR
    scan_rar,
#endif
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

static std::string lowerstr(const std::string &str)
{
    std::string ret;
    for(std::string::const_iterator i=str.begin();i!=str.end();i++){
	ret.push_back(tolower(*i));
    }
    return ret;
}

class path_printer_finished: public std::exception {
public:
    virtual const char *what() const throw() {
        return "path printer finished.";
    }
} printing_done;

void process_path_printer(const scanner_params &sp)
{
    /* 1. Get next token 
     * 2. if prefix part is a number, skip forward that much in sbuf and repeat.
     *    if the prefix is PRINT, print the buffer
     *    if next part is a string, strip it and run that decoder.
     *    if next part is a |, print
     * 3. If we are print, throw an exception to prevent continued analysis of buffer.
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
        throw printing_done;
	//return;			// our job is done
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
    scanner_t *s = be13::plugin::find_scanner(lowerstr(prefix));
    if(s){
        (*s)(scanner_params(scanner_params::PHASE_SCAN,
                            sbuf_t(new_path,sp.sbuf),
                            sp.fs,sp.print_options),
             recursion_control_block(process_path_printer,prefix));
        return;
    }
    cerr << "Unknown name in path: " << prefix << "\n";
}

/**
 * process_path uses the scanners to decode the path for the purpose of
 * decoding the image data and extracting the information.
 */


static void process_open_path(const image_process &p,string path,scanner_params::PrintOptions &po,
                              const size_t process_path_bufsize)
{
    /* Check for "/r" in path which means print raw */
    if(path.size()>2 && path.substr(path.size()-2,2)=="/r"){
	path = path.substr(0,path.size()-2);
    }

    if(debug & DEBUG_PRINT_STEPS){
        cerr << "process_path_open. path="<<path;
    }

    string  prefix = get_and_remove_token(path);
    int64_t offset = stoi64(prefix);

    if(debug & DEBUG_PRINT_STEPS){
        cerr << "  prefix="<<prefix<<" offset="<<offset<<"   new path="<<path<<"\n";
    }

    /* Get the offset into the buffer process */
    u_char *buf = (u_char *)calloc(process_path_bufsize,1);
    if(!buf){
        std::cerr << "Cannot allocate " << process_path_bufsize << " buffer\n";
        return;
    }
    int count = p.pread(buf,process_path_bufsize,offset);
    if(count<0){
	cerr << p.image_fname() << ": " << strerror(errno) << " (Read Error)\n";
	return;
    }

    /* make up a bogus feature recorder set and with a disabled feature recorder.
     * Then we call the path printer, which throws an exception after the printing
     * to prevent further printing.
     *
     * The printer is called when a PRINT token is found in the
     * forensic path, so that has to be added.
     */
    feature_recorder_set fs(feature_recorder_set::SET_DISABLED);

    pos0_t pos0(path+"-PRINT"); // insert the PRINT token
    sbuf_t sbuf(pos0,buf,count,count,true); // sbuf system will free
    scanner_params sp(scanner_params::PHASE_SCAN,sbuf,fs,po);
    try {
        process_path_printer(sp);
    }
    catch (path_printer_finished &e) {
    }
}

/**
 * process a path for a given filename.
 * Opens the image and calls the function above.
 * Also implements HTTP server with "-http" option.
 * Feature recorders disabled.
 */
static void process_path(const char *fn,string path,size_t pagesize,size_t marginsize)
{
    image_process *pp = image_process::open(fn,0,pagesize,0);
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
	    process_open_path(*pp,path,po,pagesize+marginsize);
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
	    process_open_path(*pp,p2,po,pagesize+marginsize);
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
    process_open_path(*pp,path,po,pagesize+marginsize);
}

class bulk_extractor_restarter {
    std::stringstream cdata;
    std::string thisElement;
    std::string provided_filename;
    BulkExtractor_Phase1::seen_page_ids_t &seen_page_ids;
#ifdef HAVE_LIBEXPAT
    static void startElement(void *userData, const char *name_, const char **attrs) {
        class bulk_extractor_restarter &self = *(bulk_extractor_restarter *)userData;
        self.cdata.str("");
        self.thisElement = name_;
        if(self.thisElement=="debug:work_start"){
            for(int i=0;attrs[i] && attrs[i+1];i+=2){
                if(strcmp(attrs[i],"pos0") == 0){
                    self.seen_page_ids.insert(attrs[i+1]);
                }
            }
        }
    }
    static void endElement(void *userData,const char *name_){
        class bulk_extractor_restarter &self = *(bulk_extractor_restarter *)userData;
        if(self.thisElement=="provided_filename") self.provided_filename = self.cdata.str();
        self.cdata.str("");
    }
    static void characterDataHandler(void *userData,const XML_Char *s,int len){ 
        class bulk_extractor_restarter &self = *(bulk_extractor_restarter *)userData;
        self.cdata.write(s,len);
    }
#endif

public:;
    bulk_extractor_restarter(const std::string &opt_outdir,
                             const std::string &reportfilename,
                             const std::string &image_fname,
                             BulkExtractor_Phase1::seen_page_ids_t &seen_page_ids_):
        cdata(),thisElement(),provided_filename(),seen_page_ids(seen_page_ids_){
#ifdef HAVE_LIBEXPAT
        if(access(reportfilename.c_str(),R_OK)){
            std::cerr << opt_outdir << ": error\n";
            std::cerr << "report.xml file is missing or unreadable.\n";
            std::cerr << "Directory may not have been created by bulk_extractor.\n";
            std::cerr << "Cannot continue.\n";
            exit(1);
        }
	
        XML_Parser parser = XML_ParserCreate(NULL);
        XML_SetUserData(parser, this);
        XML_SetElementHandler(parser, startElement, endElement);
        XML_SetCharacterDataHandler(parser,characterDataHandler);
        std::fstream in(reportfilename.c_str());
        if(!in.is_open()){
            std::cout << "Cannot open " << reportfilename << ": " << strerror(errno) << "\n";
            exit(1);
        }
        try {
            bool error = false;
            std::string line;
            while(getline(in,line) && !error){
                if (!XML_Parse(parser, line.c_str(), line.size(), 0)) {
                    std::cout << "XML Error: " << XML_ErrorString(XML_GetErrorCode(parser))
                              << " at line " << XML_GetCurrentLineNumber(parser) << "\n";
                    error = true;
                    break;
                }
            }
            if(!error) XML_Parse(parser, "", 0, 1);    // clear the parser
        }
        catch (const std::exception &e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
        XML_ParserFree(parser);
        if(image_fname != provided_filename){
            std::cerr << "Error: \n" << image_fname << " != " << provided_filename << "\n";
            exit(1);
        }
#else
        errx(1,"Compiled without libexpat; cannot restart.");
#endif
    }
};


/****************************************************************
 *** Usage
 ****************************************************************/

static void usage(const char *progname)
{
    BulkExtractor_Phase1::Config cfg;   // get a default config

    std::cout << "bulk_extractor version " PACKAGE_VERSION " " << /* svn_revision << */ "\n";
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
    std::cout << "   -s frac[:passes] - Set random sampling parameters\n";
    std::cout << "\nTuning parameters:\n";
    std::cout << "   -C NN        - specifies the size of the context window (default "
              << feature_recorder::context_window_default << ")\n";
    std::cout << "   -S fr:<name>:window=NN   specifies context window for recorder to NN\n";
    std::cout << "   -S fr:<name>:window_before=NN  specifies context window before to NN for recorder\n";
    std::cout << "   -S fr:<name>:window_after=NN   specifies context window after to NN for recorder\n";
    std::cout << "   -G NN        - specify the page size (default " << cfg.opt_pagesize << ")\n";
    std::cout << "   -g NN        - specify margin (default " <<cfg.opt_marginsize << ")\n";
    std::cout << "   -j NN        - Number of analysis threads to run (default " <<threadpool::numCPU() << ")\n";
    std::cout << "   -M nn        - sets max recursion depth (default " << scanner_def::max_depth << ")\n";
    std::cout << "   -m <max>     - maximum number of minutes to wait for memory starvation\n";
    std::cout << "                  default is " << cfg.max_bad_alloc_errors << "\n";
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
    std::cout << "   -S name=value - sets a bulk extractor option name to be value\n";
    std::cout << "\n";
    std::cout << "\n";
}


/**
 * Create the dfxml output
 */

static void dfxml_create(dfxml_writer &xreport,const string &command_line,const BulkExtractor_Phase1::Config &cfg)
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
    xreport.xmlout("threads",cfg.num_threads);
    xreport.xmlout("pagesize",cfg.opt_pagesize);
    xreport.xmlout("marginsize",cfg.opt_marginsize);
    xreport.push("scanners");
    /* Generate a list of the scanners in use */

    std::vector<std::string> ev;
    be13::plugin::get_enabled_scanners(ev);
    for(std::vector<std::string>::const_iterator it=ev.begin();it!=ev.end();it++){
        xreport.xmlout("scanner",(*it));
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
    if(str.find('t')!=string::npos  || str.find('T')!=string::npos) val *= 1024LL * 1024LL * 1024LL * 1024LL;
    return val;
}


/*
 * Make sure that the filename provided is sane.
 * That is, do not allow the analysis of a *.E02 file...
 */
void validate_fn(const std::string &fn)
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


void be_mkdir(string dir)
{
#ifdef WIN32
    if(mkdir(dir.c_str())){
        cerr << "Could not make directory " << dir << "\n";
        exit(1);
    }
#else
    if(mkdir(dir.c_str(),0777)){
        cerr << "Could not make directory " << dir << "\n";
        exit(1);
    }
#endif
}


/* be_hash. Currently this just returns the MD5 of the sbuf,
 * but eventually it will allow the use of different hashes.
 */
std::string be_hash_name("md5");
std::string be_hash(const uint8_t *buf,size_t bufsize)
{
    if(be_hash_name=="md5" || be_hash_name=="MD5"){
        return md5_generator::hash_buf(buf,bufsize).hexdigest();
    }
    if(be_hash_name=="sha1" || be_hash_name=="SHA1" || be_hash_name=="sha-1" || be_hash_name=="SHA-1"){
        return sha1_generator::hash_buf(buf,bufsize).hexdigest();
    }
    if(be_hash_name=="sha256" || be_hash_name=="SHA256" || be_hash_name=="sha-256" || be_hash_name=="SHA-256"){
        return sha256_generator::hash_buf(buf,bufsize).hexdigest();
    }
    std::cerr << "Invalid hash name: " << be_hash_name << "\n";
    std::cerr << "This version of bulk_extractor only supports MD5, SHA1, and SHA256\n";
    exit(1);
}



void stat_callback(void *user,const std::string &name,uint64_t calls,double seconds)
{
    dfxml_writer *xreport = reinterpret_cast<dfxml_writer *>(user);

    xreport->set_oneline(true);
    xreport->push("path");
    xreport->xmlout("name",name);
    xreport->xmlout("calls",(int64_t)calls);
    xreport->xmlout("seconds",seconds);
    xreport->pop();
    xreport->set_oneline(false);
}

/********************
 *** find support ***
 ********************/

regex_list find_list;
void add_find_pattern(const string &pat)
{
    find_list.add_regex("(" + pat + ")"); // make a group
}


void process_find_file(const char *findfile)
{
    ifstream in;

    in.open(findfile,ifstream::in);
    if(!in.good()){
	err(1,"Cannot open %s",findfile);
    }
    while(!in.eof()){
	string line;
	getline(in,line);
	truncate_at(line,'\r');         // remove a '\r' if present
	if(line.size()>0){
	    if(line[0]=='#') continue;	// ignore lines that begin with a comment character
	    add_find_pattern(line);
	}
    }
}



int main(int argc,char **argv)
{
#ifdef HAVE_MCHECK
    mtrace();
#endif
    feature_recorder::set_main_threadid();
    BulkExtractor_Phase1::Config cfg;
    cfg.num_threads = threadpool::numCPU();
    if(getenv("BULK_EXTRACTOR_DEBUG")){
	debug = atoi(getenv("BULK_EXTRACTOR_DEBUG"));
    }
    const char *progname = argv[0];
    scanner_info::scanner_config be_config; // the bulk extractor config
    const char *opt_path = 0;
    int opt_recurse = 0;
    int opt_zap = 0;
    string opt_outdir;
    setvbuf(stdout,0,_IONBF,0);		// don't buffer stdout
    std::string command_line = dfxml_writer::make_command_line(argc,argv);
    std::string opt_sampling_params;
    std::vector<std::string> scanner_dirs;

    /* Figure out the current time */
    time_t t = time(0);
    struct tm now;
    gmtime_r(&t,&now);
    opt_last_year = now.tm_year + 1900 + 5; // allow up to 5 years in the future
    int opt_h = 0;
    int opt_H = 0;

#ifdef WIN32
    setmode(1,O_BINARY);		// make stdout binary
    threadpool::win32_init();
#endif
    /* look for usage first */
    if(argc==1) opt_h=1;

    /* Process options */
    int ch;
    while ((ch = getopt(argc, argv, "A:B:b:C:d:E:e:F:f:G:g:Hhj:M:m:o:P:p:q:Rr:S:s:VW:w:x:Y:z:Z")) != -1) {
	switch (ch) {
	case 'A': feature_recorder::offset_add  = stoi64(optarg);break;
	case 'b': feature_recorder::banner_file = optarg; break;
	case 'C': feature_recorder::context_window_default = atoi(optarg);break;
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
            be13::plugin::set_scanner_debug(debug);
	}
	break;
	case 'E':
            be13::plugin::scanners_disable_all();
	    be13::plugin::scanners_enable(optarg);
	    break;
	case 'e':
	    be13::plugin::scanners_enable(optarg);
	    break;
	case 'F': process_find_file(optarg); break;
	case 'f': add_find_pattern(optarg); break;
	case 'G': cfg.opt_pagesize = scaled_stoi(optarg); break;
	case 'g': cfg.opt_marginsize = scaled_stoi(optarg); break;
	case 'j': cfg.num_threads = atoi(optarg); break;
	case 'M': scanner_def::max_depth = atoi(optarg); break;
	case 'm': cfg.max_bad_alloc_errors = atoi(optarg); break;
	case 'o': opt_outdir = optarg;break;
	case 'P': scanner_dirs.push_back(optarg);break;
	case 'p': opt_path = optarg; break;
        case 'q':
	    if(atoi(optarg)==-1) cfg.opt_quiet = 1;// -q -1 turns off notifications
	    else cfg.opt_notify_rate = atoi(optarg);
	    break;
	case 'r':
	    if(alert_list.readfile(optarg)){
		err(1,"Cannot read alert list %s",optarg);
	    }
	    break;
	case 'R': opt_recurse = 1; break;
	case 'S':
	{
	    std::vector<std::string> params = split(optarg,'=');
	    if(params.size()!=2){
		std::cerr << "Invalid paramter: " << optarg << "\n";
		exit(1);
	    }
	    be_config.namevals[params[0]] = params[1];
	    continue;
	}
	case 's': opt_sampling_params = optarg; break;
	case 'V': std::cout << "bulk_extractor " << PACKAGE_VERSION << "\n"; exit (1);
	case 'W':
            fprintf(stderr,"-W has been deprecated. Specify with -S word_min=NN and -S word_max=NN\n");
            exit(1);
	    break;
	case 'w': if(stop_list.readfile(optarg)){
		err(1,"Cannot read stop list %s",optarg);
	    }
	    break;
	case 'x':
	    be13::plugin::scanners_disable(optarg);
	    break;
	case 'Y': {
	    string optargs = optarg;
	    size_t dash = optargs.find('-');
	    if(dash==string::npos){
		cfg.opt_offset_start = stoi64(optargs);
	    } else {
		cfg.opt_offset_start = scaled_stoi(optargs.substr(0,dash));
		cfg.opt_offset_end   = scaled_stoi(optargs.substr(dash+1));
	    }
	    break;
	}
	case 'z': cfg.opt_page_start = stoi64(optarg);break;
	case 'Z': opt_zap=true;break;
	case 'H': opt_H++;continue;
	case 'h': opt_h++;continue;
	}
    }

    cfg.validate();

    argc -= optind;
    argv += optind;

    /* Create a configuration that will be used to initialize the scanners */
    extern bool opt_enable_histograms;
    scanner_info si;

    be_config.debug = debug;
    be_config.hasher.name = be_hash_name;
    be_config.hasher.func = be_hash;

    si.config = &be_config;
    si.get_config("work_start_work_end",&worker::opt_work_start_work_end,
                  "Record work start and end of each scanner in report.xml file");
    si.get_config("enable_histograms",&opt_enable_histograms,
                  "Disable generation of histograms");
    si.get_config("debug_histogram_malloc_fail_frequency",&HistogramMaker::debug_histogram_malloc_fail_frequency,
                  "Set >0 to make histogram maker fail with memory allocations");
    si.get_config("hash_alg",&be_hash_name,"Specifies hash algorithm to be used for all hash calculations");
    si.get_config("dup_data_alerts",&be13::plugin::dup_data_alerts,"Notify when duplicate data is not processed");

    /* Make sure that the user selected a valid hash */
    {
        uint8_t buf[1];
        be_hash(buf,0);
    }

    /* Load all the scanners and enable the ones we care about */

    be13::plugin::load_scanner_directories(scanner_dirs,be_config);
    be13::plugin::load_scanners(scanners_builtin,be_config); 
    be13::plugin::scanners_process_enable_disable_commands();

    /* Print usage if necessary */
    if(opt_H){ be13::plugin::info_scanners(true,true,scanners_builtin,'e','x'); exit(0);}
    if(opt_h){ usage(progname);be13::plugin::info_scanners(false,true,scanners_builtin,'e','x'); exit(0);}

    /* Give an error if a find list was specified
     * but no scanner that uses the find list is enabled.
     */

    if(find_list.size()>0){
        /* Look through the enabled scanners and make sure that
	 * at least one of them is a FIND scanner
	 */
        if(!be13::plugin::find_scanner_enabled()){
            errx(1,"find words are specified with -F but no find scanner is enabled.\n");
        }
    }

    if(opt_path){
	if(argc!=1) errx(1,"-p requires a single argument.");
	process_path(argv[0],opt_path,cfg.opt_pagesize,cfg.opt_marginsize);
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
    string reportfilename = opt_outdir + "/report.xml";

    BulkExtractor_Phase1::seen_page_ids_t seen_page_ids; // pages that do not need re-processing
    image_process *p = 0;
    std::string image_fname = *argv;

    if(opt_outdir.size()==0){
        fprintf(stderr,"output directory not provided\n");
        exit(1);
    }

    if(directory_missing(opt_outdir) || directory_empty(opt_outdir)){
        /* First time running */
	/* Validate the args */
	if ( argc !=1 ) errx(1,"Disk image option not provided. Run with -h for help.");
	validate_fn(image_fname);
	if (directory_missing(opt_outdir)) be_mkdir(opt_outdir);
    } else {
	/* Restarting */
	std::cout << "Restarting from " << opt_outdir << "\n";
        bulk_extractor_restarter r(opt_outdir,reportfilename,image_fname,seen_page_ids);

        /* Rename the old report and create a new one */
        std::string old_reportfilename = reportfilename + "." + itos(t);
        if(rename(reportfilename.c_str(),old_reportfilename.c_str())){
            std::cerr << "Could not rename " << reportfilename << " to " << old_reportfilename << ": " << strerror(errno) << "\n";
            exit(1);
        }
    }

    /* If disk image does not exist, we are in restart mode */
    p = image_process::open(image_fname,opt_recurse,cfg.opt_pagesize,cfg.opt_marginsize);
    if(!p) err(1,"Cannot open %s: ",image_fname.c_str());
    
    /* Determine the feature files that will be used */
    feature_file_names_t feature_file_names;
    be13::plugin::get_scanner_feature_file_names(feature_file_names);
    feature_recorder_set fs(feature_file_names,image_fname,opt_outdir,stop_list.size()>0);
    be13::plugin::scanners_init(&fs);

    /* Look for commands that impact per-recorders */
    for(scanner_info::config_t::const_iterator it=be_config.namevals.begin();it!=be_config.namevals.end();it++){
        /* see if there is a <recorder>: */
        std::vector<std::string> params = split(it->first,':');
        if(params.size()>=3 && params.at(0)=="fr"){
            feature_recorder *fr = fs.get_name(params.at(1));
            const std::string &cmd = params.at(2);
            if(fr){
                if(cmd=="window") fr->set_context_window(stoi(it->second));
                if(cmd=="window_before") fr->set_context_window_before(stoi(it->second));
                if(cmd=="window_after") fr->set_context_window_after(stoi(it->second));
            }
        }
        /* See if there is a scanner? */
    }


    /* Store the configuration in the XML file */
    dfxml_writer  *xreport = new dfxml_writer(reportfilename,false);
    dfxml_create(*xreport,command_line,cfg);
    xreport->xmlout("provided_filename",image_fname); // save this information


    /* provide documentation to the user; the DFXML information comes from elsewhere */
    if(!cfg.opt_quiet){
	std::cout << "bulk_extractor version: " << PACKAGE_VERSION << "\n";
#ifdef HAVE_GETHOSTNAME
	char hostname[1024];
	gethostname(hostname,sizeof(hostname));
	std::cout << "Hostname: " << hostname << "\n";
#endif
	std::cout << "Input file: " << image_fname << "\n";
	std::cout << "Output directory: " << opt_outdir << "\n";
	std::cout << "Disk Size: " << p->image_size() << "\n";
	std::cout << "Threads: " << cfg.num_threads << "\n";
    }

    /****************************************************************
     *** THIS IS IT! PHASE 1!
     ****************************************************************/

    //md5_generator *md5g = new md5_generator();		// keep track of MD5
    BulkExtractor_Phase1 phase1(*xreport,timer,cfg);

    if(opt_sampling_params.size()>0) BulkExtractor_Phase1::set_sampling_parameters(cfg,opt_sampling_params);

    xreport->add_timestamp("phase1 start");
    phase1.run(*p,fs,seen_page_ids);
    phase1.wait_for_workers(*p);
    xreport->add_timestamp("phase1 end");

    if(cfg.opt_quiet==0) std::cout << "Phase 2. Shutting down scanners\n";
    xreport->add_timestamp("phase2 start");
    be13::plugin::phase_shutdown(fs);
    xreport->add_timestamp("phase2 end");


    if(cfg.opt_quiet==0) std::cout << "Phase 3. Creating Histograms\n";
    xreport->add_timestamp("phase3 start");
    be13::plugin::phase_histogram(fs,0); // TK - add an xml error notifier!
    xreport->add_timestamp("phase3 end");

    /* report and then print final usage information */
    xreport->push("report");
    xreport->xmlout("total_bytes",phase1.total_bytes);
    xreport->xmlout("elapsed_seconds",timer.elapsed_seconds());
    xreport->xmlout("max_depth_seen",be13::plugin::get_max_depth_seen());
    xreport->xmlout("dup_data_encountered",be13::plugin::dup_data_encountered);
    xreport->pop();			// report
    xreport->flush();

    xreport->push("scanner_times");
    fs.get_stats(xreport,stat_callback);
    xreport->pop();
    xreport->add_rusage();
    xreport->pop();			// bulk_extractor
    xreport->close();
    delete p;				// not strictly needed, but why not?
    if(cfg.opt_quiet==0){
	float mb_per_sec = (phase1.total_bytes / 1000000.0) / timer.elapsed_seconds();

	std::cout.precision(4);
        printf("Elapsed time: %g sec.\n",timer.elapsed_seconds());
        printf("Total MB processed: %d\n",int(phase1.total_bytes / 100000));
        
        printf("Overall performance: %g MBytes/sec\n",mb_per_sec);
        if (fs.has_name("email")) {
            feature_recorder *fr = fs.get_name("email");
            if(fr){
                std::cout << "Total " << fr->name << " features found: " << fr->count() << "\n";
            }
        }
    }
#ifdef HAVE_MCHECK
    muntrace();
#endif
    exit(0);
}

