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

#include "config.h"
#include "bulk_extractor.h"
#include "dig.h"
#include "utf8.h"

#ifdef HAVE_LIBAFFLIB
#endif

#include "image_process.h"
#ifdef HAVE_LIBAFFLIB
#ifndef HAVE_STL
#define HAVE_STL			/* needed */
#endif
#include <afflib/utils.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 65536
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif 

#ifndef MAX
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#endif 


/****************************************************************
 *** get_filesize()
 ****************************************************************/

/**
 * It's hard to figure out the filesize in an opearting system independent method that works with both
 * files and devices. This seems to work. It only requires a functioning pread64 or pread.
 */

#ifdef WIN32
int pread64(HANDLE current_handle,char *buf,size_t bytes,uint64_t offset)
{
    DWORD bytes_read = 0;
    LARGE_INTEGER li;
    li.QuadPart = offset;
    li.LowPart = SetFilePointer(current_handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if(li.LowPart == INVALID_SET_FILE_POINTER) return -1;
    if (FALSE == ReadFile(current_handle, buf, (DWORD) bytes, &bytes_read, NULL)){
        return -1;
    }
    return bytes_read;
}
#else
  #if !defined(HAVE_PREAD64) && !defined(HAVE_PREAD) && defined(HAVE__LSEEKI64)
static size_t pread64(int d,void *buf,size_t nbyte,int64_t offset)
{
    if(_lseeki64(d,offset,0)!=offset) return -1;
    return read(d,buf,nbyte);
}
  #endif
#endif

#ifdef WIN32
int64_t get_filesize(HANDLE fd)
#else
int64_t get_filesize(int fd)
#endif
{
    char buf[64];
    int64_t raw_filesize = 0;		/* needs to be signed for lseek */
    int bits = 0;
    int i =0;

#if defined(HAVE_PREAD64)
    /* If we have pread64, make sure it is defined */
    extern size_t pread64(int fd,char *buf,size_t nbyte,off_t offset);
#endif

#if !defined(HAVE_PREAD64) && defined(HAVE_PREAD)
    /* if we are not using pread64, make sure that off_t is 8 bytes in size */
#define pread64(d,buf,nbyte,offset) pread(d,buf,nbyte,offset)
    if(sizeof(off_t)!=8){
	err(1,"Compiled with off_t==%d and no pread64 support.",(int)sizeof(off_t));
    }
#endif

#ifndef WIN32
    /* We can use fstat if sizeof(st_size)==8 and st_size>0 */
    struct stat st;
    memset(&st,0,sizeof(st));
    if(sizeof(st.st_size)==8 && fstat(fd,&st)==0){
	if(st.st_size>0) return st.st_size;
    }
#endif

    /* Phase 1; figure out how far we can seek... */
    for(bits=0;bits<60;bits++){
	raw_filesize = ((int64_t)1<<bits);
	if(::pread64(fd,buf,1,raw_filesize)!=1){
	    break;
	}
    }
    if(bits==60) errx(1,"Partition detection not functional.\n");

    /* Phase 2; blank bits as necessary */
    for(i=bits;i>=0;i--){
	int64_t test = (int64_t)1<<i;
	int64_t test_filesize = raw_filesize | ((int64_t)1<<i);
	if(::pread64(fd,buf,1,test_filesize)==1){
	    raw_filesize |= test;
	} else{
	    raw_filesize &= ~test;
	}
    }
    if(raw_filesize>0) raw_filesize+=1;	/* seems to be needed */
    return raw_filesize;
}

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int64_t getSizeOfFile(const std::string &fname)
{
#ifdef WIN32
    HANDLE current_handle = CreateFileA(fname.c_str(), FILE_READ_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				     OPEN_EXISTING, 0, NULL);
    if(current_handle==INVALID_HANDLE_VALUE){
        fprintf(stderr,"bulk_extractor WIN32 subsystem: cannot open file '%s'\n",fname.c_str());
        return 0;
    }
    int64_t fname_length = get_filesize(current_handle);
    ::CloseHandle(current_handle);
#else
    int fd = ::open(fname.c_str(),O_RDONLY|O_BINARY);
    if(fd<0){
        std::cerr << "*** unix getSizeOfFile: Cannot open " << fname << ": " << strerror(errno) << "\n";
        return 0;
    }
    int64_t fname_length = get_filesize(fd);
    ::close(fd);
#endif
    return fname_length;
}



/****************************************************************
 *** AFF START
 ****************************************************************/

#ifdef HAVE_LIBAFFLIB
int process_aff::open()
{
    const char *fn = image_fname().c_str();
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

int64_t process_aff::image_size() const
{
    return af_get_imagesize(af);
}


/**
 * Iterator support
 */

image_process::iterator process_aff::begin() const
{
    image_process::iterator it(this);
    it.raw_offset = 0;
    return it;
}

image_process::iterator process_aff::end() const
{
    image_process::iterator it(this);
    it.page_counter = pagelist.size();
    it.raw_offset = af_get_imagesize(af);
    it.eof = true;
    return it;
}

/* Note - af_get_pagesize() used to be in afflib_i.h; it was moved,
 * but we may not have the new definition.
 */
#ifdef HAVE_LIBAFFLIB
__BEGIN_DECLS
#ifdef HAVE_DIAGNOSTIC_REDUNDANT_DECLS
#pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
int	af_get_pagesize(AFFILE *af);	// returns page size, or -1
__END_DECLS
#endif


/* Increment the AFF iterator by going to the next page.
 * If we hit the end of the pagelist, note that we are at the end of file.
 */
void process_aff::increment_iterator(class image_process::iterator &it) const
{
    if(it.page_counter < pagelist.size()){
	it.page_counter++;
	it.raw_offset = pagelist[it.page_counter] * af_get_pagesize(af);
    } else {
	it.eof = true;
    }
}

pos0_t process_aff::get_pos0(const image_process::iterator &it) const
{
    int64_t pagenum = pagelist[it.page_counter];
    return pos0_t("",pagenum * af_get_pagesize(af));
}

sbuf_t *process_aff::sbuf_alloc(image_process::iterator &it) const
{
    size_t bufsize  = af_get_pagesize(af)+margin;
    unsigned char *buf = (unsigned char *)malloc(bufsize);
    if(!buf) throw std::bad_alloc();

    pos0_t pos0 = get_pos0(it);
    
    af_seek(af,pos0.offset,0);
    ssize_t bytes_read = af_read(af,buf,bufsize);
    /**
     * af_read() returns 0 at end of file, if no data is available,
     * or if the data is bad. We need to be willing to return an sbuf
     * with zero bytes, which we do below.
     */
    if(bytes_read>=0){
	ssize_t af_pagesize = af_get_pagesize(af);
	if(af_pagesize>bytes_read) af_pagesize = bytes_read;
	sbuf_t *sbuf = new sbuf_t(pos0,buf,bytes_read,af_pagesize,true);
	return sbuf;
    }
    free(buf);
    return 0;				// no buffer to return
}

double process_aff::fraction_done(const image_process::iterator &it) const
{
    return (double)it.page_counter / (double)pagelist.size();
}

std::string process_aff::str(const image_process::iterator &it) const
{
    char buf[64];
    snprintf(buf,sizeof(buf),"Page %" PRId64 "",it.page_counter);
    return std::string(buf);
}

uint64_t process_aff::max_blocks(const image_process::iterator &it) const
{
    return pagelist.size();
}

uint64_t process_aff::seek_block(image_process::iterator &it,uint64_t block) const
{
    it.page_counter = block;
    return block;
}


process_aff::~process_aff()
{
    if(af) af_close(af);
}
#endif


/****************************************************************
 *** AFF END
 ****************************************************************/



/****************************************************************
 *** EWF START
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
#ifdef HAVE_LIBEWF_HANDLE_CLOSE
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

#ifdef WIN32
static std::string utf16to8(const std::wstring &fn16)
{
    std::string fn8;
    utf8::utf16to8(fn16.begin(),fn16.end(),back_inserter(fn8));
    return fn8;
}

static std::wstring utf8to16(const std::string &fn8)
{
    std::wstring fn16;
    utf8::utf8to16(fn8.begin(),fn8.end(),back_inserter(fn16));
    return fn16;
}
#endif

void local_e01_glob(const std::string &fname,char ***libewf_filenames,int *amount_of_filenames)
{
    std::cerr << "Experimental code for E01 names with MD5s appended\n";
#ifdef WIN32
    /* Find the directory name */
    std::string dirname(fname);
    size_t pos = dirname.rfind("\\");                  // this this slash
    if(pos==std::string::npos) pos=dirname.rfind("/"); // try the other slash!
    if(pos!=std::string::npos){
        dirname.resize(pos+1);          // remove what's after the 
    } else {
        dirname = "";                   // no directory?
    }

    /* Make the directory search template */
    char *buf = (char *)malloc(fname.size()+16);
    strcpy(buf,fname.c_str());
    /* Find the E01 */
    char *cc = strstr(buf,".E01.");
    if(!cc){
        err(1,"Cannot find .E01. in filename");
    }
    for(;*cc;cc++){
        if(*cc!='.') *cc='?';          // replace the E01 and the MD5s at the end with ?s
    }
    std::wstring wbufstring = utf8to16(buf); // convert to utf16
    const wchar_t *wbuf = wbufstring.c_str();

    /* Find the files */
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile(wbuf, &FindFileData);
    if(hFind == INVALID_HANDLE_VALUE){
        std::cerr << "Invalid file pattern: " << utf16to8(wbufstring) << "\n";
        exit(1);
    }
    std::vector<std::string> files;
    files.push_back(dirname + utf16to8(FindFileData.cFileName));
    while(FindNextFile(hFind,&FindFileData)!=0){
        files.push_back(dirname + utf16to8(FindFileData.cFileName));
    }

    /* Sort the files */
    sort(files.begin(),files.end());
    
    /* Make the array */
    *amount_of_filenames = files.size();
    *libewf_filenames = (char **)calloc(sizeof(char *),files.size());
    for(size_t i=0;i<files.size();i++){
        (*libewf_filenames)[i] = strdup(files[i].c_str());
    }
    free((void *)buf);
#else
    std::cerr << "This code only runs on Windows.\n";
#endif    
}

int process_ewf::open()
{
    const char *fname = image_fname().c_str();
    char **libewf_filenames = NULL;
    int amount_of_filenames = 0;

#ifdef HAVE_LIBEWF_HANDLE_CLOSE
    bool use_libewf_glob = true;
    libewf_error_t *error=0;
    if(image_fname().find(".E01.")!=std::string::npos){
        use_libewf_glob = false;
    }

    if(use_libewf_glob){
        if(libewf_glob(fname,strlen(fname),LIBEWF_FORMAT_UNKNOWN,
                       &libewf_filenames,&amount_of_filenames,&error)<0){
            libewf_error_fprint(error,stdout);
            libewf_error_free(&error);
            err(1,"libewf_glob");
        }
    } else {
        local_e01_glob(image_fname(),&libewf_filenames,&amount_of_filenames);
        std::cerr << "amount of filenames=" << amount_of_filenames << "\n";
        for(int i=0;i<amount_of_filenames;i++){
            std::cerr << libewf_filenames[i] << "\n";
        }
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
    /* Free the allocated filenames */
    if(use_libewf_glob){
        if(libewf_glob_free(libewf_filenames,amount_of_filenames,&error)<0){
            printf("libewf_glob_free failed\n");
            if(error) libewf_error_fprint(error,stdout);
            err(1,"libewf_glob_free");
        }
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

std::vector<std::string> process_ewf::getewfdetails() const{
    return(details);
}


//int process_ewf::debug = 0;
int process_ewf::pread(unsigned char *buf,size_t bytes,int64_t offset) const
{
#ifdef HAVE_LIBEWF_HANDLE_CLOSE
    libewf_error_t *error=0;
#if defined(HAVE_LIBEWF_HANDLE_READ_RANDOM)
    int ret = libewf_handle_read_random(handle,buf,bytes,offset,&error);
#endif
#if defined(HAVE_LIBEWF_HANDLE_READ_BUFFER_AT_OFFSET) && !defined(HAVE_LIBEWF_HANDLE_READ_RANDOM)
    int ret = libewf_handle_read_buffer_at_offset(handle,buf,bytes,offset,&error);
#endif
    if(ret<0){
	if (report_read_errors) libewf_error_fprint(error,stderr);
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


int64_t process_ewf::image_size() const
{
    return ewf_filesize;
}


image_process::iterator process_ewf::begin() const
{
    image_process::iterator it(this);
    it.raw_offset = 0;
    return it;
}


image_process::iterator process_ewf::end() const
{
    image_process::iterator it(this);
    it.raw_offset = this->ewf_filesize;
    it.eof = true;
    return it;
}

pos0_t process_ewf::get_pos0(const image_process::iterator &it) const
{
    return pos0_t("",it.raw_offset);
}

/** Read from the iterator into a newly allocated sbuf */
sbuf_t *process_ewf::sbuf_alloc(image_process::iterator &it) const
{
    int count = pagesize + margin;

    if(this->ewf_filesize < it.raw_offset + count){    /* See if that's more than I need */
	count = this->ewf_filesize - it.raw_offset;
    }

    unsigned char *buf = (unsigned char *)malloc(count);
    if(!buf) throw std::bad_alloc();			// no memory

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

    sbuf_t *sbuf = new sbuf_t(get_pos0(it),buf,count,pagesize,true);
    return sbuf;
}

/**
 * just add the page size for process_ewf
 */
void process_ewf::increment_iterator(image_process::iterator &it) const
{
    it.raw_offset += pagesize;
    if(it.raw_offset > this->ewf_filesize) it.raw_offset = this->ewf_filesize;
}

double process_ewf::fraction_done(const image_process::iterator &it) const
{
    return (double)it.raw_offset / (double)this->ewf_filesize;
}

std::string process_ewf::str(const image_process::iterator &it) const
{
    char buf[64];
    snprintf(buf,sizeof(buf),"Offset %" PRId64 "MB",it.raw_offset/1000000);
    return std::string(buf);
}

uint64_t process_ewf::max_blocks(const image_process::iterator &it) const
{
  return this->ewf_filesize / pagesize;
}

uint64_t process_ewf::seek_block(image_process::iterator &it,uint64_t block) const
{
    it.raw_offset = pagesize * block;
    return block;
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

static bool is_multipart_file(const std::string &fn)
{
    return fn_ends_with(fn,".000")
	|| fn_ends_with(fn,".001")
	|| fn_ends_with(fn,"001.vmdk");
}

/* fn can't be & because it will get modified */
static std::string make_list_template(std::string fn,int *start)
{
    /* First find where the digits are */
    size_t p = fn.rfind("000");
    if(p==std::string::npos) p = fn.rfind("001");
    assert(p!=std::string::npos);
    
    *start = atoi(fn.substr(p,3).c_str()) + 1;
    fn.replace(p,3,"%03d");	// make it a format
    return fn;
}


process_raw::process_raw(const std::string &fname,size_t pagesize_,size_t margin_)
    :image_process(fname,pagesize_,margin_),
     file_list(),raw_filesize(0),current_file_name(),
#ifdef WIN32
     current_handle(INVALID_HANDLE_VALUE)
#else
     current_fd(-1)
#endif
{
}

process_raw::~process_raw() {
#ifdef WIN32
    if(current_handle!=INVALID_HANDLE_VALUE) ::CloseHandle(current_handle);
#else
    if(current_fd>0) ::close(current_fd);
#endif
}

#ifdef WIN32
BOOL GetDriveGeometry(const wchar_t *wszPath, DISK_GEOMETRY *pdg)
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
    BOOL bResult   = FALSE;                 // results flag
    DWORD junk     = 0;                     // discard results

    hDevice = CreateFileW(wszPath,          // drive to open
                          0,                // no access to the drive
                          FILE_SHARE_READ | // share mode
                          FILE_SHARE_WRITE, 
                          NULL,             // default security attributes
                          OPEN_EXISTING,    // disposition
                          0,                // file attributes
                          NULL);            // do not copy file attributes

    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
        {
            return (FALSE);
        }

    bResult = DeviceIoControl(hDevice,                       // device to be queried
                              IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
                              NULL, 0,                       // no input buffer
                              pdg, sizeof(*pdg),            // output buffer
                              &junk,                         // # bytes returned
                              (LPOVERLAPPED) NULL);          // synchronous I/O

    CloseHandle(hDevice);

    return (bResult);
}

#endif

/**
 * Add the file to the list, keeping track of the total size
 */
void process_raw::add_file(const std::string &fname)
{
    int64_t fname_length = 0;
  
    /* Get the physical size of drive under Windows */
    fname_length = getSizeOfFile(fname);
#ifdef WIN32
    if (fname_length==0){
        /* On Windows, see if we can use this */
        fprintf(stderr,"%s checking physical drive\n",fname.c_str());
        // http://msdn.microsoft.com/en-gb/library/windows/desktop/aa363147%28v=vs.85%29.aspx
        DISK_GEOMETRY pdg = { 0 }; // disk drive geometry structure
        std::wstring wszDrive = safe_utf8to16(fname);
        GetDriveGeometry(wszDrive.c_str(), &pdg);
        fname_length = pdg.Cylinders.QuadPart * (ULONG)pdg.TracksPerCylinder *
            (ULONG)pdg.SectorsPerTrack * (ULONG)pdg.BytesPerSector;
    }
#endif
    file_list.push_back(file_info(fname,raw_filesize,fname_length));
    raw_filesize += fname_length;
}

const process_raw::file_info *process_raw::find_offset(int64_t pos) const
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
    add_file(image_fname());

    /* Get the list of the files if this is a split-raw file */
    if(is_multipart_file(image_fname())){
	int num=0;
        std::string templ = make_list_template(image_fname(),&num);
	for(;;num++){
	    char probename[PATH_MAX];
	    snprintf(probename,sizeof(probename),templ.c_str(),num); 
	    if(access(probename,R_OK)!=0) break;	    // no more files
	    add_file(std::string(probename)); // found another name
	}
    }
    return 0;
}

int64_t process_raw::image_size() const
{
    return raw_filesize;
}


/**
 * Read randomly between a split file.
 * 1. Determine which file to read and how many bytes from that file can be read.
 * 2. Perform the read.
 * 3. If there are additional files to read in the next file, recurse.
 */

int process_raw::pread(unsigned char *buf,size_t bytes,int64_t offset) const
{
    const file_info *fi = find_offset(offset);
    if(fi==0) return 0;			// nothing to read.

    /* See if the file is the one that's currently opened.
     * If not, close the current one and open the new one.
     */

    if(fi->name != current_file_name){
#ifdef WIN32
        if(current_handle!=INVALID_HANDLE_VALUE) ::CloseHandle(current_handle);
#else
	if(current_fd>=0) close(current_fd);
#endif

	current_file_name = fi->name;
	fprintf(stderr,"Attempt to open %s\n",fi->name.c_str());
#ifdef WIN32
        current_handle = CreateFileA(fi->name.c_str(), FILE_READ_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				     OPEN_EXISTING, 0, NULL);
        if(current_handle==INVALID_HANDLE_VALUE){
	  fprintf(stderr,"bulk_extractor WIN32 subsystem: cannot open file '%s'\n",fi->name.c_str());
	  return -1;
	}
#else        
	current_fd = ::open(fi->name.c_str(),O_RDONLY|O_BINARY);
	if(current_fd<=0) return -1;	// can't read this data
#endif
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
#ifdef WIN32
    DWORD bytes_read = 0;
    LARGE_INTEGER li;
    li.QuadPart = offset - fi->offset;
    li.LowPart = SetFilePointer(current_handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if(li.LowPart == INVALID_SET_FILE_POINTER) return -1;
    if (FALSE == ReadFile(current_handle, buf, (DWORD) bytes, &bytes_read, NULL)){
        return -1;
    }
#else
    ssize_t bytes_read = ::pread64(current_fd,buf,bytes,offset - fi->offset);
#endif
    if(bytes_read<0) return -1;		// error???
    if((size_t)bytes_read==bytes) return bytes_read; // read precisely the correct amount!

    /* Need to recurse */
    ssize_t bytes_read2 = this->pread(buf+bytes_read,bytes-bytes_read,offset+bytes_read);
    if(bytes_read2<0) return -1;	// error on second read
    if(bytes_read==0) return 0;		// kind of odd.

    return bytes_read + bytes_read2;
}


image_process::iterator process_raw::begin() const
{
    image_process::iterator it(this);
    return it;
}


/* Returns an iterator at the end of the image */
image_process::iterator process_raw::end() const
{
    image_process::iterator it(this);
    it.raw_offset = this->raw_filesize;
    it.eof = true;
    return it;
}

void process_raw::increment_iterator(image_process::iterator &it) const
{
    it.raw_offset += pagesize;
    if(it.raw_offset > this->raw_filesize) it.raw_offset = this->raw_filesize;
}

double process_raw::fraction_done(const image_process::iterator &it) const
{
    return (double)it.raw_offset / (double)this->raw_filesize;
}

std::string process_raw::str(const image_process::iterator &it) const
{
    char buf[64];
    snprintf(buf,sizeof(buf),"Offset %" PRId64 "MB",it.raw_offset/1000000);
    return std::string(buf);
}


pos0_t process_raw::get_pos0(const image_process::iterator &it) const
{
    return pos0_t("",it.raw_offset);
}

/** Read from the iterator into a newly allocated sbuf.
 * uses pagesize.
 */
sbuf_t *process_raw::sbuf_alloc(image_process::iterator &it) const
{
    int count = pagesize + margin;

    if(this->raw_filesize < it.raw_offset + count){    /* See if that's more than I need */
	count = this->raw_filesize - it.raw_offset;
    }
    unsigned char *buf = (unsigned char *)malloc(count);
    if(!buf) throw std::bad_alloc();			// no memory
    count = this->pread(buf,count,it.raw_offset);       // do the read
    if(count==0){
	free(buf);
	it.eof = true;
	return 0;
    }
    if(count<0){
	free(buf);
	throw read_error();
    }
    sbuf_t *sbuf = new sbuf_t(get_pos0(it),buf,count,pagesize,true);
    return sbuf;
}

static std::string filename_extension(std::string fn)
{
    size_t dotpos = fn.rfind('.');
    if(dotpos==std::string::npos) return "";
    return fn.substr(dotpos+1);
}

uint64_t process_raw::max_blocks(const image_process::iterator &it) const
{
    return (this->raw_filesize+pagesize-1) / pagesize;
}

uint64_t process_raw::seek_block(image_process::iterator &it,uint64_t block) const
{
    if(block * pagesize > (uint64_t)raw_filesize){
        block = raw_filesize / pagesize;
    }

    it.raw_offset = block * pagesize;
    return block;
}


/****************************************************************
 *** Directory Recursion
 ****************************************************************/

/**
 * directories don't get page sizes or margins; the page size is the entire
 * file and the margin is 0.
 */
process_dir::process_dir(const std::string &image_dir):
    image_process(image_dir,0,0),files()
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

int64_t process_dir::image_size() const
{
    return files.size();		// the 'size' is in files
}

image_process::iterator process_dir::begin() const
{
    image_process::iterator it(this);
    return it;
}

image_process::iterator process_dir::end() const
{
    image_process::iterator it(this);
    it.file_number = files.size();
    it.eof = true;
    return it;
}

void process_dir::increment_iterator(image_process::iterator &it) const
{
    it.file_number++;
    if(it.file_number>files.size()) it.file_number=files.size();
}

pos0_t process_dir::get_pos0(const image_process::iterator &it) const
{
    return pos0_t(files[it.file_number],0);
}

/** Read from the iterator into a newly allocated sbuf
 * with mapped memory.
 */
sbuf_t *process_dir::sbuf_alloc(image_process::iterator &it) const
{
    std::string fname = files[it.file_number];
    sbuf_t *sbuf = sbuf_t::map_file(fname);
    if(sbuf==0) throw read_error();	// can't read
    return sbuf;
}

double process_dir::fraction_done(const image_process::iterator &it) const
{
    return (double)it.file_number / (double)files.size();
}

std::string process_dir::str(const image_process::iterator &it) const
{
    return std::string("File ")+files[it.file_number];
}


uint64_t process_dir::max_blocks(const image_process::iterator &it) const
{
    return files.size();
}

uint64_t process_dir::seek_block(class image_process::iterator &it,uint64_t block) const
{
    it.file_number = block;
    return it.file_number;
}




/****************************************************************
 *** COMMON - Implement 'open' for the iterator
 ****************************************************************/

#include <functional>
#include <locale>
image_process *image_process::open(std::string fn,bool opt_recurse,
                                   size_t pagesize_,size_t margin_)
{
    image_process *ip = 0;
    std::string ext = filename_extension(fn);
    struct stat st;
    bool  is_windows_unc = false;

#ifdef WIN32
    if(fn.size()>2 && fn[0]=='\\' && fn[1]=='\\') is_windows_unc=true;
#endif

    memset(&st,0,sizeof(st));
    if(stat(fn.c_str(),&st) && !is_windows_unc){
	return 0;			// no file?
    }
    if(S_ISDIR(st.st_mode)){
	/* If this is a directory, process specially */
	if(opt_recurse==0){
	    std::cerr << "error: " << fn << " is a directory but -R (opt_recurse) not set\n";
	    errno = 0;
	    return 0;	// directory and cannot recurse
	}
        /* Quickly scan the directory and see if it has a .E01, .000 or .001 file.
         * If so, give the user an error.
         */
        DIR *dirp = opendir(fn.c_str());
        struct dirent *dp=0;
        if(!dirp){
            std::cerr <<"error: cannot open directory " << fn << ": " << strerror(errno) << "\n";
            errno=0;
            return 0;
        }
        while ((dp = readdir(dirp)) != NULL){
            if (strstr(dp->d_name,".E01") || strstr(dp->d_name,".000") || strstr(dp->d_name,".001")){
                std::cerr << "error: file " << dp->d_name << " is in directory " << fn << "\n";
                std::cerr << "       The -R option is not for reading a directory of EnCase files\n";
                std::cerr << "       or a directory of disk image parts. Please process these\n";
                std::cerr << "       as a single disk image. If you need to process these files\n";
                std::cerr << "       then place them in a sub directory of " << fn << "\n";
                errno=0;
                closedir(dirp);
                return 0;
            }
        }
        closedir(dirp);
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
	    ip = new process_aff(fn,pagesize_,margin_);
#else
            std::cerr << "This program was compiled without AFF support\n";
	    exit(1);
#endif
	}
	if(ext=="e01" || fn.find(".E01.")!=std::string::npos){
#ifdef HAVE_LIBEWF
	    ip = new process_ewf(fn,pagesize_,margin_);
#else
            std::cerr << "This program was compiled without E01 support\n";
	    exit(1);
#endif
	}
	if(!ip) ip = new process_raw(fn,pagesize_,margin_);
    }
    /* Try to open it */
    if(ip->open()){
	fprintf(stderr,"Cannot open %s",fn.c_str());
        return 0;
    }
    return ip;
}

