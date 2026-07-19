/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// config.h needed solely for mmap
#include "config.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <ios>

#include "sbuf.h"
#include "dfxml_cpp/src/hash_t.h"
#include "formatter.h"
#include "unicode_escape.h"
#include "feature_recorder.h"

/****************************************************************
 *** SBUF_T
 *** Implement the sbuf abstraction, which is the primary buffer management
 *** tool of bulk_extractor. sbufs maintain memory management.
 *** Could this be done with a smart pointer?
 ****************************************************************/

#ifndef O_BINARY
#define O_BINARY 0
#endif

std::atomic<bool>    sbuf_t::debug_range_exception = false; // alert exceptions
std::atomic<bool>    sbuf_t::debug_alloc = false; // debug sbuf leaks; set by bulk_extractor when DEBUG_SBUF_ALLOC is set
std::atomic<bool>    sbuf_t::debug_leak  = false; // debug sbuf leaks
std::set<sbuf_t *>   sbuf_t::sbuf_alloced;        // allocated sbufs, but only if debug_alloc is true
std::mutex           sbuf_t::sbuf_allocedM;
std::atomic<int64_t> sbuf_t::sbuf_total = 0;
std::atomic<int64_t> sbuf_t::sbuf_count = 0;

/****************************************************************
 *** All allocators go here.
 ***/

/* Make an empty sbuf */
sbuf_t::sbuf_t()
{
    sbuf_total += 1;
    sbuf_count += 1;
    if (debug_leak) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        sbuf_alloced.insert( this );
    }
    if (debug_alloc) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        std::cerr << "sbuf_t::sbuf() " << this << " " << *this << std::endl;
    }
}

/* from an offset */
sbuf_t::sbuf_t(const sbuf_t &src, size_t offset):
    pos0(src.pos0 + (offset < src.bufsize ? offset : src.bufsize)),
    bufsize( offset < src.bufsize ? src.bufsize - offset : 0),
    pagesize( offset < src.pagesize ? src.pagesize - offset : 0),
    parent(&src), buf(src.buf+offset)
{
    parent->add_child(*this);
    sbuf_total += 1;
    sbuf_count += 1;
    if (debug_leak) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        sbuf_alloced.insert( this );
    }
    if (debug_alloc) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        std::cerr << "sbuf_t::sbuf(src,offset) " << this << " " << *this << std::endl;
    }
}

// start at offset for a given len
sbuf_t::sbuf_t(const sbuf_t &src, size_t offset, size_t len):
    pos0(src.pos0 + (offset < src.bufsize ? offset : src.bufsize)),
    bufsize( offset + len < src.bufsize ? len : (offset > src.bufsize ? 0 : src.bufsize - offset)),
    pagesize( offset + len < src.pagesize ? len : (offset > src.pagesize ? 0 : src.pagesize - offset)),
    parent(&src), buf(src.buf+offset)
{
    parent->add_child(*this);
    sbuf_total += 1;
    sbuf_count += 1;
    if (debug_leak) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        sbuf_alloced.insert( this );
    }
    if (debug_alloc) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        std::cerr << "sbuf_t::sbuf(src,offset,len) " << this << " " << *this << std::endl;
    }
}

/* Create an sbuf from a block of memory that does not need to be freed when the sbuf is deleted. */
sbuf_t::sbuf_t(pos0_t pos0_, const uint8_t *buf_, size_t bufsize_):
    pos0(pos0_), bufsize(bufsize_), pagesize(bufsize_),
    parent(), buf(buf_), malloced(nullptr) {
    sbuf_total += 1;
    sbuf_count += 1;

    if (debug_leak) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        sbuf_alloced.erase( this );
    }
    if (debug_alloc) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        std::cerr << "sbuf_t::sbuf(pos0,buf,bufsize) " << this << " " << *this << std::endl;
    }
}

