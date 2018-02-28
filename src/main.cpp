/*
 * main.cpp
 *
 * The main() for bulk_extractor.
 * This has all of the code and global variables that aren't needed when BE is running as a library.
 */

/**
 * Singletons:
 * feature_recorder_set fs - the collection of feature recorders.
 * xml xreport             - the DFXML output.
 * image_process p         - the image being processed.
 * 
 * Note that all of the singletons are passed to the phase1() function.
 */


#include "bulk_extractor.h"
#include "findopts.h"
#include "image_process.h"
#include "threadpool.h"
#include "be13_api/aftimer.h"
#include "be13_api/histogram.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"
#include "be13_api/unicode_escape.h"

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

#ifdef WIN32 
// Allows us to open standard input in binary mode by default 
// See http://gnuwin32.sourceforge.net/compile.html for more 
int _CRT_fmode = _O_BINARY;
#endif

/* Debug help */
__attribute__((noreturn)) 
void debug_help()
{
    puts("#define DEBUG_PEDANTIC    0x0001	// check values more rigorously");
    puts("#define DEBUG_PRINT_STEPS 0x0002      // prints as each scanner is started");
    puts("#define DEBUG_SCANNER     0x0004	// dump all feature writes to stderr");
    puts("#define DEBUG_NO_SCANNERS 0x0008      // do not run the scanners ");
    puts("#define DEBUG_DUMP_DATA   0x0010	// dump data as it is seen ");
    puts("#define DEBUG_INFO        0x0040	// print extra info");
    puts("#define DEBUG_EXIT_EARLY  1000	// just print the size of the volume and exis ");
    puts("#define DEBUG_ALLOCATE_512MiB 1002	// Allocate 512MiB, but don't set any flags ");
    exit(1);
}

/****************************************************************
 *** Usage for the stand-alone program
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
#ifdef HAVE_LIBLIGHTGREP
    std::cout << "                  LIGHTGREP ENABLED\n";
#endif
    std::cout << "   -o outdir    - specifies output directory. Must not exist.\n";
    std::cout << "                  bulk_extractor creates this directory.\n";
    std::cout << "Options:\n";
    std::cout << "   -i           - INFO mode. Do a quick random sample and print a report.\n";
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
    std::cout << "   -m <max>     - maximum number of minutes to wait after all data read\n";
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
    std::cout << "   -dN          - debug mode (see source code)\n";
    std::cout << "   -Z           - zap (erase) output directory\n";
    std::cout << "\nControl of Scanners:\n";
    std::cout << "   -P <dir>     - Specifies a plugin directory\n";
    std::cout << "             Default dirs include /usr/local/lib/bulk_extractor /usr/lib/bulk_extractor and\n";
    std::cout << "             BE_PATH environment variable\n";
    std::cout << "   -e <scanner>  enables <scanner> -- -e all   enables all\n";
    std::cout << "   -x <scanner>  disable <scanner> -- -x all   disables all\n";
    std::cout << "   -E <scanner>    - turn off all scanners except <scanner>\n";
    std::cout << "                     (Same as -x all -e <scanner>)\n";
    std::cout << "          note: -e, -x and -E commands are executed in order\n";
    std::cout << "              e.g.: '-E gzip -e facebook' runs only gzip and facebook\n";
    std::cout << "   -S name=value - sets a bulk extractor option name to be value\n";
    std::cout << "\n";
}


std::string svn_revision_clean()
{
    return std::string("");
}

/**
 * scaled_stoi64:
 * Like a normal stoi, except it can handle modifies k, m, and g
 */
static uint64_t scaled_stoi64(const std::string &str)
{
    std::stringstream ss(str);
    uint64_t val;
    ss >> val;
    if(str.find('k')!=std::string::npos  || str.find('K')!=std::string::npos) val *= 1024;
    if(str.find('m')!=std::string::npos  || str.find('m')!=std::string::npos) val *= 1024 * 1024;
    if(str.find('g')!=std::string::npos  || str.find('g')!=std::string::npos) val *= 1024 * 1024 * 1024;
    if(str.find('t')!=std::string::npos  || str.find('T')!=std::string::npos) val *= 1024LL * 1024LL * 1024LL * 1024LL;
    return val;
}


