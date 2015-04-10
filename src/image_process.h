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
 *
 * subclasses must implement these two methods for path printing:
 * pread()
 *
 * iterators are constant for all of the process subclasses. Each
 * subclass needs to implement these methods that operator on the
 * iterator:
 *
 * iterator allocating methods:
 * begin() - returns an image_process::iterator with initialized variables (for example, raw_offset)
 * end()   - returns an iterator that pionts to end
 *
 * ~()   - Destructor. Closes files and frees resources
 *
 * iterator modifying methods:
 * increment_iterator() - advances to the iterator to the next thing
 * sbuf_alloc() - reads the data and increments the pointer
 *
 * const methods:
 * get_pos0 - returns the forensic path of byte[0] that the sbuf would return
 * fraction_done() - the amount done
 * str() - returns a string for current position
 * max_blocks() - returns the number of blocks with the current block size
 * seek_block(block_number) - seeks to a block number n where 0 <= n < max_blocks()
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

class image_process {
private:
    /******************************************************
     *** neither copying nor assignment is implemented. ***
     ******************************************************/
    image_process(const image_process &);
    image_process &operator=(const image_process &);

    /****************************************************************/
    const std::string image_fname_;			/* image filename */
public:    
    /**
     * open() figures out which child class to call, calls its open, then
     * returns an object.
     */
    static image_process *open(std::string fn,bool recurse,
                               size_t opt_pagesize,size_t opt_margin);
    const size_t pagesize;                    // page size we are using
    const size_t margin;                      // margin size we are using
    bool  report_read_errors;

    class read_error: public std::exception {
	virtual const char *what() const throw() {
	    return "read error";
	}
    };

    /* This was the easiest way to make an image_process::iterator
     * without the use of templates.
     * Basically, the iterator has a pointer to the specific image_process
     * class that is being used. The class implements virtual functions. The iterator calls them.
     */
    class iterator {
    public:
	int64_t  raw_offset;		
	uint64_t page_counter;
	size_t   file_number;
	const class image_process &myimage;
	bool     eof;
	iterator(const class image_process *myimage_):
            raw_offset(0),page_counter(0),file_number(0),myimage(*myimage_),eof(false){
	}
	bool operator !=(const iterator &it) const{
	    if(this->eof && it.eof) return false; /* both in EOF states, so they are equal */
	    if(this->raw_offset!=it.raw_offset
	       || this->page_counter!=it.page_counter
	       || this->file_number!=it.file_number
	       ) return true;
	    return false;
	}
        bool operator ==(const iterator &it) const{
            return !((*this) != it);
        }
	pos0_t get_pos0() const { return myimage.get_pos0(*this); }			   // returns the current pos0
	sbuf_t *sbuf_alloc() { return myimage.sbuf_alloc(*this); }   // allocates an sbuf at pos0
	double fraction_done() const { return myimage.fraction_done(*this); }
        std::string str() const { return myimage.str(*this); }
	uint64_t max_blocks() const { return myimage.max_blocks(*this);}
	uint64_t seek_block(uint64_t block) { return myimage.seek_block(*this,block);} // returns block number
        void set_raw_offset(int64_t anOffset){ raw_offset=anOffset;}
    };

    image_process(const std::string &fn,size_t pagesize_,size_t margin_):image_fname_(fn),pagesize(pagesize_),margin(margin_),
                                                                         report_read_errors(true){}
    virtual ~image_process(){};

    /* image support */
    virtual int open()=0;				    /* open; return 0 if successful */
    virtual int pread(uint8_t *,size_t bytes,int64_t offset) const =0;	    /* read */
    virtual int64_t image_size() const=0;
    virtual const std::string &image_fname() const { return image_fname_;}

    /* iterator support; these virtual functions are called by iterator through (*myimage) */
    virtual image_process::iterator begin() const =0;
    virtual image_process::iterator end() const=0;
    virtual void increment_iterator(class image_process::iterator &it) const = 0;
    virtual pos0_t get_pos0(const class image_process::iterator &it) const =0;
    virtual sbuf_t *sbuf_alloc(class image_process::iterator &it) const = 0;
    virtual double fraction_done(const class image_process::iterator &it) const = 0;
    virtual std::string str(const class image_process::iterator &it) const = 0; // returns a string representation of where we are
    virtual uint64_t max_blocks(const class image_process::iterator &it) const = 0;
    // seek_block modifies the iterator, but not the image!
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const = 0; // returns -1 if failure
    virtual void set_report_read_errors(bool val){report_read_errors=val;}
};

inline image_process::iterator & operator++(image_process::iterator &it){
    it.myimage.increment_iterator(it);
    return it;
}


/****************************************************************
 *** AFF
 ****************************************************************/

#ifdef HAVE_LIBAFFLIB
//#pragma GCC diagnostic ignored "-Wreserved-user-defined-literal"               // required for C11
#include <afflib/afflib.h>
#include <vector>			
class process_aff : public image_process {
    /******************************************************
     *** neither copying nor assignment is implemented. ***
     ******************************************************/
    process_aff(const process_aff &);
    process_aff &operator=(const process_aff &);
    /****************************************************************/

