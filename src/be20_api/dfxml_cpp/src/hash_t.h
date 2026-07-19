/*
 * C++ covers for md5, sha1, and sha256 (and sha512 if present)
 *
 * hash representation classes: md5_t, sha1_t, sha256_t (sha512_t)
 * has generators: md5_generator(), sha1_generator(), sha256_generator()
 *
 * Generating a hash:
 * sha1_t val = sha1_generator::hash_buf(buf,bufsize)
 * sha1_t generator hasher;
 *       hasher.update(buf,bufsize)
 *       hasher.update(buf,bufsize)
 *       hasher.update(buf,bufsize)
 * sha1_t val = hasher.digest()
 *
 * Using the values:
 * string val.hexdigest()   --- return a hext digest
 * val.size()		    --- the size of the hash in bytes
 * uint8_t val.digest[SIZE] --- the buffer of the raw bytes
 *
 * note -  val.final()  has been depreciated due to collision with C++11
 *
 * This can be updated in the future for Mac so that the hash__ class
 * is then subclassed by a hash__openssl or a hash__commonCrypto class.
 *
 *
 * On MacOS, common crypto header files found in
 * /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/CommonCrypto
 *
 * Revision History:
 * 2012 - Simson L. Garfinkel - Created for bulk_extractor.
 *
 * 2020 - Simson L. Garfinkel - updated for MacOS CommonCrypto.
 *                            - file relicensed under LGPL
 */

#ifndef  HASH_T_H
#define  HASH_T_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <iostream>

#if defined(HAVE_GCRYPT) && defined(HAVE_GCRYPT_H)
#include <gcrypt.h>
#define USE_GCRYPT
#define HASH_IMPLEMENTATION_NAME "gcrypt"
#define HAVE_SHA512_T
#elif defined(HAVE_COMMONCRYPTO_COMMONDIGEST_H)
// We are going to ignore -Wdeprecated-declarations, because we need MD5
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <CommonCrypto/CommonDigest.h>
#define USE_COMMON_CRYPTO
#define HASH_IMPLEMENTATION_NAME "CommonCrypto"
#define HAVE_CRYPTO
#define HAVE_SHA512_T
#elif defined(HAVE_OPENSSL_HMAC_H) && defined(HAVE_OPENSSL_EVP_H)
#include <openssl/hmac.h>
#include <openssl/evp.h>
#define USE_OPENSSL
#define HASH_IMPLEMENTATION_NAME "OpenSSL"
#ifdef HAVE_EVP_SHA512
#define HAVE_SHA512_T
#endif
#else
#error CommonCrypto or OpenSSL required for hash_t.h. Did you include config.h?
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_MMAP_H
#include <sys/mmap.h>
#endif

/* This is a templated hasher. Originally it was designed for use with
 * openssl's EVP_MD system. Now it works with both OpenSSL's system and Common_Crypto
 */

#ifdef O_BINARY
#define HASHT_O_BINARY O_BINARY
#else
#define HASHT_O_BINARY 0
#endif

#define HASH_T_VERSION 2