/* Flexible allocator used by _new static methods below*/
sbuf_t::sbuf_t(pos0_t pos0_, const sbuf_t *parent_,
               const uint8_t* buf_, size_t bufsize_, size_t pagesize_,
               int fd_):
    pos0(pos0_), bufsize(bufsize_), pagesize(pagesize_),
    fd(fd_), parent(parent_), buf(buf_)
{
    if (parent) {
        parent->add_child(*this);
    }
    sbuf_total += 1;
    sbuf_count += 1;
    if (debug_leak) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        sbuf_alloced.insert( this );
    }
    if (debug_alloc) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        std::cerr << "sbuf_t::sbuf(pos0,parent,buf,bufsize,pagesize,fd) " << this << " " << *this << std::endl;
    }
}


/****************************************************************
 *** deallocator ***
 ****************************************************************/

sbuf_t::~sbuf_t()
{
    if (debug_alloc) {
        const std::lock_guard<std::mutex> lock(sbuf_allocedM); // protect this function
        std::cerr << "sbuf_t::~sbuf_t() " << this << " " << *this << std::endl;
    }
    if (children != 0) {
        std::runtime_error(Formatter() << "sbuf.cpp: error: sbuf children=" << children);
    }
    {
        const std::lock_guard<std::mutex> lock(Mhistogram); // protect this function
        if (histogram){
            delete histogram;
            histogram = nullptr;
        }
    }
    if (parent) parent->del_child(*this);
    if (fd>0) {
#ifdef HAVE_MMAP
        munmap((void*)buf, bufsize);
#else
        std::runtime_error(Formatter() << "sbuf.cpp: fd>0 and HAVE_MMAP is not defined");
#endif
        ::close(fd);
    }
    if (malloced != nullptr) {
        free( malloced );
    }
    sbuf_count -= 1;
    if (debug_leak) {
        const std::lock_guard<std::mutex> lk(sbuf_allocedM); // protect this function
        sbuf_alloced.erase( this );
    }
}

/****************************************************************
 *** accessors
 ****************************************************************/

const uint8_t* sbuf_t::get_buf() const
{
    return buf;
}

void* sbuf_t::malloc_buf() const
{
    if (malloced == nullptr ) {
        throw std::runtime_error("malloc_buf called on sbuf_t that was not malloced");
    }
    return malloced;
}

void sbuf_t::add_child(const sbuf_t& child) const
{
    children   += 1;
    reference_count += 1;
}

void sbuf_t::del_child(const sbuf_t& child) const
{
    children   -= 1;
    assert(children >= 0);
    reference_count -= 1;
}

/****************************************************************
 ** Allocators.
 ****************************************************************/

/* a new sbuf with the data from one but the pos from another.
 * Buffer is not freed when sbuf is deleted.
 */
sbuf_t* sbuf_t::sbuf_new(pos0_t pos0_, const uint8_t* buf_, size_t bufsize_, size_t pagesize_)
{
    return new sbuf_t(pos0_, nullptr, // pos0, parent
                      buf_, bufsize_, std::min(bufsize_,pagesize_), // buf, bufsize, pagesize
                      NO_FD); // fd
}

/* Allocate from a string, copying the string into an allocated buffer, and automatically calling free(buf_) when the sbuf is deleted.
 * No space is left for terminating \0.
 */
sbuf_t* sbuf_t::sbuf_malloc(pos0_t pos0, const std::string &str)
{
    sbuf_t *ret = sbuf_t::sbuf_malloc(pos0,  str.size(), str.size());
    memcpy( ret->malloc_buf(), str.c_str(), str.size());
    return ret;
}


/*
 * Deletes this sbuf and allocates a new one.
 */