    mutable AFFILE *af;
    std::vector<int64_t> pagelist;
public:
    process_aff(std::string fname,size_t pagesize_,size_t margin_) : image_process(fname,pagesize_,margin_),af(0),pagelist(){}
    virtual ~process_aff();

    virtual image_process::iterator begin() const;
    virtual image_process::iterator end() const;
    virtual void increment_iterator(class image_process::iterator &it) const;

    /* Iterator Support */
    virtual int open();
    virtual int pread(uint8_t *,size_t bytes,int64_t offset) const;	    /* read */
    virtual pos0_t get_pos0(const class image_process::iterator &it) const;    
    virtual sbuf_t *sbuf_alloc(class image_process::iterator &it) const;
    virtual double fraction_done(const class image_process::iterator &it) const;
    virtual std::string str(const class image_process::iterator &it) const;
    virtual int64_t image_size() const;
    virtual uint64_t max_blocks(const class image_process::iterator &it) const;
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const; // returns -1 if failue
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
     ******************************************************/
    process_ewf(const process_ewf &);
    process_ewf &operator=(const process_ewf &);
    /****************************************************************/

    int64_t ewf_filesize;
    std::vector<std::string> details; 	       
    mutable libewf_handle_t *handle;
    //static int debug;

 public:
    process_ewf(const std::string &fname,size_t pagesize_,size_t margin_) :
        image_process(fname,pagesize_,margin_), ewf_filesize(0), details() ,handle(0) {}
    virtual ~process_ewf();
    std::vector<std::string> getewfdetails() const;
    int open();
    int pread(uint8_t *,size_t bytes,int64_t offset) const;	    /* read */

    /* iterator support */
    virtual image_process::iterator begin() const;
    virtual image_process::iterator end() const;
    virtual void    increment_iterator(class image_process::iterator &it) const;
    virtual pos0_t  get_pos0(const class image_process::iterator &it) const;    
    virtual sbuf_t *sbuf_alloc(class image_process::iterator &it) const;
    virtual double  fraction_done(const class image_process::iterator &it) const;
    virtual std::string str(const class image_process::iterator &it) const;
    virtual int64_t  image_size() const;
    virtual uint64_t max_blocks(const class image_process::iterator &it) const;
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const; // returns -1 if failue
};
#endif

/****************************************************************
 *** RAW
 ****************************************************************/

class process_raw : public image_process {
    class file_info {
    public:;
        file_info(const std::string &name_,int64_t offset_,int64_t length_):name(name_),offset(offset_),length(length_){};
        std::string name;
	int64_t offset;
	int64_t length;
    };
    typedef std::vector<file_info> file_list_t;
    file_list_t file_list;
    void        add_file(const std::string &fname);
    class       file_info const *find_offset(int64_t offset) const;
    int64_t     raw_filesize;			/* sume of all the lengths */
    mutable std::string current_file_name;		/* which file is currently open */
#ifdef WIN32
    mutable HANDLE current_handle;		/* currently open file */
#else
    mutable int current_fd;			/* currently open file */
#endif
public:
    process_raw(const std::string &image_fname,size_t pagesize,size_t margin);
    virtual ~process_raw();
    virtual int open();
    virtual int pread(uint8_t *,size_t bytes,int64_t offset) const;	    /* read */

    /* iterator support */
    virtual image_process::iterator begin() const;
    virtual image_process::iterator end() const;
    virtual void     increment_iterator(class image_process::iterator &it) const;

    virtual pos0_t   get_pos0(const class image_process::iterator &it) const;    
    virtual sbuf_t  *sbuf_alloc(class image_process::iterator &it) const;
    virtual double   fraction_done(const class image_process::iterator &it) const;
    virtual std::string str(const class image_process::iterator &it) const;
    virtual int64_t  image_size() const;
    virtual uint64_t max_blocks(const class image_process::iterator &it) const;
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const; // returns -1 if failue
};

/****************************************************************
 *** Directory Recursion
 ****************************************************************/


class process_dir : public image_process {
 private:
    std::vector<std::string> files;		/* all of the files */

 public:
    process_dir(const std::string &image_dir);
    virtual ~process_dir();

    virtual int open();
    virtual int pread(uint8_t *,size_t bytes,int64_t offset) const __attribute__((__noreturn__));	 /* read */
    
    /* iterator support */
    virtual image_process::iterator begin() const;
    virtual image_process::iterator end() const;
    virtual void increment_iterator(class image_process::iterator &it) const;
    
    virtual pos0_t   get_pos0(const class image_process::iterator &it)   const;    
    virtual sbuf_t   *sbuf_alloc(class image_process::iterator &it) const;   /* maps the next dir */
    virtual double   fraction_done(const class image_process::iterator &it) const; /* number of dirs processed */
    virtual std::string str(const class image_process::iterator &it) const;
    virtual int64_t  image_size() const;				    /* total bytes */
    virtual uint64_t max_blocks(const class image_process::iterator &it) const;
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const; // returns -1 if failu};
};

/****************************************************************
 *** MISC
 ****************************************************************/

/**
 * Open the type as specified by extension.
 */


#endif