/* be_hash. Currently this just returns the MD5 of the sbuf,
 * but eventually it will allow the use of different hashes.
 */
static std::string be_hash_name("md5");
static std::string be_hash_func(const uint8_t *buf,size_t bufsize)
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
static feature_recorder_set::hash_def be_hash(be_hash_name,be_hash_func);

static int stat_callback(void *user,const std::string &name,uint64_t calls,double seconds)
{
    dfxml_writer *xreport = reinterpret_cast<dfxml_writer *>(user);

    xreport->set_oneline(true);
    xreport->push("path");
    xreport->xmlout("name",name);
    xreport->xmlout("calls",(int64_t)calls);
    xreport->xmlout("seconds",seconds);
    xreport->pop();
    xreport->set_oneline(false);
    return 0;
}

void be_mkdir(const std::string &dir)
{
#ifdef WIN32
    if(mkdir(dir.c_str())){
        std::cerr << "Could not make directory " << dir << "\n";
        exit(1);
    }
#else
    if(mkdir(dir.c_str(),0777)){
        std::cerr << "Could not make directory " << dir << "\n";
        exit(1);
    }
#endif
}

/*
 * Make sure that the filename provided is sane.
 * That is, do not allow the analysis of a *.E02 file...
 */
void validate_fn(const std::string &fn)
{
    int r= access(fn.c_str(),R_OK);
    if(r!=0){
#ifndef WIN32
      /* ignore this check under windows for now */
        std::cerr << "cannot open: " << fn << ": " << strerror(errno) << " (code " << r << ")\n";
	exit(1);
#endif
    }
    if(fn.size()>3){
	size_t e = fn.rfind('.');
	if(e!=std::string::npos){
            std::string ext = fn.substr(e+1);
	    if(ext=="E02" || ext=="e02"){
                std::cerr << "Error: invalid file name\n";
                std::cerr << "Do not use bulk_extractor to process individual EnCase files.\n";
                std::cerr << "Instead, just run bulk_extractor with FILENAME.E01\n";
                std::cerr << "The other files in an EnCase multi-volume archive will be opened\n";
                std::cerr << "automatically.\n";
		exit(1);
	    }
	}
    }
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


/***************************************************************************************
 *** PATH PRINTER - Used by bulk_extractor for printing pages associated with a path ***
 ***************************************************************************************/

/* Get the next token from the path. Tokens are separated by dashes.
 * NOTE: modifies argument
 */
static std::string get_and_remove_token(std::string &path)
{
    while(path[0]=='-'){
	path = path.substr(1); // remove any leading dashes.
    }
    size_t dash = path.find('-');	// find next dash
    if(dash==std::string::npos){		// no string; return the rest
        std::string prefix = path;
	path = "";
	return prefix;
    }
    std::string prefix = path.substr(0,dash);
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

static std::string HTTP_EOL = "\r\n";		// stdout is in binary form
void process_path_printer(const scanner_params &sp)
{
    /* 1. Get next token 
     * 2. if prefix part is a number, skip forward that much in sbuf and repeat.
     *    if the prefix is PRINT, print the buffer
     *    if next part is a string, strip it and run that decoder.
     *    if next part is a |, print
     * 3. If we are print, throw an exception to prevent continued analysis of buffer.
     */

    std::string new_path = sp.sbuf.pos0.path;
    std::string prefix = get_and_remove_token(new_path);

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
                std::string v1 = it->second.substr(6,dash-6);
                std::string v2 = it->second.substr(dash+1);
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
    std::cerr << "Unknown name in path: " << prefix << "\n";
}



/**
 * process_path uses the scanners to decode the path for the purpose of
 * decoding the image data and extracting the information.
 */


static void process_open_path(const image_process &p,std::string path,scanner_params::PrintOptions &po,
                              const size_t process_path_bufsize)
{
    /* Check for "/r" in path which means print raw */
    if(path.size()>2 && path.substr(path.size()-2,2)=="/r"){
	path = path.substr(0,path.size()-2);
    }

    std::string  prefix = get_and_remove_token(path);
    int64_t offset = stoi64(prefix);

    /* Get the offset into the buffer process */
    u_char *buf = (u_char *)calloc(process_path_bufsize,1);
    if(!buf){
        std::cerr << "Cannot allocate " << process_path_bufsize << " buffer\n";
        return;
    }
    int count = p.pread(buf,process_path_bufsize,offset);
    if(count<0){
        std::cerr << p.image_fname() << ": " << strerror(errno) << " (Read Error)\n";
	return;
    }

    /* make up a bogus feature recorder set and with a disabled feature recorder.
     * Then we call the path printer, which throws an exception after the printing
     * to prevent further printing.
     *
     * The printer is called when a PRINT token is found in the
     * forensic path, so that has to be added.
     */
    feature_recorder_set fs(feature_recorder_set::SET_DISABLED,feature_recorder_set::null_hasher,
                            feature_recorder_set::NO_INPUT,feature_recorder_set::NO_OUTDIR);

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
static void process_path(const char *fn,std::string path,size_t pagesize,size_t marginsize)
{
    image_process *pp = image_process::open(fn,0,pagesize,0);
    if(pp==0){
	if(path=="-http"){
	    std::cout << "HTTP/1.1 502 Filename " << fn << " is invalid" << HTTP_EOL << HTTP_EOL;
	} else {
            std::cerr << "Filename " << fn << " is invalid\n";
	}
	exit(1);
    }

    if(path=="-"){
	/* process path interactively */
	printf("Path Interactive Mode:\n");
	if(pp==0){
            std::cerr << "Invalid file name: " << fn << "\n";
	    exit(1);
	}
	do {
	    getline(std::cin,path);
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
            std::string line;		// the specific query
	    scanner_params::PrintOptions po;
	    scanner_params::setPrintMode(po,scanner_params::MODE_HTTP);	// options for this query
	    
	    getline(std::cin,line);
	    truncate_at(line,'\r');
	    if(line.substr(0,4)!="GET "){
		std::cout << "HTTP/1.1 501 Method not implemented" << HTTP_EOL << HTTP_EOL;
		return;
	    }
	    size_t space = line.find(" HTTP/1.1");
	    if(space==std::string::npos){
		std::cout << "HTTP/1.1 501 Only HTTP/1.1 is implemented" << HTTP_EOL << HTTP_EOL;
		return;
	    }
            std::string p2 = line.substr(4,space-4);

	    /* Get the additional header options */
	    do {
		getline(std::cin,line);
		truncate_at(line,'\r');
		if(line.size()==0) break; // double new-line
		size_t colon = line.find(":");
		if(colon==std::string::npos){
		    std::cout << "HTTP/1.1 502 Malformed HTTP request" << HTTP_EOL;
		    return;
		}
                std::string name = line.substr(0,colon);
                std::string val  = line.substr(colon+1);
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

/**
 * Create the dfxml output
 */

static void dfxml_create(dfxml_writer &xreport,const std::string &command_line,const BulkExtractor_Phase1::Config &cfg)
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



/* It is gross to do this with statics rather than a singleton whose address is passed in 'user',
 * but that is the way it is implemented.
 */
static std::string current_ofname;
static std::ofstream o;
static bool needs_stamping=false;
static int histogram_dump_callback(void *user,const feature_recorder &fr,
                                   const histogram_def &def,
                                   const std::string &str,const uint64_t &count)
{
    if(count==0){
        if (o.is_open()) o.close();        // close old stream
        std::string ofname = fr.fname_counter(def.suffix);
        if (current_ofname != ofname){
            o.open(ofname.c_str());
            if(!o.is_open()){
                std::cerr << "Cannot open histogram output file: " << ofname << "\n";
                return -1;
            }
            needs_stamping = true;
        }
        return 0;
    }

    if (needs_stamping){
        fr.banner_stamp(o,feature_recorder::histogram_file_header);
        needs_stamping = false;
    }

    o << str << "\t" << "n=" << count << "\n";
    return 0;
}

static void add_if_present(std::vector<std::string> &scanner_dirs,const std::string &dir)
{
    if (access(dir.c_str(),O_RDONLY) == 0){
        scanner_dirs.push_back(dir);
    }
}

int main(int argc,char **argv)
{
#ifdef HAVE_MCHECK
    mtrace();
#endif

    /* setup */
    feature_recorder::set_main_threadid();
    const char *progname = argv[0];

    word_and_context_list alert_list;		/* shold be flagged */
    word_and_context_list stop_list;		/* should be ignored */

    scanner_info::scanner_config   s_config;    // the bulk extractor phase 1 config created from the command line
    BulkExtractor_Phase1::Config   cfg;
    cfg.num_threads = threadpool::numCPU();

    /* Options */
    const char *opt_path = 0;
    int         opt_recurse = 0;
    int         opt_zap = 0;
    int         opt_h = 0;
    int         opt_H = 0;
    std::string opt_sampling_params;
    std::string opt_outdir;
    bool        opt_write_feature_files = true;
    bool        opt_write_sqlite3     = false;
    bool        opt_enable_histograms = true;

    /* Startup */
    setvbuf(stdout,0,_IONBF,0);		// don't buffer stdout
    std::string command_line = dfxml_writer::make_command_line(argc,argv);
    std::vector<std::string> scanner_dirs; // where to look for scanners

    /* Add the default plugin_path */
    add_if_present(scanner_dirs,"/usr/local/lib/bulk_extractor");
    add_if_present(scanner_dirs,"/usr/lib/bulk_extractor");
    add_if_present(scanner_dirs,".");

    if (getenv("BE_PATH")) {
        std::vector<std::string> dirs = split(getenv("BE_PATH"),':');
        for(std::vector<std::string>::const_iterator it = dirs.begin(); it!=dirs.end(); it++){
            add_if_present(scanner_dirs,*it);
        }
    }

#ifdef WIN32
    setmode(1,O_BINARY);		// make stdout binary
    threadpool::win32_init();
#endif
    /* look for usage first */
    if(argc==1) opt_h=1;

    /* Process options */
    int ch;
    while ((ch = getopt(argc, argv, "A:B:b:C:d:E:e:F:f:G:g:Hhij:M:m:o:P:p:q:Rr:S:s:VW:w:x:Y:z:Z")) != -1) {
	switch (ch) {
	case 'A': feature_recorder::offset_add  = stoi64(optarg);break;
	case 'b': feature_recorder::banner_file = optarg; break;
	case 'C': feature_recorder::context_window_default = atoi(optarg);break;
	case 'd':
	{
            if(strcmp(optarg,"h")==0) debug_help();
	    int d = atoi(optarg);
	    switch(d){
	    case DEBUG_ALLOCATE_512MiB: 
		if(calloc(1024*1024*512,1)){
                    std::cerr << "-d1002 -- Allocating 512MB of RAM; may be repeated\n";
		} else {
                    std::cerr << "-d1002 -- CANNOT ALLOCATE MORE RAM\n";
		}
		break;
	    default:
		cfg.debug  = d;
		break;
	    }
            be13::plugin::set_scanner_debug(cfg.debug);
	}
	break;
	case 'E':
            be13::plugin::scanners_disable_all();
	    be13::plugin::scanners_enable(optarg);
	    break;
	case 'e':
	    be13::plugin::scanners_enable(optarg);
	    break;
	case 'F': FindOpts::get().Files.push_back(optarg); break;
	case 'f': FindOpts::get().Patterns.push_back(optarg); break;
	case 'G': cfg.opt_pagesize = scaled_stoi64(optarg); break;
	case 'g': cfg.opt_marginsize = scaled_stoi64(optarg); break;
        case 'i':
            std::cout << "info mode:\n";
            cfg.opt_info = true;
            break;
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
	    s_config.namevals[params[0]] = params[1];
	    continue;
	}
	case 's':
#if defined(HAVE_SRANDOM) && !defined(HAVE_SRANDOMDEV)
            srandom(time(0));
#endif
#if defined(HAVE_SRANDOMDEV)
            srandomdev();               // if we are sampling initialize
#endif
            opt_sampling_params = optarg;
            break;
	case 'V': std::cout << "bulk_extractor " << PACKAGE_VERSION << "\n"; exit (0);
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
	    std::string optargs = optarg;
	    size_t dash = optargs.find('-');
	    if(dash==std::string::npos){
		cfg.opt_offset_start = stoi64(optargs);
	    } else {
		cfg.opt_offset_start = scaled_stoi64(optargs.substr(0,dash));
		cfg.opt_offset_end   = scaled_stoi64(optargs.substr(dash+1));
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

    if(cfg.debug & DEBUG_PRINT_STEPS) std::cerr << "DEBUG: DEBUG_PRINT_STEPS\n";
    if(cfg.debug & DEBUG_PEDANTIC) validateOrEscapeUTF8_validate = true;

    /* Create a configuration that will be used to initialize the scanners */
    scanner_info si;

    s_config.debug       = cfg.debug;
    si.config = &s_config;

    /* Make individual configuration options appear on the command line interface. */
    si.get_config("work_start_work_end",&worker::opt_work_start_work_end,
                  "Record work start and end of each scanner in report.xml file");
    si.get_config("enable_histograms",&opt_enable_histograms,
                  "Disable generation of histograms");
    si.get_config("debug_histogram_malloc_fail_frequency",&HistogramMaker::debug_histogram_malloc_fail_frequency,
                  "Set >0 to make histogram maker fail with memory allocations");
    si.get_config("hash_alg",&be_hash_name,"Specifies hash algorithm to be used for all hash calculations");
    si.get_config("dup_data_alerts",&be13::plugin::dup_data_alerts,"Notify when duplicate data is not processed");
    si.get_config("write_feature_files",&opt_write_feature_files,"Write features to flat files");
    si.get_config("write_feature_sqlite3",&opt_write_sqlite3,"Write feature files to report.sqlite3");
    si.get_config("report_read_errors",&cfg.opt_report_read_errors,"Report read errors");

    /* Make sure that the user selected a valid hash */
    {
        uint8_t buf[1];
        be_hash_func(buf,0);
    }

    /* Load all the scanners and enable the ones we care about */

    be13::plugin::load_scanner_directories(scanner_dirs,s_config);
    be13::plugin::load_scanners(scanners_builtin,s_config); 
    be13::plugin::scanners_process_enable_disable_commands();

    /* Print usage if necessary */
    if(opt_H){ be13::plugin::info_scanners(true,true,scanners_builtin,'e','x'); exit(0);}
    if(opt_h){ usage(progname);be13::plugin::info_scanners(false,true,scanners_builtin,'e','x'); exit(0);}

    /* Give an error if a find list was specified
     * but no scanner that uses the find list is enabled.
     */

    if(!FindOpts::get().empty()) {
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
                std::string name = dp->d_name;
		if(name=="." || name=="..") continue;
                std::string fname = opt_outdir + std::string("/") + name;
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
    std::string reportfilename = opt_outdir + "/report.xml";

    BulkExtractor_Phase1::seen_page_ids_t seen_page_ids; // pages that do not need re-processing
    image_process *p = 0;                                // the image process iterator

    /* Get image or directory */
    if (*argv == NULL) {
        if (opt_recurse) {
            fprintf(stderr,"filedir not provided\n");
        } else {
            fprintf(stderr,"imagefile not provided\n");
        }
        exit(1);
    }
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
        std::string old_reportfilename = reportfilename + "." + itos(time(0));
        if(rename(reportfilename.c_str(),old_reportfilename.c_str())){
            std::cerr << "Could not rename " << reportfilename << " to " << old_reportfilename << ": " << strerror(errno) << "\n";
            exit(1);
        }
    }

    /* Open the image file (or the device) now */
    p = image_process::open(image_fname,opt_recurse,cfg.opt_pagesize,cfg.opt_marginsize);
    if(!p) err(1,"Cannot open %s: ",image_fname.c_str());
    
    /***
     *** Create the feature recording set.
     *** Initialize the scanners.
     ****/

    /* Determine the feature files that will be used */
    feature_file_names_t feature_file_names;
    be13::plugin::get_scanner_feature_file_names(feature_file_names);
    uint32_t flags = 0;
    if (stop_list.size()>0)        flags |= feature_recorder_set::CREATE_STOP_LIST_RECORDERS;
    if (opt_write_sqlite3)         flags |= feature_recorder_set::ENABLE_SQLITE3_RECORDERS;
    if (!opt_write_feature_files)  flags |= feature_recorder_set::DISABLE_FILE_RECORDERS;

    {
        feature_recorder_set fs(flags,be_hash,image_fname,opt_outdir);
        fs.init(feature_file_names);
        if(opt_enable_histograms) be13::plugin::add_enabled_scanner_histograms_to_feature_recorder_set(fs);
        be13::plugin::scanners_init(fs);

        fs.set_stop_list(&stop_list);
        fs.set_alert_list(&alert_list);

        /* Look for commands that impact per-recorders */
        for(scanner_info::config_t::const_iterator it=s_config.namevals.begin();it!=s_config.namevals.end();it++){
            /* see if there is a <recorder>: */
            std::vector<std::string> params = split(it->first,':');
            if(params.size()>=3 && params.at(0)=="fr"){
                feature_recorder *fr = fs.get_name(params.at(1));
                const std::string &cmd = params.at(2);
                if(fr){
                    if(cmd=="window")        fr->set_context_window(stoi64(it->second));
                    if(cmd=="window_before") fr->set_context_window_before(stoi64(it->second));
                    if(cmd=="window_after")  fr->set_context_window_after(stoi64(it->second));
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
            memset(hostname,0,sizeof(hostname));
            if(gethostname(hostname,sizeof(hostname)-1)==0){
                if (hostname[0]) std::cout << "Hostname: " << hostname << "\n";
            }
#endif
            std::cout << "Input file: " << image_fname << "\n";
            std::cout << "Output directory: " << opt_outdir << "\n";
            std::cout << "Disk Size: " << p->image_size() << "\n";
            std::cout << "Threads: " << cfg.num_threads << "\n";
        }

        /****************************************************************
         *** THIS IS IT! PHASE 1!
         ****************************************************************/

        if ( fs.flag_set(feature_recorder_set::ENABLE_SQLITE3_RECORDERS )) {
            fs.db_transaction_begin();
        }
        BulkExtractor_Phase1 phase1(*xreport,timer,cfg);
        if(cfg.debug & DEBUG_PRINT_STEPS) std::cerr << "DEBUG: STARTING PHASE 1\n";

        if(opt_sampling_params.size()>0) BulkExtractor_Phase1::set_sampling_parameters(cfg,opt_sampling_params);
        xreport->add_timestamp("phase1 start");
        phase1.run(*p,fs,seen_page_ids);

        if(cfg.debug & DEBUG_PRINT_STEPS) std::cerr << "DEBUG: WAITING FOR WORKERS\n";
        std::string md5_string;
        phase1.wait_for_workers(*p,&md5_string);
        delete p;				// not strictly needed, but why not?
        p = 0;

        if ( fs.flag_set(feature_recorder_set::ENABLE_SQLITE3_RECORDERS )) {
            fs.db_transaction_commit();
        }
        xreport->add_timestamp("phase1 end");
        if(md5_string.size()>0){
            std::cout << "MD5 of Disk Image: " << md5_string << "\n";
        }

        /*** PHASE 2 --- Shutdown ***/
        if(cfg.opt_quiet==0) std::cout << "Phase 2. Shutting down scanners\n";
        xreport->add_timestamp("phase2 (shutdown) start");
        be13::plugin::phase_shutdown(fs);
        xreport->add_timestamp("phase2 (shutdown) end");

        /*** PHASE 3 --- Create Histograms ***/
        if(cfg.opt_quiet==0) std::cout << "Phase 3. Creating Histograms\n";
        xreport->add_timestamp("phase3 (histograms) start");
        if(opt_enable_histograms) fs.dump_histograms(0,histogram_dump_callback,0);        // TK - add an xml error notifier!
        xreport->add_timestamp("phase3 (histograms) end");

        /*** PHASE 4 ---  report and then print final usage information ***/
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
        if(cfg.opt_quiet==0){
            float mb_per_sec = (phase1.total_bytes / 1000000.0) / timer.elapsed_seconds();

            std::cout.precision(4);
            printf("Elapsed time: %g sec.\n",timer.elapsed_seconds());
            printf("Total MB processed: %d\n",int(phase1.total_bytes / 1000000));
        
            printf("Overall performance: %g MBytes/sec (%g MBytes/sec/thread)\n",
                   mb_per_sec,mb_per_sec/cfg.num_threads);
            if (fs.has_name("email")) {
                feature_recorder *fr = fs.get_name("email");
                if(fr){
                    std::cout << "Total " << fr->name << " features found: " << fr->count() << "\n";
                }
            }
        }
    }
#ifdef HAVE_MCHECK
    muntrace();
#endif
    exit(0);
}