sbuf_t *sbuf_t::realloc(size_t newsize)
{
    if (parent!=nullptr) {
        throw std::runtime_error("sbuf_t::realloc called on sbuf that has a parent.");
    }
    if (children!=0) {
        throw std::runtime_error("sbuf_t::realloc called on sbuf that has children.");
    }
    if (reference_count>0) {
        throw std::runtime_error("sbuf_t::realloc called on sbuf that has a reference_count>0");
    }
    if (malloced==nullptr) {
        throw std::runtime_error("sbuf_t::realloc called on buffer that was not malloced");
    }
    if (buf_writable==nullptr) {
        throw std::runtime_error("sbuf_t::realloc called on buffer that is not writable");
    }
    if (malloced != buf_writable) {
        throw std::runtime_error("sbuf_t::realloc called on buffer where malloced!=writable");
    }
    if (newsize > bufsize) {
        throw std::runtime_error("sbuf_t::realloc attempt to make sbuf bigger");
    }
    const std::lock_guard<std::mutex> lock(Mhistogram); // protect this function
    if (histogram){
        throw std::runtime_error("sbuf_t::realloc attempt on an sbuf that has a histogram");
    }

#ifdef OLD_CODE
    malloced = ::realloc(malloced, newsize);
    if (malloced==nullptr) {
        throw std::bad_alloc();
    }
    sbuf_t *ret = new sbuf_t(pos0, nullptr,
                             static_cast<const uint8_t *>(malloced), newsize, newsize,
                             0);
    ret->malloced = malloced;           // ret will delete it
    ret->buf_writable = static_cast<uint8_t *>(malloced);
    buf          = nullptr;             // don't print it.
    malloced     = nullptr;             // prevent double deletion
    buf_writable = nullptr;             // no longer writable
    delete this;                        // this is a move
    return ret;
#else
    malloced = ::realloc(malloced, newsize);
    if (malloced==nullptr) {
        throw std::bad_alloc();
    }
    buf_writable = static_cast<uint8_t *>(malloced);

    /* These are all const, and we're going to nuke them. Have pitty on my const sole.  */
    *(const_cast<uint8_t **>(&buf))        = buf_writable;
    *(const_cast<size_t *>(&bufsize))      = newsize;
    *(const_cast<size_t *>(&pagesize))     = newsize;
    return this;
#endif
}

/** Allocate a subset of an sbuf's memory to a child sbuf.
 * from within an existing sbuf.
 * The allocated buf MUST be freed before the parent, since no copy is made...
 */
sbuf_t *sbuf_t::new_slice(pos0_t new_pos0, size_t off, size_t len) const
{
    if (off > bufsize)     throw range_exception_t(off, len); // check to make sure off is in the buffer
    if (off+len > bufsize) throw range_exception_t(off, len); // check to make sure off+len is in the buffer

    size_t new_pagesize = pagesize;
    if (off > pagesize) {
        new_pagesize -= off;            // we only have this much left
    }
    if (new_pagesize > len) {
        new_pagesize = len;             // we only have this much left
    }

    return new sbuf_t(new_pos0, highest_parent(),
                      buf + off, len, new_pagesize,
                      NO_FD);
}

sbuf_t *sbuf_t::new_slice(size_t off, size_t len) const
{
    return new_slice(pos0+off, off, len);
}


/** Copy a subset of an sbuf's memory to a child sbuf.
 */
sbuf_t *sbuf_t::new_slice_copy(size_t off, size_t len) const
{
    auto src  = slice(off, len);
    assert(src.bufsize  <= len);
    assert(src.pagesize <= len);
    auto *dst = sbuf_t::sbuf_malloc(src.pos0, src.bufsize, src.bufsize);
    memcpy( dst->malloc_buf(), src.buf, src.bufsize);
    return dst;
}

sbuf_t sbuf_t::slice(size_t off, size_t len) const
{
    if (off > bufsize)     throw range_exception_t(off, len); // check to make sure off is in the buffer
    if (off+len > bufsize) throw range_exception_t(off, len); // check to make sure off+len is in the buffer

    size_t new_pagesize = pagesize;
    if (off > pagesize) {
        new_pagesize -= off;            // we only have this much left
    }
    if (new_pagesize > len) {
        new_pagesize = len;             // we only have this much left
    }

    return sbuf_t(pos0 + off, highest_parent(),
                      buf + off, len, new_pagesize,
                      NO_FD);
}

sbuf_t *sbuf_t::new_slice(size_t off) const
{
    return new_slice(off, bufsize - off);
}

sbuf_t sbuf_t::slice(size_t off) const
{
    return slice(off, bufsize - off);
}


