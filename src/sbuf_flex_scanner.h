/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* sbuf_flex_scanner.h:
 * Used to build a C++ class that can interoperate with sbufs.
 * Previously we used the flex c++ option, but it had cross-platform problems.
 */

#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"

#ifdef HAVE_DIAGNOSTIC_EFFCPP
#pragma GCC diagnostic ignored "-Weffc++"
#endif

#define YY_NO_INPUT

/* Needed for flex: */
#define ECHO {}                   /* Never echo anything */
#define YY_SKIP_YYWRAP            /* Never wrap */
#define YY_NO_INPUT

#include "config.h"
#include "be20_api/sbuf.h"
#include "be20_api/scanner_params.h"
#include "be20_api/scanner_set.h"

/* should this become a template? */
class sbuf_scanner {
public:
    class sbuf_scanner_exception: public std::exception {
    private:
        std::string m_error;
    public:
        sbuf_scanner_exception(std::string error):m_error(error){}
        const char* what() const noexcept {
            return m_error.c_str();
        }
    };
    explicit sbuf_scanner(const sbuf_t &sbuf_): sbuf(sbuf_){
        sbuf_buf = sbuf.get_buf();      // unsafe but fast
    }
    virtual ~sbuf_scanner(){}
    const sbuf_t &sbuf;
    const uint8_t *sbuf_buf {nullptr};
    // pos & point may be redundent.
    // pos counts the number of bytes into the buffer and is incremented by the flex rules
    // point counts the point where we are removing characters
    size_t pos   {0};  /* The position regex is matching from---visible for C++ called by Flex */
    size_t point {0};  /* The position we are reading from---visible to Flex machine */
    bool   first {true}; /* True if we have sent the first character */

    size_t get_input(char *buf, size_t max_size){
        if ((int)max_size < 0) return 0;
        if (point > sbuf.bufsize) return 0;
        int count=0;

        /* Provide a leading space the first time through */
        if (first && max_size>0) {
            *buf++ = ' ';
            max_size--;
            count++;
            first = false;
        }

        while ((max_size > 0) && (point < sbuf.bufsize) ){
            *buf++ = sbuf_buf[point++];
            max_size--;
            count++;
        }
        /* Provide an extra space at the end, so that regular expressions that specify "/<text>" always find an end */
        if (point==sbuf.bufsize && max_size>0){
            *buf++ = ' ';
            point++;
            max_size--;
            count++;
        }
        return count;
    };

    void check_margin() {
        if (pos >= sbuf.pagesize ) {
            // throw margin_reached();
            point = sbuf.bufsize+1;
        }
    }

};

#define YY_INPUT(buf,result,max_size) result = get_extra(yyscanner)->get_input(buf,max_size);
#define YY_FATAL_ERROR(msg) {throw sbuf_scanner::sbuf_scanner_exception(msg);}
#define SBUF (s.sbuf)
#define POS  (s.pos-1)

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
