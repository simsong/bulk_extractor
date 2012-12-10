/**
 * image_process.cpp:
 *
 * 64-bit file support.
 * Linux: See http://www.gnu.org/s/hello/manual/libc/Feature-Test-Macros.html
 *
 * MacOS & FreeBSD: Not needed; off_t is a 64-bit value.
 */

// Just for this module
#define _FILE_OFFSET_BITS 64

#include "bulk_extractor.h"
#include "dig.h"

#ifdef HAVE_LIBAFFLIB
#ifndef HAVE_STL
#define HAVE_STL			/* needed */
#endif
#include <afflib/afflib.h>
#include <afflib/utils.h>
#endif

#include "image_process.h"

#ifndef PATH_MAX
#define PATH_MAX 65536
#endif

extern scanner_def scanners[];
extern int debug;

#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif 

#ifndef MAX
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#endif 


/****************************************************************
 *** AFF
 ****************************************************************/

#ifdef HAVE_LIBAFFLIB
int process_aff::open()
{
    const char *fn = image_fname.c_str();
    af = af_open(fn,O_RDONLY,0666);
    if(!af){
	return -1;
    }
    if(af_cannot_decrypt(af)) errx(1,"Cannot decrypt %s: encryption key not set?",fn);

    /* build the pagelist */
    aff::seglist sl(af);			// get a segment list
    for(aff::seglist::const_iterator i=sl.begin();i!=sl.end();i++){
	if((*i).pagenumber()>=0){
	    pagelist.push_back((*i).pagenumber());
	}
    }
    sort(pagelist.begin(),pagelist.end());
    return 0;
}

int process_aff::pread(unsigned char *buf,size_t bytes,int64_t offset) const
{
    af_seek(af,offset,0);
    return af_read(af,buf,bytes);
}

int64_t process_aff::image_size()
{
    return af_get_imagesize(af);
}


/**
 * Iterator support
 */

image_process::iterator process_aff::begin()
{
    image_process::iterator it;
    it.myimage = this;
    it.raw_offset = 0;
    return it;
}

image_process::iterator process_aff::end()
{
    image_process::iterator it;
    it.page_counter = pagelist.size();
    it.raw_offset = af_get_imagesize(af);
    it.myimage = this;
    it.eof = true;
    return it;
}

/* Note - af_get_pagesize() used to be in afflib_i.h; it was moved,
 * but we may not have the new definition.
 */
#ifdef HAVE_LIBAFFLIB
__BEGIN_DECLS
#ifdef GNUC_HAS_DIAGNOSTIC_PRAGMA
#pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
int	af_get_pagesize(AFFILE *af);	// returns page size, or -1
__END_DECLS
#endif


/* Increment the AFF iterator by going to the next page.
 * If we hit the end of the pagelist, note that we are at the end of file.
 */
void process_aff::increment_iterator(class image_process::iterator &it)
{
    if(it.page_counter < pagelist.size()){
	it.page_counter++;
    it.raw_offset += it.page_counter * af_get_pagesize(af);
    } else {
	it.eof = true;
    }
}

pos0_t process_aff::get_pos0(const image_process::iterator &it) const
{
    int64_t pagenum = pagelist[it.page_counter];
    pos0_t pos0;
    pos0.offset = pagenum * af_get_pagesize(af);
    return pos0;
}

sbuf_t *process_aff::sbuf_alloc(image_process::iterator &it)
{
    size_t bufsize  = af_get_pagesize(af)+opt_margin;
    unsigned char *buf = (unsigned char *)malloc(bufsize);
    if(!buf) throw bad_alloc();

    pos0_t pos0 = get_pos0(it);
    
    af_seek(af,pos0.offset,0);
    ssize_t bytes_read = af_read(af,buf,bufsize);
    /**
     * af_read() returns 0 at end of file, if no data is available,
     * or if the data is bad. We need to be willing to return an sbuf
     * with zero bytes, which we do below.
     */
    if(bytes_read>=0){
	ssize_t pagesize = af_get_pagesize(af);
	if(pagesize>bytes_read) pagesize = bytes_read;
	sbuf_t *sbuf = new sbuf_t(pos0,buf,bytes_read,pagesize,true);
	return sbuf;
    }
    free(buf);
    return 0;				// no buffer to return
}

double process_aff::fraction_done(class image_process::iterator &it)
{
    return (double)it.page_counter / (double)pagelist.size();
}