/* Map a file when we are given an open fd.
 * The fd is not closed when the file is unmapped.
 * If there is no mmap, just allocate space and read the file
 *
 * TODO: Add CreateFileMapping, MapViewOfFile,UnmapViewOfFile and CloseHandle
 * as described in  https://imadiversion.co.uk/2016/12/08/c-17-and-memory-mapped-io/
 */

sbuf_t* sbuf_t::map_file(const std::filesystem::path fname) {
    int mfd = NO_FD;
    std::uintmax_t bytes = std::filesystem::file_size( fname );
#ifdef HAVE_MMAP
    mfd = ::open(fname.c_str(), O_RDONLY);
    uint8_t* mbuf = (uint8_t*)mmap(0, bytes, PROT_READ, MAP_FILE | MAP_SHARED, mfd, 0);
#else
    uint8_t *mbuf = static_cast<uint8_t*>(malloc(bytes));
    if (mbuf == nullptr) {
        throw std::bad_alloc();
    }
    std::fstream infile(fname, std::ios::in | std::ios::binary);
    if (!infile.is_open()){
        throw std::runtime_error(Formatter() << "Cannot open " << fname);
    }
    infile.read( reinterpret_cast<char *>(mbuf), bytes);
    if (infile.rdstate() & std::ios::eofbit) {
        free(mbuf); /* read failed */
        throw std::runtime_error(Formatter() << "End of file: " << fname);
    }
    if (infile.rdstate() & std::ios::failbit) {
        free(mbuf); /* read failed */
        throw std::runtime_error(Formatter() << "Fail file: " << fname);
    }
    if (infile.rdstate() & std::ios::badbit) {
        free(mbuf); /* read failed */
        throw std::runtime_error(Formatter() << "Bad file: " << fname);
    }
    infile.close();
#endif
    return new sbuf_t(pos0_t(fname.string() + pos0_t::map_file_delimiter), nullptr,
                      mbuf, bytes, bytes,
                      mfd);
}


/*
 * Allocate a new sbuf with a malloc and return a writable buffer.
 * In the future we will add guard bytes. Byte 0 is at pos0.
 * There's no parent, because this sbuf owns the memory.
 */
sbuf_t* sbuf_t::sbuf_malloc(pos0_t pos0_, size_t bufsize_, size_t pagesize_)
{
    assert( bufsize_ >= pagesize_ );
    uint8_t *new_malloced = static_cast<uint8_t *>(malloc(bufsize_));
    sbuf_t *ret = new sbuf_t(pos0_, nullptr,
                             new_malloced, bufsize_, pagesize_,
                             NO_FD);
    ret->malloced = static_cast<void *>(new_malloced);
    ret->buf_writable = new_malloced;
    assert(ret->buf == ret->malloced);
    assert(ret->buf == ret->buf_writable);
    if (debug_alloc) {
        std::cerr << "sbuf_t::sbuf_malloc(" << pos0_ << "," << bufsize_ << "," << pagesize_ << ") = " << ret << std::endl;
    }
    return ret;
}

void sbuf_t::wbuf(size_t i, uint8_t val)
{
    if ( buf_writable==nullptr) {
        throw std::runtime_error("Attempt to write to unwritable sbuf");
    }
    if ( i<0 ){
        throw std::runtime_error("Attempt to write sbuf i<0");
    }
    if ( i>bufsize ){
        throw std::runtime_error("Attempt to write sbuf i>bufsize");
    }
    buf_writable[i] = val;
}

/**
 * rawdump the sbuf to an ostream.
 */
void sbuf_t::raw_dump(std::ostream& os, uint64_t start, uint64_t len) const {
    for (uint64_t i = start; i < start + len && i < bufsize; i++) { os << buf[i]; }
}

/**
 * rawdump the sbuf to a file descriptor
 */
void sbuf_t::raw_dump(int fd2, uint64_t start, uint64_t len) const {
    if (len > bufsize - start) len = bufsize - start; // maximum left
    uint64_t written = ::write(fd2, buf + start, len);
    if (written != len) {
        std::cerr << "write: cannot write sbuf.\n";
    }
}

