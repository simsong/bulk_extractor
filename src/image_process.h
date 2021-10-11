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
 * process_raw - process a RAW or splitraw file.
 * process_dir - recursively process a directory of files (but not E01  files)
 *
 * Conditional compilation assures that this compiles no matter which class libraries are installed.
 *
 * subclasses must implement these two methods for path printing:
 * - pread()
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
 * sbuf_alloc()         - reads the data and increments the pointer, returns a new sbuf
 *
 * const methods:
 * get_pos0 - returns the forensic path of byte[0] that the sbuf would return
 * fraction_done() - the amount done
 * str() - returns a string for current position
 * max_blocks() - returns the number of blocks with the current block size
 * seek_block(block_number) - seeks to a block number n where 0 <= n < max_blocks()
 */

#include "be13_api/sbuf.h"
#include "be13_api/abstract_image_reader.h"

#include <filesystem>

#if defined(_WIN32)
#  include <winsock2.h>
#  include <windows.h>
#  include <windowsx.h>
#endif

class image_process : public abstract_image_reader {
    /******************************************************
     *** neither copying nor assignment is implemented. ***
     ******************************************************/
    image_process(const image_process &)=delete;
    image_process &operator=(const image_process &)=delete;

    /****************************************************************/
    const std::filesystem::path image_fname_;			/* image filename */

public:
    /* These two functions are only used in WIN32 but are defined here so that they can be tested on all platforms */
    static std::string filename_extension(std::filesystem::path fn); // returns extension

    static bool fn_ends_with(std::filesystem::path str,std::string suffix);
    static bool is_multipart_file(std::filesystem::path fn);
    static std::string make_list_template(std::filesystem::path fn,int *start);

    struct EndOfImage : public std::exception {
        EndOfImage(){};
        const char *what() const noexcept override {return "end of image.";}
    };
    struct SeekError : public std::exception {
        SeekError(){};
        const char *what() const noexcept override {return "seek error.";}
    };
    struct ReadError : public std::exception {
        ReadError(){};
        const char *what() const noexcept override {return "read error.";}
    };
    struct NoSuchFile : public std::exception {
        std::string m_error{};
        NoSuchFile(std::string_view error):m_error(error){}
        const char *what() const noexcept override {return m_error.c_str();}
    };
    struct NoSupport : public std::exception {
        std::string m_error{};
        NoSupport(std::string_view error):m_error(error){}
        const char *what() const noexcept override {return m_error.c_str();}
    };

    /**
     * open() figures out which child class to call, calls its open, then
     * returns an object.
     */
    static image_process *open(std::filesystem::path fn, bool recurse, size_t opt_pagesize, size_t opt_margin);
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
	const class image_process &myimage;
	uint64_t raw_offset {};
	uint64_t page_number {};
	size_t   file_number {};
	bool     eof {};
	iterator(const class image_process *myimage_):myimage(*myimage_) { }
	bool operator !=(const iterator &it) const {
	    if (this->eof && it.eof) return false; /* both in EOF states, so they are equal */
	    if (this->raw_offset!=it.raw_offset ||
                this->page_number!=it.page_number || this->file_number!=it.file_number
	       ) return true;
	    return false;
	}
        bool operator ==(const iterator &it) const {
            return !((*this) != it);
        }
	pos0_t get_pos0() const { return myimage.get_pos0(*this); }			   // returns the current pos0
	sbuf_t *sbuf_alloc()    { return myimage.sbuf_alloc(*this); }   // allocates an sbuf at pos0
	double fraction_done() const { return myimage.fraction_done(*this); }
        std::string str() const { return myimage.str(*this); }
	uint64_t max_blocks() const { return myimage.max_blocks(*this);}
	uint64_t seek_block(uint64_t block) { return myimage.seek_block(*this,block);} // returns block number
        void set_raw_offset(uint64_t anOffset){ raw_offset=anOffset;}
    };

    image_process(std::filesystem::path fn, size_t pagesize_, size_t margin_);
    virtual ~image_process();

    /* image support */
    virtual int open()=0;				    /* open; return 0 if successful */
    /* pread defined in superclass */
    //virtual int pread(void *,size_t bytes,int64_t offset) const = 0 ;
    virtual int64_t image_size() const=0;
    virtual std::filesystem::path image_fname() const;

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

    uint64_t ewf_filesize {};
    std::vector<std::string> details {};
    mutable libewf_handle_t *handle {};

 public:
    static void local_e01_glob(std::filesystem::path fname,char ***libewf_filenames,int *amount_of_filenames);

    process_ewf(std::filesystem::path fname, size_t pagesize_, size_t margin_) : image_process(fname, pagesize_, margin_) {}
    virtual ~process_ewf();
    std::vector<std::string> getewfdetails() const;
    int open() override;
    virtual ssize_t pread(void *,size_t bytes,uint64_t offset) const override;	    /* read */

    /* iterator support */
    virtual image_process::iterator begin() const override;
    virtual image_process::iterator end() const override;
    virtual void    increment_iterator(class image_process::iterator &it) const override;
    virtual pos0_t  get_pos0(const class image_process::iterator &it) const override;
    virtual sbuf_t  *sbuf_alloc(class image_process::iterator &it) const override;
    virtual double  fraction_done(const class image_process::iterator &it) const override;
    virtual std::string str(const class image_process::iterator &it) const override;
    virtual int64_t  image_size() const override;
    virtual uint64_t max_blocks(const class image_process::iterator &it) const override;
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const override; // returns -1 if failue
};
#endif

