/**
 * image_process.cpp:
 *
 * Implements:
 *   - process_ewf (if libewf is installed)
 *   - process_raw (using std::iostream's 64-bit support.
 *   - process_dir (for scanning files in a directory
 */

#include "config.h"

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 65536
#endif

#include <algorithm>
#include <stdexcept>
#include <functional>
#include <locale>
#include <string>
#include <vector>

#include "be13_api/utf8.h"
#include "be13_api/utils.h"
#include "be13_api/aftimer.h"
#include "be13_api/formatter.h"
#include "image_process.h"

/****************************************************************
 *** static functions
 ****************************************************************/

image_process::image_process(std::filesystem::path fn, size_t pagesize_, size_t margin_):
    image_fname_(fn),pagesize(pagesize_),margin(margin_),report_read_errors(true)
{
}

image_process::~image_process()
{
}


std::filesystem::path image_process::image_fname() const
{
    return image_fname_;
}



bool image_process::fn_ends_with(std::filesystem::path path, std::string suffix)
{
    std::string str(path.string());
    if (suffix.size() > str.size()) return false;
    return str.substr(str.size()-suffix.size())==suffix;
}

bool image_process::is_multipart_file(std::filesystem::path fn)
{
    return fn_ends_with(fn,".000")
	|| fn_ends_with(fn,".001")
	|| fn_ends_with(fn,"001.vmdk");
}

/* Given a disk image name, if it contains 000 or 001 replace that with a %03d to make an snprintf template */
std::string image_process::make_list_template(std::filesystem::path path_,int *start)
{
    /* First find where the digits are */
    std::string path(path_.string());
    size_t p = path.rfind("000");
    if (p==std::string::npos) p = path.rfind("001");
    assert(p!=std::string::npos);

    *start = atoi(path.substr(p,3).c_str()) + 1;
    path.replace(p,3,"%03d");	// make it a format
    return path;
}



/****************************************************************
 *** EWF START
 ****************************************************************/

/**
 * Works with both new libewf API ("experimental") and old libewf API
 */

#ifdef HAVE_LIBEWF
#ifdef libewf_handle_get_header_value_case_number
#define LIBEWFNG
#endif

/****************************************************************
 ** process_ewf
 */