static std::string hexch(unsigned char ch) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%02x", ch);
    return std::string(buf);
}

/**
 * hexdump the sbuf.
 */
void sbuf_t::hex_dump(std::ostream& os, uint64_t start, uint64_t len) const {
    const size_t bytes_per_line = 32;
    size_t max_spaces = 0;
    for (uint64_t i = start; i < start + len && i < bufsize; i += bytes_per_line) {
        size_t spaces = 0;

        /* Print the offset */
        char b[64];
        snprintf(b, sizeof(b), "%04x: ", (int)i);
        os << b;
        spaces += strlen(b);

        for (size_t j = 0; j < bytes_per_line && i + j < bufsize && i + j < start + len; j++) {
            unsigned char ch = (*this)[i + j];
            os << hexch(ch);
            spaces += 2;
            if (j % 2 == 1) {
                os << " ";
                spaces += 1;
            }
        }
        if (spaces > max_spaces) max_spaces = spaces;
        for (; spaces < max_spaces; spaces++) { os << ' '; }
        for (size_t j = 0; j < bytes_per_line && i + j < bufsize && i + j < start + len; j++) {
            unsigned char ch = (*this)[i + j];
            if (ch >= ' ' && ch <= '~')
                os << ch;
            else
                os << '.';
        }
        os << "\n";
    }
}

/* Write to a file descriptor */
ssize_t sbuf_t::write(int fd_, size_t loc, size_t len) const
{
    if (loc >= bufsize) return 0;                 // cannot write
    if (loc + len > bufsize) len = bufsize - loc; // clip at the end
    return ::write(fd_, buf + loc, len);
}

/* Write to a FILE */
ssize_t sbuf_t::write(FILE* f, size_t loc, size_t len) const
{
    if (loc >= bufsize) return 0;                 // cannot write
    if (loc + len > bufsize) len = bufsize - loc; // clip at the end
    return ::fwrite(buf + loc, 1, len, f);
}

/* Write to an output stream */
ssize_t sbuf_t::write(std::ostream& os, size_t loc, size_t len) const
{
    if (loc >= bufsize) return 0;                 // cannot write
    if (loc + len > bufsize) len = bufsize - loc; // clip at the end
    os.write(reinterpret_cast<const char*>(buf+loc), len);
    if (os.rdstate() & (std::ios::failbit|std::ios::badbit)){
        throw std::runtime_error("sbuf_t::write");
    }
    return len;
}

ssize_t sbuf_t::write(std::ostream &os) const
{
    return write(os, 0, bufsize);
}

/* Write to path */
ssize_t sbuf_t::write(std::filesystem::path path) const
{
    std::ofstream os;
    os.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!os.is_open()) {
        throw std::runtime_error(Formatter() << "cannot open file for writing:" << path);
    }
    this->write(os, 0, bufsize);
    os.close();
    if (os.bad()) {
        throw feature_recorder::DiskWriteError(Formatter() << "error writing file " << path);
    }
    return bufsize;
}

/* Return a substring */
const std::string sbuf_t::substr(size_t loc, size_t len) const {
    if (loc >= bufsize) return std::string("");   // cannot write
    if (loc + len > bufsize) len = bufsize - loc; // clip at the end
    return std::string((const char*)buf + loc, len);
}

bool sbuf_t::is_constant(size_t off, size_t len, uint8_t ch) const // verify that it's constant
{
    while (len > 0) {
        if (((*this)[off]) != ch) return false;
        off++;
        len--;
    }
    return true;
}

uint16_t sbuf_t::distinct_characters(size_t off, size_t len) const // verify that it's constant
{
    if (off==0 && len==bufsize){
        return get_distinct_character_count();
    }
    uint32_t counts[256];
    memset(counts,0,sizeof(counts));
    uint16_t distinct_counts = 0;
    /* How many distinct counts do we have? */
    while ( len >0 )	{
        if (++counts[ (*this)[off] ]  == 1 ){
            distinct_counts++;
        }
        off++;
        len--;
    }
    return distinct_counts;
}