namespace dfxml {

/*
 * we should use std::fs::filesystem_error(), but it doesn't seem to be ready for prime time yet.
 */
class fserror:public std::exception {
public:;
    const std::string msg;
    const int  error_code;
    fserror(const std::string &msg_,int error_code_):msg(msg_),error_code(error_code_){
    }
    virtual const char *what() const throw() {
        return msg.c_str();
    }
};

/* hash__ is a template that gets specialized to
 * md5_t, sha1_t, sha256_t and sha512_t (if available)
 *
 * It makes a copy of what it is initialized with.
 * In version 2.0 it is immutable.
 *
 * This works with both CommonCrypto and with OpenSSL.
 * We actually ignore the Type and the allocator
 */
template<size_t SIZE>
class hash
{
public:
    uint8_t digest[SIZE];
    static size_t size() {
        return(SIZE);
    }
    /* This returns a place where we can put results */
    hash(const uint8_t *provided):digest(){
	memcpy(this->digest,provided,size());
    }
    /* python like interface for hexdigest */
    static unsigned int hex2int(char ch){
	if(ch>='0' && ch<='9') return ch-'0';
	if(ch>='a' && ch<='f') return ch-'a'+10;
	if(ch>='A' && ch<='F') return ch-'A'+10;
	return 0;
    }
    static unsigned int hex2int(char ch0,char ch1){
        return (hex2int(ch0)<<4) | hex2int(ch1);
    }
    static hash fromhex(const std::string &hexbuf) {
        uint8_t digest[SIZE];
        assert(hexbuf.size()==SIZE*2);
	for(unsigned int i=0;i+1<hexbuf.size() && (i/2)<size();i+=2){
	    digest[i/2] = hex2int(hexbuf[i],hexbuf[i+1]);
	}
	return hash(digest);
    }
    const char *hexdigest(char *hexbuf,size_t bufsize) const {
	const char *hexbuf_start = hexbuf;
	for(unsigned int i=0;i<SIZE && bufsize>=3;i++){
	    snprintf(hexbuf,bufsize,"%02x",this->digest[i]);
	    hexbuf  += 2;
	    bufsize -= 2;
	}
	return hexbuf_start;
    }
    std::string hexdigest() const {
	std::string ret;
	char buf[SIZE*2+1];
	return std::string(hexdigest(buf,sizeof(buf)));
    }
    /**
     * Convert a hex representation to binary, and return
     * the number of bits converted.
     * @param binbuf output buffer
     * @param binbuf_size size of output buffer in bytes.
     * @param hex    input buffer (in hex)
     * @return the number of converted bits.
     */
    static int hex2bin(uint8_t *binbuf,size_t binbuf_size,const char *hex)
    {
	int bits = 0;
	while(hex[0] && hex[1] && binbuf_size>0){
	    *binbuf++ = hex2int(hex[0],hex[1]);
	    hex  += 2;
	    bits += 8;
	    binbuf_size -= 1;
	}
	return bits;
    }
    static const hash *new_from_hex(const char *hex) {
	hash *val = new hash();
	if(hex2bin(val->digest,sizeof(val->digest),hex)!=SIZE*8){
	    std::cerr << "invalid input " << hex << "(" << SIZE*8 << ")\n";
	    exit(1);
	}
	return val;
    }
    bool operator<(const hash &s2) const {
	/* Check the first byte manually as a performance hack */
	if(this->digest[0] < s2.digest[0]) return true;
	if(this->digest[0] > s2.digest[0]) return false;
	return memcmp(this->digest, s2.digest, SIZE) < 0;
    }
    bool operator==(const hash &s2) const {
	if(this->digest[0] != s2.digest[0]) return false;
	return memcmp(this->digest, s2.digest, SIZE) == 0;
    }
    friend std::ostream& operator<<(std::ostream& os,const hash &s2) {
        os << s2.hexdigest();
        return os;
    }
};


typedef hash<16> md5_t;
typedef hash<20> sha1_t;
typedef hash<32> sha256_t;
#ifdef HAVE_SHA512_T
typedef hash<64> sha512_t;
#endif

/* Now that we have our types defined,
 * we define a function that returns the name of the digest.
 * These work for either type
 */
inline std::string digest_implementation_name() { return HASH_IMPLEMENTATION_NAME; }
template<typename T> inline std::string digest_name();
template<> inline std::string digest_name<md5_t>() { return "MD5"; }
template<> inline std::string digest_name<sha1_t>() { return "SHA1"; }
template<> inline std::string digest_name<sha256_t>() { return "SHA256"; }
#ifdef HAVE_SHA512
template<> inline std::string digest_name<sha512_t>() { return "SHA512"; }
#endif


/*
 * This is the templated hash generator.
 * It needs to be specialized for the implementation.
 * It would be cool to do this with subclasses from a common base class, but
 * instead we use the preprocessor.
 *
 * With OpenSSL, it's a cover for the EVP_MD system.
 * With CommonCrypto, we just call the functions directly.
 */

#ifdef USE_GCRYPT
template<enum gcry_md_algos mac_algo,size_t SIZE>
class hash_generator__ { 			/* generates the hash */
 private:
    gcry_md_hd_t h {};                     // the hash value
    bool finalized {false};
    uint8_t *digest_ {nullptr};               // created
    /* Not allowed to copy; these are prototyped but not defined,
     * so any attempt to use them will fail, but we won't get the -Weffc++ warnings
     */
    hash_generator__ & operator=(const hash_generator__ &);
    hash_generator__(const hash_generator__ &);
public:
    int64_t hashed_bytes {0};
    /* This function takes advantage of the fact that different hash functions produce residues with different sizes */
    hash_generator__(){
        gcry_md_open(&h, mac_algo, 0);
    }