string process_aff::str(class image_process::iterator &it)
{
    char buf[64];
    snprintf(buf,sizeof(buf),"Page %"PRId64"",it.page_counter);
    return string(buf);
}


process_aff::~process_aff()
{
    if(af) af_close(af);
}
#endif


/****************************************************************
 *** EWF
 ****************************************************************/

/**
 * Works with both new API and old API
 */

#ifdef HAVE_LIBEWF
#ifdef libewf_handle_get_header_value_case_number
#define LIBEWFNG
#endif

process_ewf::~process_ewf()
{
#ifdef LIBEWFNG
    if(handle){
	libewf_handle_close(handle,NULL);
	libewf_handle_free(&handle,NULL);
    }
#else
    if(handle){
	libewf_close(handle);
    }
#endif    
}

int process_ewf::open()
{
    const char *fname = image_fname.c_str();
    char **libewf_filenames = NULL;
    int amount_of_filenames = 0;

#ifdef LIBEWFNG
    libewf_error_t *error=0;
    if(libewf_glob(fname,strlen(fname),LIBEWF_FORMAT_UNKNOWN,
		   &libewf_filenames,&amount_of_filenames,&error)<0){
	libewf_error_fprint(error,stdout);
	libewf_error_free(&error);
	err(1,"libewf_glob");
    }
    handle = 0;
    if(libewf_handle_initialize(&handle,NULL)<0){
	err(1,"Cannot initialize EWF handle?");
    }
    if(libewf_handle_open(handle,libewf_filenames,amount_of_filenames,
			  LIBEWF_OPEN_READ,&error)<0){
	if(error) libewf_error_fprint(error,stdout);
	err(1,"Cannot open: %s",fname);
    }
    if(libewf_glob_free(libewf_filenames,amount_of_filenames,&error)<0){
	printf("libewf_glob_free failed\n");
	if(error) libewf_error_fprint(error,stdout);
	err(1,"libewf_glob_free");
    }
    libewf_handle_get_media_size(handle,(size64_t *)&ewf_filesize,NULL);
#else

    amount_of_filenames = libewf_glob(fname,strlen(fname),LIBEWF_FORMAT_UNKNOWN,&libewf_filenames);
    if(amount_of_filenames<0){
	err(1,"libewf_glob");
    }
    handle = libewf_open(libewf_filenames,amount_of_filenames,LIBEWF_OPEN_READ);
    if(handle==0){
	fprintf(stderr,"amount_of_filenames:%d\n",amount_of_filenames);
	for(int i=0;i<amount_of_filenames;i++){
	    fprintf(stderr,"  %s\n",libewf_filenames[i]);
	}
	err(1,"libewf_open");
    }
    libewf_get_media_size(handle,(size64_t *)&ewf_filesize);
#endif

#ifdef HAVE_LIBEWF_HANDLE_GET_UTF8_HEADER_VALUE_NOTES
    uint8_t ewfbuf[65536];
    int status= libewf_handle_get_utf8_header_value_notes(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if(status == 1 && strlen(ewfbuf)>0){
	std::string notes = reinterpret_cast<char *>(ewfbuf);
	details.push_back(std::string("NOTES: ")+notes);
    }
  
    status = libewf_handle_get_utf8_header_value_case_number(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if(status == 1 && strlen(ewfbuf)>0){
	std::string case_number = reinterpret_cast<char *>(ewfbuf);
	details.push_back(std::string("CASE NUMBER: ")+case_number);
    }

    status = libewf_handle_get_utf8_header_value_evidence_number(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if(status == 1 && strlen(ewfbuf)>0){
	std::string evidenceno = reinterpret_cast<char *>(ewfbuf);
	details.push_back(std::string("EVIDENCE NUMBER: ")+evidenceno);
    }

    status = libewf_handle_get_utf8_header_value_examiner_name(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if(status == 1 && strlen(ewfbuf)>0){
	std::string examinername = reinterpret_cast<char *>(ewfbuf) ;
	details.push_back(std::string("EXAMINER NAME: "+examinername));
    }
#endif	
    return 0;
}

vector<string> process_ewf::getewfdetails(){
	return(details);
}


int process_ewf::pread(unsigned char *buf,size_t bytes,int64_t offset) const
{
#ifdef LIBEWFNG
    libewf_error_t *error=0;
    int ret = libewf_handle_read_random(handle,buf,bytes,offset,&error);
    if(ret<0){
#ifdef HAVE_LIBEWF_ERROR_BACKTRACE_FPRINT
	if(debug & DEBUG_PEDANTIC) libewf_error_backtrace_fprint(error,stderr);
#endif
	libewf_error_fprint(error,stderr);
	libewf_error_free(&error);
    }
    return ret;
#else
    if((int64_t)bytes+offset > (int64_t)ewf_filesize){
	bytes = ewf_filesize - offset;
    }
    return libewf_read_random(handle,buf,bytes,offset);
#endif
}


int64_t process_ewf::image_size()
{
    return ewf_filesize;
}


image_process::iterator process_ewf::begin()
{
    image_process::iterator it;
    it.raw_offset = 0;
    it.myimage = this;
    return it;
}


image_process::iterator process_ewf::end()
{
    image_process::iterator it;
    it.raw_offset = this->ewf_filesize;
    it.myimage = this;
    it.eof = true;
    return it;
}

pos0_t process_ewf::get_pos0(const image_process::iterator &it) const
{
    pos0_t pos0;
    pos0.offset = it.raw_offset;
    return pos0;
}

/** Read from the iterator into a newly allocated sbuf */
sbuf_t *process_ewf::sbuf_alloc(image_process::iterator &it)
{
    int count = opt_pagesize + opt_margin;

    if(this->ewf_filesize < it.raw_offset + count){    /* See if that's more than I need */
	count = this->ewf_filesize - it.raw_offset;
    }

    unsigned char *buf = (unsigned char *)malloc(count);
    if(!buf) throw bad_alloc();			// no memory

    count = this->pread(buf,count,it.raw_offset); // do the read
    if(count<0){
	free(buf);
	throw read_error();
    }
    if(count==0){
	free(buf);
	it.eof = true;
	return 0;
    }

    sbuf_t *sbuf = new sbuf_t(get_pos0(it),buf,count,opt_pagesize,true);
    return sbuf;
}

void process_ewf::increment_iterator(image_process::iterator &it)
{
    it.raw_offset += opt_pagesize;
    if(it.raw_offset > this->ewf_filesize) it.raw_offset = this->ewf_filesize;
}

double process_ewf::fraction_done(class image_process::iterator &it)
{
    return (double)it.raw_offset / (double)this->ewf_filesize;
}

string process_ewf::str(class image_process::iterator &it)
{
    char buf[64];
    snprintf(buf,sizeof(buf),"Offset %"PRId64"MB",it.raw_offset/1000000);
    return string(buf);
}
#endif

/****************************************************************
 *** RAW
 ****************************************************************/

/**
 * process a raw, with the appropriate threading.
 */

#ifndef O_BINARY
#define O_BINARY 0
#endif

static bool fn_ends_with(const std::string &str,const std::string &suffix)
{
    if(suffix.size() > str.size()) return false;
    return str.substr(str.size()-suffix.size())==suffix;
}

static bool is_multipart_file(string fn)
{
    return fn_ends_with(fn,".000")
	|| fn_ends_with(fn,".001")
	|| fn_ends_with(fn,"001.vmdk");
}

static string make_list_template(string fn,int *start)
{
    /* First find where the digits are */
    size_t p = fn.rfind("000");
    if(p==string::npos) p = fn.rfind("001");
    assert(p!=string::npos);
    
    *start = atoi(fn.substr(p,3).c_str()) + 1;
    fn.replace(p,3,"%03d");	// make it a format
    return fn;
}


process_raw::process_raw(string image_fname_) :image_process(image_fname_),file_list(),
					       raw_filesize(0),current_file_name(),current_fd(-1) {
}

process_raw::~process_raw() {
    if(current_fd>0) ::close(current_fd);
}

/**
 * Add the file to the list, keeping track of the total size
 */
void process_raw::add_file(string fname)
{
    int fd = ::open(fname.c_str(),O_RDONLY|O_BINARY);
    if(fd<0){
	cerr << "*** Cannot open " << fname << ": " << strerror(errno) << "\n";
	exit(1);
    }
    int64_t fname_length = get_filesize(fd);
    ::close(fd);
    file_list.push_back(file_info(fname,raw_filesize,fname_length));
    raw_filesize += fname_length;
}

const class process_raw::file_info *process_raw::find_offset(int64_t pos) const
{
    for(process_raw::file_list_t::const_iterator it = file_list.begin();it != file_list.end();it++){
	if((*it).offset<=pos && pos< ((*it).offset+(*it).length)){
	    return &(*it);
	}
    }
    return 0;
}

/**
 * Open the first image and, optionally, all of the others.
 */
int process_raw::open()
{
    add_file(image_fname);

    /* Get the list of the files if this is a split-raw file */
    if(is_multipart_file(image_fname)){
	int num=0;
	string templ = make_list_template(image_fname,&num);
	for(;;num++){
	    char probename[PATH_MAX];
	    snprintf(probename,sizeof(probename),templ.c_str(),num); 
	    if(access(probename,R_OK)!=0) break;	    // no more files
	    add_file(string(probename)); // found another name
	}
    }
    return 0;
}

int64_t process_raw::image_size()
{
    return raw_filesize;
}


/**
 * Read randomly between a split file.
 * 1. Determine which file to read and how many bytes from that file can be read.
 * 2. Perform the read.
 * 3. If there are additional files to read in the next file, recurse.
 */

/* Create a pread64 if we are on Windows */
#if !defined(HAVE_PREAD64) && !defined(HAVE_PREAD) && defined(HAVE__LSEEKI64)
static size_t pread64(int d,void *buf,size_t nbyte,int64_t offset)
{
    if(_lseeki64(d,offset,0)!=offset) return -1;
    return read(d,buf,nbyte);
}
#endif

int process_raw::pread(unsigned char *buf,size_t bytes,int64_t offset) const
{
    const class file_info *fi = find_offset(offset);
    if(fi==0) return 0;			// nothing to read.

    /* See if the file is the one that's currently opened.
     * If not, close the current one and open the new one.
     */

    if(fi->name != current_file_name){
	if(current_fd>=0) close(current_fd);
	current_file_name = fi->name;
	current_fd = ::open(fi->name.c_str(),O_RDONLY|O_BINARY);
	if(current_fd<=0) return -1;	// can't read this data
    }

#if defined(HAVE_PREAD64)
    /* If we have pread64, make sure it is defined */
    extern size_t pread64(int fd,char *buf,size_t nbyte,off_t offset);
#endif

#if !defined(HAVE_PREAD64) && defined(HAVE_PREAD)
    /* if we are not using pread64, make sure that off_t is 8 bytes in size */
#define pread64(d,buf,nbyte,offset) pread(d,buf,nbyte,offset)
#endif

    /* we have neither, so just hack it with lseek64 */

    assert(fi->offset <= offset);
    ssize_t bytes_read = ::pread64(current_fd,buf,bytes,offset - fi->offset);
    if(bytes_read<0) return -1;		// error???
    if((size_t)bytes_read==bytes) return bytes_read; // read precisely the correct amount!

    /* Need to recurse */
    ssize_t bytes_read2 = this->pread(buf+bytes_read,bytes-bytes_read,offset+bytes_read);
    if(bytes_read2<0) return -1;	// error on second read
    if(bytes_read==0) return 0;		// kind of odd.

    return bytes_read + bytes_read2;
}


image_process::iterator process_raw::begin()
{
    image_process::iterator it;
    it.myimage = this;
    return it;
}


/* Returns an iterator at the end of the image */
image_process::iterator process_raw::end()
{
    image_process::iterator it;
    it.raw_offset = this->raw_filesize;
    it.myimage = this;
    it.eof = true;
    return it;
}


void process_raw::increment_iterator(image_process::iterator &it)
{
    it.raw_offset += opt_pagesize;
    if(it.raw_offset > this->raw_filesize) it.raw_offset = this->raw_filesize;
}

double process_raw::fraction_done(class image_process::iterator &it)
{
    return (double)it.raw_offset / (double)this->raw_filesize;
}

string process_raw::str(class image_process::iterator &it)
{
    char buf[64];
    snprintf(buf,sizeof(buf),"Offset %"PRId64"MB",it.raw_offset/1000000);
    return string(buf);
}


pos0_t process_raw::get_pos0(const image_process::iterator &it) const
{
    pos0_t pos0;
    pos0.offset = it.raw_offset;
    return pos0;
}

/** Read from the iterator into a newly allocated sbuf */
sbuf_t *process_raw::sbuf_alloc(image_process::iterator &it)
{
    int count = opt_pagesize + opt_margin;

    if(this->raw_filesize < it.raw_offset + count){    /* See if that's more than I need */
	count = this->raw_filesize - it.raw_offset;
    }
    unsigned char *buf = (unsigned char *)malloc(count);
    if(!buf) throw bad_alloc();			// no memory
    count = this->pread(buf,count,it.raw_offset); // do the read
    if(count==0){
	free(buf);
	it.eof = true;
	return 0;
    }
    if(count<0){
	free(buf);
	throw read_error();
    }
    sbuf_t *sbuf = new sbuf_t(get_pos0(it),buf,count,opt_pagesize,true);
    return sbuf;
}

static std::string filename_extension(std::string fn)
{
    size_t dotpos = fn.rfind('.');
    if(dotpos==std::string::npos) return "";
    return fn.substr(dotpos+1);
}


/****************************************************************
 *** Directory Recursion
 ****************************************************************/

process_dir::process_dir(const std::string &image_dir):image_process(image_dir),files()
{
    /* Use dig to get a list of all the files */
    dig d(image_dir);
    for(dig::const_iterator it=d.begin();it!=d.end();++it){
#ifdef WIN32
	std::string fn = safe_utf16to8(*it);
	if(ends_with(fn,"/.")) continue;
	files.push_back(fn);
#else
	files.push_back(*it);
#endif
    }
}

process_dir::~process_dir()
{
}

int process_dir::open()
{
    return 0;				// always successful
}

int process_dir::pread(unsigned char *buf,size_t bytes,int64_t offset) const
{
    err(1,"process_dir does not support pread");
}

int64_t process_dir::image_size()
{
    return files.size();		// the 'size' is in files
}

image_process::iterator process_dir::begin()
{
    image_process::iterator it;
    it.myimage = this;
    return it;
}

image_process::iterator process_dir::end()
{
    image_process::iterator it;
    it.myimage = this;
    it.file_number = files.size();
    it.eof = true;
    return it;
}

void process_dir::increment_iterator(image_process::iterator &it)
{
    it.file_number++;
    if(it.file_number>files.size()) it.file_number=files.size();
}

pos0_t process_dir::get_pos0(const image_process::iterator &it) const
{
    pos0_t pos0;
    pos0.path = files[it.file_number];
    pos0.offset = 0;
    return pos0;
}

/** Read from the iterator into a newly allocated sbuf
 * with mapped memory.
 */
sbuf_t *process_dir::sbuf_alloc(image_process::iterator &it)
{
    std::string fn = files[it.file_number];
    sbuf_t *sbuf = sbuf_t::map_file(fn,pos0_t(fn+sbuf_t::U10001C));
    if(sbuf==0) throw read_error();	// can't read
    return sbuf;
}

double process_dir::fraction_done(class image_process::iterator &it)
{
    return (double)it.file_number / (double)files.size();
}

string process_dir::str(class image_process::iterator &it)
{
    return string("File ")+files[it.file_number];
}



/****************************************************************
 *** COMMON
 ****************************************************************/

#include <functional>
#include <locale>
image_process *image_process_open(string fn,int opt_recurse)
{
    image_process *ip = 0;
    string ext = filename_extension(fn);
    struct stat st;

    if(stat(fn.c_str(),&st)){
	return 0;			// no file?
    }
    if(S_ISDIR(st.st_mode)){
	/* If this is a directory, process specially */
	if(opt_recurse==0){
	    std::cerr << "error: " << fn << " is a directory but -R (opt_recurse) not set\n";
	    errno = 0;
	    return 0;	// directory and cannot recurse
	}
	ip = new process_dir(fn);
    }
    else {
	/* Otherwise open a file by checking extension.
	 *
	 * I would rather use the localized version at
	 * http://stackoverflow.com/questions/313970/stl-string-to-lower-case
	 * but it generates a compile-time error.
	 */

	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	
	if(ext=="aff"){
#ifdef HAVE_LIBAFFLIB
	    ip = new process_aff(fn);
#else
	    cerr << "This program was compiled without AFF support\n";
	    exit(0);
#endif
	}
	if(ext=="e01"){
#ifdef HAVE_LIBEWF
	    ip = new process_ewf(fn);
#else
	    cerr << "This program was compiled without E01 support\n";
	    exit(1);
#endif
	}
	if(!ip) ip = new process_raw(fn);
    }
    /* Try to open it */
    if(ip->open()){
	errx(1,"Cannot open %s",fn.c_str());
    }
    return ip;
}