#ifdef _WIN32
void process_ewf::local_e01_glob(std::filesystem::path fname,char ***libewf_filenames,int *amount_of_filenames)
{
    /* Find the directory name */
    std::string dirname(fname);
    size_t pos = dirname.rfind("\\");                  // this this slash
    if (pos==std::string::npos) pos=dirname.rfind("/"); // try the other slash!
    if (pos!=std::string::npos){
        dirname.resize(pos+1);          // remove what's after the
    } else {
        dirname = "";                   // no directory?
    }

    /* Make the directory search template */
    char *buf = (char *)malloc(fname.size()+16);
    strcpy(buf,fname.c_str());
    /* Find the E01 */
    char *cc = strstr(buf,".E01.");
    if (!cc){
        throw image_process::NoSuchFile("Cannot find .E01. in filename");
    }
    for(;*cc;cc++){
        if (*cc!='.') *cc='?';          // replace the E01 and the MD5s at the end with ?s
    }
    std::wstring wbufstring = safe_utf8to16(buf); // convert to utf16
    const wchar_t *wbuf = wbufstring.c_str();

    /* Find the files */
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile(wbuf, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE){
        throw std::runtime_error( Formatter() << "Invalid file pattern " << safe_utf16to8(wbufstring) );
    }
    std::vector<std::filesystem::path> files;
    files.push_back(dirname + safe_utf16to8(FindFileData.cFileName));
    while(FindNextFile(hFind,&FindFileData)!=0){
        files.push_back(dirname + safe_utf16to8(FindFileData.cFileName));
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
}
#endif


process_ewf::~process_ewf()
{
#ifdef HAVE_LIBEWF_HANDLE_CLOSE
    if (handle){
	libewf_handle_close(handle,NULL);
	libewf_handle_free(&handle,NULL);
    }
#else
    if (handle){
	libewf_close(handle);
    }
#endif
}


int process_ewf::open()
{
    std::filesystem::path fname = image_fname();
    std::string fname_string = fname.string();
    char **libewf_filenames = NULL;
    int amount_of_filenames = 0;

#ifdef HAVE_LIBEWF_HANDLE_CLOSE
    bool use_libewf_glob = true;
    libewf_error_t *error=0;

    if (fname_string.find(".E01")!=std::string::npos){
        use_libewf_glob = false;
    }

    if (use_libewf_glob){
        if (libewf_glob(fname.c_str(), strlen(fname.c_str()), LIBEWF_FORMAT_UNKNOWN,
                       &libewf_filenames,&amount_of_filenames,&error)<0){
            libewf_error_fprint(error,stdout);
            libewf_error_free(&error);
            throw std::invalid_argument("libewf_glob");
        }
    } else {
#ifdef _WIN32
        local_e01_glob(fname, &libewf_filenames,&amount_of_filenames);
        for(int i=0;i<amount_of_filenames;i++){
            std::cout << "opening " << libewf_filenames[i] << std::endl;
        }
#else
        throw std::runtime_error("process_ewf::open: cannot process ewf file.");
#endif
    }
    handle = 0;
    if (libewf_handle_initialize(&handle,NULL)<0){
	throw image_process::NoSuchFile("Cannot initialize EWF handle?");
    }
    if (libewf_handle_open(handle,libewf_filenames,amount_of_filenames,
			  LIBEWF_OPEN_READ,&error)<0){
	if (error) libewf_error_fprint(error, stderr);
        fflush(stderr);
        for(size_t i = 0; libewf_filenames[i]; i++){
            std::cout << "opening " << libewf_filenames[i] << std::endl;
        }
	throw image_process::NoSuchFile( fname.string() );
    }
    /* Free the allocated filenames */
    if (use_libewf_glob){
        if (libewf_glob_free(libewf_filenames,amount_of_filenames,&error)<0){
            printf("libewf_glob_free failed\n");
            if (error) libewf_error_fprint(error,stdout);
            throw image_process::NoSuchFile("libewf_glob_free");
        }
    }
    libewf_handle_get_media_size(handle,(size64_t *)&ewf_filesize,NULL);
#else
    amount_of_filenames = libewf_glob(fname,strlen(fname),LIBEWF_FORMAT_UNKNOWN,&libewf_filenames);
    if (amount_of_filenames<0){
	err(1,"libewf_glob");
    }
    handle = libewf_open(libewf_filenames,amount_of_filenames,LIBEWF_OPEN_READ);
    if (handle==0){
	fprintf(stderr,"amount_of_filenames:%d\n",amount_of_filenames);
	for(int i=0;i<amount_of_filenames;i++){
	    fprintf(stderr,"  %s\n",libewf_filenames[i]);
	}
	throw image_process::NoSuchFile("libewf_open");
    }
    libewf_get_media_size(handle,(size64_t *)&ewf_filesize);
#endif

#ifdef HAVE_LIBEWF_HANDLE_GET_UTF8_HEADER_VALUE_NOTES
    uint8_t ewfbuf[65536];
    int status= libewf_handle_get_utf8_header_value_notes(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if (status == 1 && strlen(ewfbuf)>0){
	std::string notes = reinterpret_cast<char *>(ewfbuf);
	details.push_back(std::string("NOTES: ")+notes);
    }

    status = libewf_handle_get_utf8_header_value_case_number(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if (status == 1 && strlen(ewfbuf)>0){
	std::string case_number = reinterpret_cast<char *>(ewfbuf);
	details.push_back(std::string("CASE NUMBER: ")+case_number);
    }

    status = libewf_handle_get_utf8_header_value_evidence_number(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if (status == 1 && strlen(ewfbuf)>0){
	std::string evidenceno = reinterpret_cast<char *>(ewfbuf);
	details.push_back(std::string("EVIDENCE NUMBER: ")+evidenceno);
    }

    status = libewf_handle_get_utf8_header_value_examiner_name(handle, ewfbuf, sizeof(ewfbuf)-1, &error);
    if (status == 1 && strlen(ewfbuf)>0){
	std::string examinername = reinterpret_cast<char *>(ewfbuf) ;
	details.push_back(std::string("EXAMINER NAME: "+examinername));
    }
#endif
    std::cout << "\r                                                                      "
              << std::endl;
    return 0;
}

std::vector<std::string> process_ewf::getewfdetails() const
{
    return(details);
}


ssize_t process_ewf::pread(void *buf,size_t bytes,uint64_t offset) const
{
#ifdef HAVE_LIBEWF_HANDLE_CLOSE
    libewf_error_t *error=0;
#if defined(HAVE_LIBEWF_HANDLE_READ_RANDOM)
    int ret = libewf_handle_read_random(handle,buf,bytes,offset,&error);
#endif
#if defined(HAVE_LIBEWF_HANDLE_READ_BUFFER_AT_OFFSET) && !defined(HAVE_LIBEWF_HANDLE_READ_RANDOM)
    int ret = libewf_handle_read_buffer_at_offset(handle,buf,bytes,offset,&error);
#endif
    if (ret<0){
	if (report_read_errors) libewf_error_fprint(error,stderr);
	libewf_error_free(&error);
    }
    return ret;
#else
    if ((int64_t)bytes+offset > (int64_t)ewf_filesize) {
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
    size_t count = pagesize + margin;
    size_t this_pagesize = pagesize;

    if (this->ewf_filesize < it.raw_offset + count){    /* See if that's more than I need */
	count = this->ewf_filesize - it.raw_offset;
    }

    if (this_pagesize > count ) {
        this_pagesize = count;
    }

    auto sbuf = sbuf_t::sbuf_malloc(get_pos0(it), count, this_pagesize);
    unsigned char *buf = static_cast<unsigned char *>(sbuf->malloc_buf());
    int count_read = this->pread(buf, count, it.raw_offset);
    if (count_read<0){
        delete sbuf;
	throw read_error();
    }
    if (count==0){
        delete sbuf;
	it.eof = true;
	return 0;
    }
    return sbuf;
}

/**
 * just add the page size for process_ewf
 */
void process_ewf::increment_iterator(image_process::iterator &it) const
{
    it.raw_offset += pagesize;
    if (it.raw_offset > this->ewf_filesize) it.raw_offset = this->ewf_filesize;
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

process_raw::process_raw(std::filesystem::path fname, size_t pagesize_, size_t margin_)
    :image_process(fname, pagesize_, margin_)
{
}

process_raw::~process_raw()
{
    file_list.clear();
}

/* If we are running on WIN32 and we've been asked to process a raw device, get its "Drive Geometry" to figure out how big it is.
 */
#ifdef _WIN32
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

    if (hDevice == INVALID_HANDLE_VALUE){    // cannot open the drive
        throw image_process::NoSuchFile("GetDriveGeometry: Cannot open drive");
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
 * https://docs.microsoft.com/en-us/windows/win32/devio/calling-deviceiocontrol
 */
void process_raw::add_file(std::filesystem::path path)
{
    int64_t path_filesize = std::filesystem::file_size(path);

#ifdef _WIN32
    if (path_filesize==0){
        /* On Windows, see if we can use this */
        std::stdout << path << " checking physical drive" << std::endl;
        DISK_GEOMETRY pdg = { 0 }; // disk drive geometry structure
        std::wstring wszDrive = safe_utf8to16(path.string());
        GetDriveGeometry(wszDrive.c_str(), &pdg);
        path_filesize = pdg.Cylinders.QuadPart
            * (ULONG)pdg.TracksPerCylinder
            * (ULONG)pdg.SectorsPerTrack
            * (ULONG)pdg.BytesPerSector;
    }
#endif
    file_list.push_back( std::shared_ptr<process_raw::file_info>(new file_info(path, raw_filesize, path_filesize)));
    raw_filesize += path_filesize;
}

/*
 * This currently performs a linear search through the file list. It is not efficient, but it is reliable.
 * We could use a clever data structure, but then we would need to debug it.
 */
const std::shared_ptr<process_raw::file_info> process_raw::find_offset(uint64_t pos) const
{
    for (const auto &it:file_list) {
	if (it->offset<=pos && pos<it->offset + it->length) {
	    return it;
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
    if (is_multipart_file(image_fname())){
	int num=0;
        std::string templ = make_list_template(image_fname(),&num);
	for(;;num++){
	    char probename[PATH_MAX];
	    snprintf(probename, sizeof(probename), templ.c_str(), num);
            std::filesystem::path probe_path = probename;
            // If the file exists, add it, otherwise break.
            if (std::filesystem::exists( std::filesystem::path( probename ))) {
                add_file(std::filesystem::path(probename));
            } else {
                break;
            }
	}
    }
    return 0;
}

/* Return the size of all the images */
int64_t process_raw::image_size() const
{
    return raw_filesize;
}


/**
 * Read randomly between a split file.
 * 1. Determine which file to read and how many bytes from that file can be read.
 * 2. Perform the read.
 * 3. If there are additional files to read in the next file, recurse.
 * NOTE: This code is single-threaded because we are seeking the fstreams.
 */

ssize_t process_raw::pread(void *buf, size_t bytes, uint64_t offset) const
{
    std::shared_ptr<file_info> fi = find_offset(offset);
    if (fi==0) return 0;			// nothing to read.

    // make sure that the offset falls within the selection.
    assert(offset >= fi->offset);
    assert(offset <  fi->offset + fi->length);

    // Determine the offset and available bytes in the segment
    uint64_t file_offset = offset - fi->offset;
    uint64_t available_bytes = fi->length - file_offset;

    size_t bytes_to_read = bytes;
    if (bytes_to_read > available_bytes) {
        bytes_to_read = available_bytes;
    }
#ifdef _DEBUG_
    std::cerr << fi->path
              << " pread bytes=" << bytes
              << " offset=" << offset
              << " file_offset=" << file_offset
              << " available_bytes=" << available_bytes
              << " bytes_to_read=" << bytes_to_read << std::endl;
#endif


    fi->stream.seekg( file_offset );
    if (fi->stream.rdstate() & (std::ios::failbit|std::ios::badbit)){
        throw SeekError();
    }

    aftimer t;
    t.start();
    fi->stream.read(reinterpret_cast<char *>(buf), bytes_to_read);
    t.stop();

    if (fi->stream.rdstate() & std::ios::failbit){
        std::cerr << "read error  failbit bytes=" << bytes << std::endl;
        throw ReadError();
    }
    if (fi->stream.rdstate() & std::ios::badbit){
        std::cerr << "read error  badbit bytes=" << bytes << std::endl;
        throw ReadError();
    }
    if (fi->stream.rdstate() & (std::ios::eofbit)){
        std::cerr << "read error  eof bytes=" << bytes << std::endl;
        throw EndOfImage();
    }

    size_t bytes_read = bytes_to_read;  // guess we got the right amount

    /* Need to recurse, which will cause the next segment to be loaded */
    if (bytes_read==bytes) return bytes_read; // read precisely the correct amount!
    if (bytes_read==0) return 0;              // This might happen at the end of the image; it prevents infinite recursion

    ssize_t bytes_read2 = this->pread(static_cast<char *>(buf)+bytes_read, bytes-bytes_read, offset+bytes_read);
    if (bytes_read2<0) return -1;	// error on second read
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

/****************************************************************
 ** process_raw
 ****************************************************************/

void process_raw::increment_iterator(image_process::iterator &it) const
{
    it.raw_offset += pagesize;
    if (it.raw_offset > this->raw_filesize) it.raw_offset = this->raw_filesize;
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
 * uses pagesize. We don't memory map the file. Perhaps we should. But then we could only do 4K pages.
 */
sbuf_t *process_raw::sbuf_alloc(image_process::iterator &it) const
{
    size_t count = pagesize + margin;
    size_t this_pagesize = pagesize;

    if (this->raw_filesize < it.raw_offset + count){    /* See if that's more than I need */
	count = this->raw_filesize - it.raw_offset;
    }
    if (this_pagesize > count ) {
        this_pagesize = count;
    }

    sbuf_t *sbuf = sbuf_t::sbuf_malloc( get_pos0(it), count, this_pagesize);
    unsigned char *buf = reinterpret_cast<unsigned char *>(sbuf->malloc_buf());
    int count_read = this->pread(buf, count, it.raw_offset);       // do the read
    if (count_read==0){
        delete sbuf;
	it.eof = true;
        throw EndOfImage();
    }
    if (count_read<0){
        delete sbuf;
	throw read_error();
    }
    return sbuf;
}

uint64_t process_raw::max_blocks(const image_process::iterator &it) const
{
    return (this->raw_filesize+pagesize-1) / pagesize;
}

uint64_t process_raw::seek_block(image_process::iterator &it,uint64_t block) const
{
    if (block * pagesize > (uint64_t)raw_filesize){
        block = raw_filesize / pagesize;
    }

    it.raw_offset = block * pagesize;
    return block;
}


/****************************************************************
 ** process_dir
 **/

/**
 * directories don't get page sizes or margins; the page size is the entire
 * file and the margin is 0.
 */
process_dir::process_dir(std::filesystem::path image_dir): image_process(image_dir,0,0)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator( image_dir )) {
        if (entry.is_regular_file()) {
            files.push_back( entry );
        }
    }
}

process_dir::~process_dir()
{
}

int process_dir::open()
{
    return 0;				// always successful
}

ssize_t process_dir::pread(void *buf,size_t bytes,uint64_t offset) const
{
    if (bytes>0) {
        throw std::runtime_error("process_dir does not support pread");
    }
    return 0;
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
    if (it.file_number>files.size()) it.file_number=files.size();
}

//#ifdef HAVE_DIAGNOSTIC_SUGGEST_ATTRIBUTE_NORETURN
//#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
//#endif
pos0_t process_dir::get_pos0(const image_process::iterator &it) const
{
    return pos0_t(files[it.file_number].string(), 0);
}
//#ifdef HAVE_DIAGNOSTIC_SUGGEST_ATTRIBUTE_NORETURN
//#pragma GCC diagnostic warning "-Wsuggest-attribute=noreturn"
//#endif

/** Read from the iterator into a newly allocated sbuf
 * with mapped memory.
 */
sbuf_t *process_dir::sbuf_alloc(image_process::iterator &it) const
{
    std::filesystem::path fname = files[it.file_number];
    sbuf_t *sbuf = sbuf_t::map_file(fname);     // returns a new sbuf
    return sbuf;
}

double process_dir::fraction_done(const image_process::iterator &it) const
{
    return (double)it.file_number / (double)files.size();
}

std::string process_dir::str(const image_process::iterator &it) const
{
    return std::string("File ") + files[it.file_number].string();
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
/* Static function */

image_process *image_process::open(std::filesystem::path fn, bool opt_recurse, size_t pagesize_, size_t margin_)
{
    image_process *ip = 0;
    std::string fname_string = fn.string();

    if ( std::filesystem::exists(fn) == false ){
	throw NoSuchFile(fname_string);
    }

    if (std::filesystem::is_directory(fn)){
	/* If this is a directory, process specially */
	if (opt_recurse==0){
	    std::cerr << "error: " << fname_string << " is a directory but -R (opt_recurse) not set" << std::endl;
	    errno = 0;
	    throw NoSuchFile(fname_string);	// directory and cannot recurse
	}
        /* Quickly scan the directory and see if it has a .E01, .000 or .001 file.
         * If so, give the user an error.
         */
        for( const auto &p : std::filesystem::directory_iterator( fn )){
            if ( p.path().extension()==".E01" ||
                 p.path().extension()==".000" ||
                 p.path().extension()==".001") {
                std::cerr << "error: file " << p.path() << " is in directory " << fn << "\n";
                std::cerr << "       The -R option is not for reading a directory of EnCase files\n";
                std::cerr << "       or a directory of disk image parts. Please process these\n";
                std::cerr << "       as a single disk image. If you need to process these files\n";
                std::cerr << "       then place them in a sub directory of " << fn << "\n";
                throw NoSuchFile( fname_string );
            }
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

        std::string ext = fn.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (ext=="e01" || fname_string.find(".E01")!=std::string::npos){
#ifdef HAVE_LIBEWF
	    ip = new process_ewf(fn,pagesize_,margin_);
#else
	    throw NoSupport("This program was compiled without E01 support");
#endif
	}
	if (ip==nullptr) {
            ip = new process_raw(fn,pagesize_,margin_);
        }
    }
    /* Try to open it */
    if (ip->open()){
        throw NoSuchFile(fname_string);
    }
    return ip;
}