    ~hash_generator__(){
        gcry_md_close(h);
    }
public:
    void update(const uint8_t *buf,size_t bufsize){
	if(finalized){
	    std::cerr << "hashgen_t::update called after finalized\n";
	    exit(1);
	}
        gcry_md_write(h, buf, bufsize);
	hashed_bytes += bufsize;
    }
    hash<SIZE> digest() {
	if(!finalized){
            digest_ = gcry_md_read(h, mac_algo);
            finalized = true;
        }
        return hash<SIZE>(digest_);
    }

    /** Compute a sha1 from a buffer and return the hash */
    static hash<SIZE>  hash_buf(const uint8_t *buf,size_t bufsize){
	hash_generator__ g;
	g.update(buf, bufsize);
	return g.digest();
    }

#ifdef HAVE_MMAP
    static hash<SIZE> hash_file(const char *fname){
	int fd = open(fname,O_RDONLY | HASHT_O_BINARY );
	if(fd<0) throw fserror("open",errno);
	struct stat st;
	if(fstat(fd,&st)<0){
	    close(fd);
	    throw fserror("fstat",errno);
	}
	const uint8_t *buf = (const uint8_t *)mmap(0,st.st_size,PROT_READ,MAP_FILE|MAP_SHARED,fd,0);
	if(buf==0){
	    close(fd);
	    throw fserror("mmap",errno);
	}
	hash<SIZE> s = hash_buf(buf,st.st_size);
	munmap((void *)buf,st.st_size);
	close(fd);
	return s;
    }
#endif
};
typedef hash_generator__<GCRY_MD_MD5,16>    md5_generator;
typedef hash_generator__<GCRY_MD_SHA1,20>   sha1_generator;
typedef hash_generator__<GCRY_MD_SHA256,32> sha256_generator;
typedef hash_generator__<GCRY_MD_SHA512,64> sha512_generator;
#endif

#ifdef USE_OPENSSL
template<const EVP_MD *md(),size_t SIZE>
class hash_generator__ { 			/* generates the hash */
 private:
    EVP_MD_CTX* mdctx;	     /* the context for computing the value */
    bool finalized;
    uint8_t digest_[SIZE];               // created
    /* Not allowed to copy; these are prototyped but not defined,
     * so any attempt to use them will fail, but we won't get the -Weffc++ warnings
     */
    hash_generator__ & operator=(const hash_generator__ &);
    hash_generator__(const hash_generator__ &);
public:
    int64_t hashed_bytes;
    /* This function takes advantage of the fact that different hash functions produce residues with different sizes */
    hash_generator__():mdctx(NULL),finalized(false),digest_(),hashed_bytes(0){
#ifdef HAVE_EVP_MD_CTX_NEW
        mdctx = EVP_MD_CTX_new();
#else
        mdctx = EVP_MD_CTX_create();
#endif
        if (!mdctx) throw std::bad_alloc();
        EVP_DigestInit_ex(mdctx, md(), NULL);
    }

    ~hash_generator__(){
#ifdef HAVE_EVP_MD_CTX_FREE
        EVP_MD_CTX_free(mdctx);
#else
        EVP_MD_CTX_destroy(mdctx);
#endif
    }
public:
    void update(const uint8_t *buf,size_t bufsize){
	if(finalized){
	    std::cerr << "hashgen_t::update called after finalized\n";
	    exit(1);
	}
	EVP_DigestUpdate(mdctx,buf,bufsize);
	hashed_bytes += bufsize;
    }
    hash<SIZE> digest() {
	if(!finalized){
            unsigned int md_len = SIZE;
            EVP_DigestFinal(mdctx,digest_,&md_len);
            finalized = true;
        }
        return hash<SIZE>(digest_);
    }