/****************************************************************
 *** RAW
 *** Read one or more raw files (to handle multipart disk images.
 ****************************************************************/

class process_raw : public image_process {
    class file_info {
    public:;
        file_info(const std::filesystem::path path_,uint64_t offset_,uint64_t length_):path(path_),offset(offset_),length(length_){};
        std::filesystem::path path {};  // the file name
	uint64_t offset   {};           // where each file starts
	uint64_t length   {};           // how long it is
    };
    typedef std::vector<file_info> file_list_t;
    file_list_t file_list {};
    void        add_file(std::filesystem::path fname);
    class       file_info const *find_offset(uint64_t offset) const; /* finds which file this offset would map to */
    uint64_t    raw_filesize {};			/* sume of all the lengths */
    mutable std::filesystem::path current_path {};
    mutable std::ifstream current_fstream {};		/* which file is currently open */
public:
    process_raw(std::filesystem::path image_fname,size_t pagesize,size_t margin);
    virtual ~process_raw();
    virtual int open() override;
    virtual ssize_t pread(void *,size_t bytes,uint64_t offset) const override;	    /* read */

    /* iterator support */
    virtual image_process::iterator begin() const override;
    virtual image_process::iterator end() const override;
    virtual void     increment_iterator(class image_process::iterator &it) const override;

    virtual pos0_t   get_pos0(const class image_process::iterator &it) const override;
    virtual sbuf_t  *sbuf_alloc(class image_process::iterator &it) const override;
    virtual double   fraction_done(const class image_process::iterator &it) const override;
    virtual std::string str(const class image_process::iterator &it) const override;
    virtual int64_t  image_size() const override;
    virtual uint64_t max_blocks(const class image_process::iterator &it) const override;
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const override; // returns -1 if failue
};

/****************************************************************
 *** Directory Recursion
 ****************************************************************/


class process_dir : public image_process {
 private:
    std::vector<std::filesystem::path> files {};		/* all of the files */

 public:
    process_dir(std::filesystem::path image_dir);
    virtual ~process_dir();

    virtual int open() override;
    virtual ssize_t pread(void *,size_t bytes,uint64_t offset) const override ;

    /* iterator support */
    virtual image_process::iterator begin() const override;
    virtual image_process::iterator end() const override;
    virtual void increment_iterator(class image_process::iterator &it) const override;

    virtual pos0_t   get_pos0(const class image_process::iterator &it)   const override;
    virtual sbuf_t  *sbuf_alloc(class image_process::iterator &it) const override;   /* maps the next dir */
    virtual double   fraction_done(const class image_process::iterator &it) const override; /* number of dirs processed */
    virtual std::string str(const class image_process::iterator &it) const override;
    virtual int64_t  image_size() const override;				    /* total bytes */
    virtual uint64_t max_blocks(const class image_process::iterator &it) const override;
    virtual uint64_t seek_block(class image_process::iterator &it,uint64_t block) const override; // returns -1 if failu};
};

/****************************************************************
 *** MISC
 ****************************************************************/
#endif
