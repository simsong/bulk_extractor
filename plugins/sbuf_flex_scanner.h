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

//2018-03-03: slg - this doesn't seem to be an issue anymore
//#ifdef HAVE_DIAGNOSTIC_DEPRECATED_REGISTER
//#pragma GCC diagnostic ignored "-Wdeprecated-register"
//#endif

#define YY_NO_INPUT

/* Needed for flex: */
#define ECHO {}                   /* Never echo anything */
#define YY_SKIP_YYWRAP            /* Never wrap */
#define YY_NO_INPUT

class sbuf_scanner {
public:
    class sbuf_scanner_exception: public std::exception {
    public:
        const char *msg;
        sbuf_scanner_exception(const char *m):msg(m){}
        virtual const char *what() const throw() {
            return msg;
        }
    };
    explicit sbuf_scanner(const sbuf_t *sbuf_): sbuf(sbuf_),pos(0),point(0){}
    virtual ~sbuf_scanner(){}
    const sbuf_t *sbuf;
    // pos & point may be redundent.
    // pos counts the number of bytes into the buffer and is incremented by the flex rules
    // point counts the point where we are removing characters
    size_t pos;    /* The position regex is matching from---visible for C++ called by Flex */
    size_t point;  /* The position we are reading from---visible to Flex machine */

    size_t get_input(char *buf,size_t max_size){
        if((int)max_size < 0) return 0;
        int count=0;
        while ((max_size > 0) && (point < sbuf->bufsize) /* && (pos < sbuf->pagesize) */ ){
            *buf++ = (char)sbuf->buf[point++];
            max_size--;
            count++;
        }
        return count;
    };
};

#define YY_INPUT(buf,result,max_size) result = get_extra(yyscanner)->get_input(buf,max_size);
#define POS  s.pos
#define SBUF (*s.sbuf)
#define YY_FATAL_ERROR(msg) {throw sbuf_scanner::sbuf_scanner_exception(msg);}

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