    /** Compute a sha1 from a buffer and return the hash */
    static hash<SIZE>  hash_buf(const uint8_t *buf,size_t bufsize){
	/* First time through find the SHA1 of 512 NULLs */
	hash_generator__ g;
	g.update(buf,bufsize);
	return g.digest();
    }

#ifdef HAVE_MMAP
    static hash<SIZE> hash_file(const char *fname){
	int fd = open(fname,O_RDONLY | HASHT_O_BINARY );
	if(fd<0) throw fserror("open",errno);
	struct stat st;
	if(fstat(fd,&st)<0){
	    close(fd);
	    throw fserror("fstat",errno);
	}
	const uint8_t *buf = (const uint8_t *)mmap(0,st.st_size,PROT_READ,MAP_FILE|MAP_SHARED,fd,0);
	if(buf==0){
	    close(fd);
	    throw fserror("mmap",errno);
	}
	hash<SIZE> s = hash_buf(buf,st.st_size);
	munmap((void *)buf,st.st_size);
	close(fd);
	return s;
    }
#endif
};
typedef hash_generator__<EVP_md5,16>    md5_generator;
typedef hash_generator__<EVP_sha1,20>   sha1_generator;
typedef hash_generator__<EVP_sha256,32> sha256_generator;
#ifdef HAVE_SHA512_T
typedef hash_generator__<EVP_sha512,64> sha512_generator;
#endif
#endif

#ifdef USE_COMMON_CRYPTO
/* a simpler implementation because there is no need to allocate and then free the
 * hash contexts.
 */
template<class CC_CTX,int Init(CC_CTX *),int Update(CC_CTX *,const void *,CC_LONG),
         int Final(unsigned char *,CC_CTX *),size_t SIZE>
class hash_generator__ { 			/* generates the hash */
 private:
    CC_CTX c;
    bool finalized;
    uint8_t digest_[SIZE];               // created
    /* Not allowed to copy; these are prototyped but not defined,
     * so any attempt to use them will fail, but we won't get the -Weffc++ warnings
     */
    hash_generator__ & operator=(const hash_generator__ &);
    hash_generator__(const hash_generator__ &);
public:
    int64_t hashed_bytes;
    /* This function takes advantage of the fact that different hash functions produce residues with different sizes */
    hash_generator__():c(),finalized(false),digest_(),hashed_bytes(0){
        Init(&c);
    }
    ~hash_generator__(){
    }
    void update(const uint8_t *buf,size_t bufsize){
	if(finalized){
	    std::cerr << "hashgen_t::update called after finalized\n";
	    exit(1);
	}
	Update(&c, buf, bufsize);
	hashed_bytes += bufsize;
    }
    hash<SIZE> digest() {
	if(!finalized){
            Final(digest_, &c);
            finalized = true;
        }
	return hash<SIZE>(digest_);
    }

    /** Compute a hash from a buffer and return the hash */
    static hash<SIZE>  hash_buf(const uint8_t *buf, size_t bufsize){
	hash_generator__ g;
	g.update(buf, bufsize);
	return g.digest();
    }

#ifdef HAVE_MMAP
    /** Static method allocator */
    static hash<SIZE> hash_file(const char *fname){
	int fd = open(fname, O_RDONLY | HASHT_O_BINARY );
	if (fd<0) throw fserror("open",errno);
	struct stat st;
	if (fstat(fd,&st)<0){
	    close(fd);
	    throw fserror("fstat",errno);
	}
	const uint8_t *buf = (const uint8_t *)mmap(0,st.st_size,PROT_READ,MAP_FILE|MAP_SHARED,fd,0);
	if(buf==0){
	    close(fd);
	    throw fserror("mmap",errno);
	}
	hash<SIZE> s = hash_buf(buf,st.st_size);
	munmap((void *)buf,st.st_size);
	close(fd);
	return s;
    }
#endif
};
typedef hash_generator__<CC_MD5_CTX,CC_MD5_Init,CC_MD5_Update,CC_MD5_Final,16> md5_generator;
typedef hash_generator__<CC_SHA1_CTX,CC_SHA1_Init,CC_SHA1_Update,CC_SHA1_Final,20> sha1_generator;
typedef hash_generator__<CC_SHA256_CTX,CC_SHA256_Init,CC_SHA256_Update,CC_SHA256_Final,32> sha256_generator;
typedef hash_generator__<CC_SHA512_CTX,CC_SHA512_Init,CC_SHA512_Update,CC_SHA512_Final,64> sha512_generator;
#endif

}

#endif
