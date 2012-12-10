#ifndef IMAGE_PROCESS_H
#define IMAGE_PROCESS_H
/** image processing plug-in framework for working with images.
 * Right now it's pretty specific to bulk_extractor. 
 *
 * However, with the introduction of iterators, it could be removed from bulk_extractor.
 *
 * This class processes and entire disk image. Inside it are three methods:
 *  process() - process the entire image; called with the thread number.
 *
 * Subclasses of this class are used to process:
 * process_ewf - process an EWF file
 * process_aff - process an AFF file
 * process_raw - process a RAW or splitraw file.
 * process_dir - recursively process a directory of files (but not E01 or AFF files)
 * 
 * Conditional compilation assures that this compiles no matter which class libraries are installed.
 */

#include "sbuf.h"
#include "dig.h"

#if defined(WIN32) 
#  include <winsock2.h>
#  include <windows.h>
#  include <windowsx.h>
#endif

/* Undef HAVE_LIBAFFLIB if we don't have the include files */
#if defined(HAVE_LIBAFFLIB) && !defined(HAVE_AFFLIB_AFFLIB_H)
#  undef HAVE_LIBAFFLIB
#endif


#ifndef HAVE_STL
#  define HAVE_STL			/* needed for AFFLIB */
#endif

extern size_t opt_pagesize;
extern size_t opt_margin;

class image_process {
private:
    /******************************************************
     *** neither copying nor assignment is implemented. ***
     *** We do this by making them private constructors ***
     *** that throw exceptions.                         ***
     ******************************************************/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying feature_recorder objects is not implemented.";
	}
    };
    image_process(const image_process &ip) __attribute__((__noreturn__)) :image_fname(){throw new not_impl();}
    const image_process &operator=(const image_process &ip){throw new not_impl();}
    /****************************************************************/

public:    
    class read_error: public exception {
	virtual const char *what() const throw() {
	    return "read error";
	}
    };

    /* This was the easiest way to make an image_process::iterator
     * without the use of templates.
     * Basically, the iterator has a pointer to the specific image_process
     * class that is being used. Each implements the following functions:
     *  - increment_iterator
     *  - sbuf_alloc()
     *  - fraction_done
     */
    class iterator {
    public:
	int64_t  raw_offset;		
	uint64_t page_counter;
	size_t   file_number;
	class image_process *myimage;
	bool     eof;
	iterator(): raw_offset(0),page_counter(0),file_number(0),myimage(0),eof(false){
	}
	bool operator !=(class iterator it){
	    if(this->eof && it.eof) return false; /* both in EOF states, so they are equal */
	    if(this->raw_offset!=it.raw_offset
	       || this->page_counter!=it.page_counter
	       || this->file_number!=it.file_number
	       ) return true;
	    return false;
	}
	pos0_t get_pos0() const { return myimage->get_pos0(*this);}			   // returns the current pos0
	sbuf_t *sbuf_alloc(){ return myimage->sbuf_alloc(*this); } // allocates an sbuf at pos0
	double fraction_done(){ return myimage->fraction_done(*this); }
	string str(){ return myimage->str(*this); }
    };

    string image_fname;			/* image filename */
    image_process(const std::string &image_fname_):image_fname(image_fname_){}
    virtual ~image_process(){};

    /* image support */
    virtual int open()=0;				    /* open; return 0 if successful */
    virtual int pread(uint8_t *,size_t bytes,int64_t offset) const =0;	    /* read */
    virtual int64_t image_size()=0;

    /* iterator support; these virtual functions are called by iterator through (*myimage) */
    virtual image_process::iterator begin()=0;
    virtual image_process::iterator end()=0;
    virtual void increment_iterator(class image_process::iterator &it)=0;
    virtual pos0_t get_pos0(const class image_process::iterator &it) const =0;
    virtual sbuf_t *sbuf_alloc(class image_process::iterator &it)=0;
    virtual double fraction_done(class image_process::iterator &it)=0;
    virtual string str(class image_process::iterator &it)=0;
};

inline image_process::iterator & operator++(image_process::iterator &it){
    it.myimage->increment_iterator(it);
    return it;
}


/****************************************************************
 *** AFF
 ****************************************************************/

#ifdef HAVE_LIBAFFLIB
#include <afflib/afflib.h>
#include <vector>			
class process_aff : public image_process {
    /******************************************************
     *** neither copying nor assignment is implemented. ***
     *** We do this by making them private constructors ***
     *** that throw exceptions.                         ***
     ******************************************************/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying feature_recorder objects is not implemented.";
	}
    };
    process_aff(const process_aff &pa) __attribute__((__noreturn__)): image_process(""),af(0),pagelist(){
	throw new not_impl();
    }
    const process_aff &operator=(const process_aff &pa) __attribute__((__noreturn__)){throw new not_impl();}
    /****************************************************************/

    mutable AFFILE *af;
    vector<int64_t> pagelist;
