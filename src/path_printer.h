#ifndef PATH_PRINTER_H
#define PATH_PRINTER_H

#include "be13_api/scanner_params.h"

class path_printer_finished: public std::exception {
public:
    virtual const char *what() const throw() {
        return "path printer finished.";
    }
} printing_done;

struct path_printer {
    static inline const std::string HTTP_EOL {"\r\n"};		// stdout is in binary form
    static inline const std::string PRINT {"PRINT"};
    static inline const std::string CONTENT_LENGTH {"Content-Length"};

    static std::string path_printer::lowerstr(const std::string str);
    static void process(const scanner_params &sp);
    static void open_path(const image_process &p,std::string path,scanner_params::PrintOptions &po,
                          const size_t process_path_bufsize);

    static void process_path(const char *fn,std::string path,size_t pagesize,size_t marginsize);
};

#endif