void sbuf_t::hex_dump(std::ostream& os) const
{
    hex_dump(os, 0, bufsize);
}

/* Determine if the sbuf consists of a repeating ngram.
 * results are computed lazily and cached for all threads.
 */
size_t sbuf_t::find_ngram_size(const size_t max_ngram) const {
    const std::lock_guard<std::mutex> lock(Mngram_size); // protect this function
    if (ngram_size == NO_NGRAM) {
        for (size_t ns = 1; ns < max_ngram; ns++) {
            bool ngram_match = true;
            for (size_t i = ns; i < pagesize ; i++) {
                if ((buf[i % ns]) != buf[i]) {
                    ngram_match = false;
                    break;
                }
            }
            if (ngram_match && ns*2 < pagesize) { // it had to repeat at least once
                ngram_size = ns;
                assert (ngram_size != NO_NGRAM); // better be set now
                return ngram_size;
            }
        }
        ngram_size = 0;                 // no ngram was found
    }
    assert (ngram_size != NO_NGRAM);    // it should be set now
    return ngram_size; // no ngram size
}

/* allocate and return a histogram */
sbuf_t::sbuf_histogram *sbuf_t::get_histogram() const
{
    const std::lock_guard<std::mutex> lock(Mhistogram); // protect this function
    if (histogram==nullptr){
        histogram = new sbuf_histogram();
        for( size_t i=0;i<bufsize;i++){
            histogram->count[ buf[ i ] ] ++;
            if (histogram->count[buf[i]]==1) {
                histogram->unique_chars += 1;
            }
        }
    }
    return histogram;
}

size_t sbuf_t::get_distinct_character_count() const
{
    return get_histogram()->unique_chars;
}

bool sbuf_t::getline(size_t& pos, size_t& line_start, size_t& line_len) const
{
    /* Scan forward until pos is at the beginning of a line.
     * The line needs to start in the page, not in the margin
     */
    if (pos >= this->pagesize) return false;
    if (pos > 0) {
        while ((pos < this->pagesize) && buf[pos - 1] != '\n') {
            ++pos;
        }
        if (pos >= this->pagesize) return false; // didn't find another start of a line
    }
    line_start = pos;
    /* Now scan to end of the line, or the end of the buffer */
    while (++pos < this->bufsize) {
        if (buf[pos] == '\n') { break; }
    }
    line_len = (pos - line_start);
    return true;
}

ssize_t sbuf_t::find(uint8_t ch, size_t start) const
{
    for (; start < pagesize; start++) {
        if (buf[start] == ch) return start;
    }
    return -1;
}

/*
 * High-speed find a binary object within an sbuf.
 */
ssize_t sbuf_t::findbin(const uint8_t* b2, size_t buflen, size_t start ) const
{
    if (buflen == 0) return -1; // nothing to search for

    for (; start < pagesize; start++) {
        const uint8_t* p = static_cast<const uint8_t *>(memchr(buf + start, b2[0], bufsize - start)); // perhaps memchr be optimized
        if (p == 0) return -1; // first character not present,
        size_t loc = p - buf;
        for (size_t i = 0; loc + i < bufsize && i<buflen; i++) {
            if ( buf[loc + i] != b2[i]) break;
            if ( i==buflen-1 ) return loc;  // reached end of buffer
        }
        start = loc + 1; // advance to character after found character
    }
    return -1;
}

/**
 * Read the requested number of UTF-8 format string octets including any \0.
 */
std::string sbuf_t::getUTF8(size_t i, size_t num_octets_requested) const
{
    // clear any residual value
    std::string utf8_string;

    while (i < bufsize && num_octets_requested > 0 ){
        utf8_string.push_back( get8u(i) );
        i += 1;
        num_octets_requested -= 1;
    }
    return utf8_string;
}

/**
 * Read UTF-8 format code octets into string up to but not including \0.
 */