public:
    image_process::iterator begin();
    image_process::iterator end();
    void increment_iterator(class image_process::iterator &it);
    process_aff(string image_fname_) : image_process(image_fname_),af(0),pagelist(){}
    virtual ~process_aff();

    /* Iterator Support */
    int open();
    int pread(uint8_t *,size_t bytes,int64_t offset) const;	    /* read */
    pos0_t get_pos0(const class image_process::iterator &it) const;    
    sbuf_t *sbuf_alloc(class image_process::iterator &it);
    double fraction_done(class image_process::iterator &it);
    string str(class image_process::iterator &it);
    int64_t image_size();
};
#endif

/****************************************************************
 *** EWF
 ****************************************************************/

/* Undefine HAVE_LIBEWF if we don't have include files */
#if defined(HAVE_LIBEWF) && !defined(HAVE_LIBEWF_H)
#undef HAVE_LIBEWF
#endif

#ifdef HAVE_LIBEWF
#include "libewf.h"

class process_ewf : public image_process {
 private:
    /******************************************************
     *** neither copying nor assignment is implemented. ***
     *** We do this by making them private constructors ***
     *** that throw exceptions.                         ***
     ******************************************************/
    class not_impl: public exception {
	virtual const char *what() const throw() {
	    return "copying feature_recorder objects is not implemented.";
	}
    };
    process_ewf(const process_ewf &pa) __attribute__((__noreturn__)):
    image_process(""),ewf_filesize(0),details(), handle(0){ throw new not_impl(); }
    const process_ewf &operator=(const process_ewf &pa){throw new not_impl();}
    /****************************************************************/

    int64_t ewf_filesize;
    vector<string> details; 	       
    mutable libewf_handle_t *handle;

 public:
    process_ewf(string image_fname_) : image_process(image_fname_), ewf_filesize(0), details() ,handle(0) {}
    virtual ~process_ewf();
    vector<string> getewfdetails();
    int open();
    int pread(uint8_t *,size_t bytes,int64_t offset) const;	    /* read */

    /* iterator support */
    image_process::iterator begin();
    image_process::iterator end();
    void increment_iterator(class image_process::iterator &it);
    pos0_t get_pos0(const class image_process::iterator &it) const;    
    sbuf_t *sbuf_alloc(class image_process::iterator &it);
    double fraction_done(class image_process::iterator &it);
    string str(class image_process::iterator &it);
    int64_t image_size();
};
#endif

/****************************************************************
 *** RAW
 ****************************************************************/

class process_raw : public image_process {
    class file_info {
    public:;
        file_info(string name_,int64_t offset_,int64_t length_):name(name_),offset(offset_),length(length_){};
        string name;
	int64_t offset;
	int64_t length;
    };
    typedef vector<file_info> file_list_t;
    file_list_t file_list;
    void add_file(string fname);
    class file_info const *find_offset(int64_t offset) const;
    int64_t raw_filesize;			/* sume of all the lengths */
    mutable string current_file_name;		/* which file is currently open */
    mutable int current_fd;			/* currently open file */
public:
    process_raw(string image_fname);
    virtual ~process_raw();
    int open();
    int pread(uint8_t *,size_t bytes,int64_t offset) const;	    /* read */

    /* iterator support */
    image_process::iterator begin();
    image_process::iterator end();
    void increment_iterator(class image_process::iterator &it);

    pos0_t get_pos0(const class image_process::iterator &it) const;    
    sbuf_t *sbuf_alloc(class image_process::iterator &it);
    double fraction_done(class image_process::iterator &it);
    string str(class image_process::iterator &it);
    int64_t image_size();
};

/****************************************************************
 *** Directory Recursion
 ****************************************************************/


class process_dir : public image_process {
 private:
    vector<string> files;		/* all of the files */

 public:
    process_dir(const std::string &image_dir);
    virtual ~process_dir();
    int open();
    int pread(uint8_t *,size_t bytes,int64_t offset) const __attribute__((__noreturn__));	 /* read */

    /* iterator support */
    image_process::iterator begin();
    image_process::iterator end();
    void increment_iterator(class image_process::iterator &it);

    pos0_t get_pos0(const class image_process::iterator &it) const;    
    sbuf_t *sbuf_alloc(class image_process::iterator &it); /* maps the next dir */
    double fraction_done(class image_process::iterator &it); /* number of dirs processed */
    string str(class image_process::iterator &it);
    int64_t image_size();				    /* total bytes */
};

/****************************************************************
 *** MISC
 ****************************************************************/

/**
 * Open the type as specified by extension.
 */

extern image_process *image_process_open(string fn,int flags);

#endif