std::string sbuf_t::getUTF8(size_t i) const
{
    std::string utf8_string;

    while (i < bufsize) {
        uint8_t octet = get8u(i);
        // stop before \0
        if (octet == 0) {
            // at \0
            break;
        }
        // accept the octet
        utf8_string.push_back(octet);
        i += 1;
    }
    return utf8_string;
}

/**
 * Read the requested number of UTF-16 format code units into wstring including any \U0000.
 */
std::wstring sbuf_t::getUTF16(size_t i, size_t num_code_units_requested) const
{
    // clear any residual value
    std::wstring utf16_string;

    while (i+1 < bufsize && num_code_units_requested > 0) {
        utf16_string.push_back(get16u(i));
        i += 2;
        num_code_units_requested -= 1;
    }
    return utf16_string;
}

/**
 * Read UTF-16 format code units into wstring up to but not including \U0000.
 */
std::wstring sbuf_t::getUTF16(size_t i) const
{
    std::wstring utf16_string;

    while (i+1 < bufsize) {
        uint16_t code_unit = get16u(i);
        // stop before \U0000
        if (code_unit == 0) {
            // at \U0000
            break;
        }
        // accept the code unit
        utf16_string.push_back(code_unit);
        i+=2;
    }
    return utf16_string;
}

/**
 * Read the requested number of UTF-16 format code units using the specified byte order into wstring including any
 * \U0000.
 */
std::wstring sbuf_t::getUTF16(size_t i, size_t num_code_units_requested, byte_order_t bo) const
{
    // clear any residual value
    std::wstring utf16_string;
    while (i+1 < bufsize && num_code_units_requested > 0) {
        utf16_string.push_back(get16u(i, bo));
        i += 2;
        num_code_units_requested -= 1;
    }
    return utf16_string;
}

/**
 * Read UTF-16 format code units using the specified byte order into wstring up to but not including \U0000.
 */
std::wstring sbuf_t::getUTF16(size_t i, byte_order_t bo) const
{
    std::wstring utf16_string;

    while (i+1 < bufsize) {
        uint16_t code_unit = get16u(i, bo);
        // stop before \U0000
        if (code_unit == 0) {
            // at \U0000
            break;
        }
        // accept the code unit
        utf16_string.push_back(code_unit);
        i+=2;
    }
    return utf16_string;
}

std::string sbuf_t::hash() const
{
    const std::lock_guard<std::mutex> lock(Mhash); // protect this function
    if (hash_.size() == 0) {
        /* hasn't been hashed yet, so hash it */
        hash_ = dfxml::sha1_generator::hash_buf(buf, bufsize).hexdigest();
    }
    return hash_;
}

/* Similar to above, but does not cache, so it is inherently threadsafe */
std::string sbuf_t::hash(hash_func_t func) const
{
    return func(buf, bufsize);
}

/* Report if the hash exists */
bool sbuf_t::has_hash() const
{
    const std::lock_guard<std::mutex> lock(Mhash); // protect this function}
    return (hash_.size() > 0 );
}


/****************************************************************
 ** ostream operators
 ****************************************************************/
std::ostream& operator<<(std::ostream& os, const sbuf_t& t) {
    os << "sbuf[pos0=" << t.pos0 << " " ;

    if (t.buf) {
        os << "buf[0..8]=";
        for (size_t i=0; i < 8 && i < t.bufsize; i++){
            os << hexch(t[i]) << ' ';
        }
        os << " (" ;
        for (size_t i=0; i < 8 && i < t.bufsize; i++){
            if (isprint(t[i])) {
                os << t[i];
            }
        }
        os << " )" ;
    }
    os << " buf= "            << static_cast<const void *>(t.buf)
       << " malloced= "       << static_cast<const void *>(t.malloced)
       << " write= "          << static_cast<const void *>(t.buf_writable)
       << " size=(" << t.bufsize << "/" << t.pagesize << ")"
       << " children="        << t.children
       << " refct="           << t.reference_count
       << " fd="              << t.fd
       << " depth="           << t.depth()
       << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const sbuf_t::range_exception_t & e) {
    os << e.message();
    return os;
}
