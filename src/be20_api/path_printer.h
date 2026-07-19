#ifndef PATH_PRINTER_H
#define PATH_PRINTER_H

#include <string>
#include <map>

#include "scanner_params.h"
#include "abstract_image_reader.h"

// C++ does not allow forward references on nested classes.
// "You can't do it, it's a hole in the C++ language. You'll have to un-nest at least one of the nested classes."
// https://stackoverflow.com/questions/951234/forward-declaration-of-nested-types-classes-in-c

struct PrintOptions : public std::map<std::string, std::string> {
    static inline const std::string HTTP_EOL {"\r\n"};		// stdout is in binary form
    static inline const size_t DEFAULT_BUFSIZE = 16384;
    enum print_mode_t { MODE_NONE = 0, MODE_HEX, MODE_RAW, MODE_HTTP };
    print_mode_t print_mode {MODE_NONE};
    size_t process_path_bufsize {DEFAULT_BUFSIZE};
    bool http_mode {false};
    std::string get(std::string key, std::string default_) const;
    void add_rfc822_header(std::ostream &os, std::string line);
    size_t content_length {0};
};


class path_printer {
    class scanner_set &ss;
    abstract_image_reader *reader {nullptr};
    mutable std::stringstream os {};    // for temp creation
    std::ostream &out;                  // for output
    path_printer(const path_printer &) = delete;
    path_printer &operator=(const path_printer &) = delete;

public:;
    class path_printer_finished: public std::exception {
    public:
        virtual const char *what() const throw() {
            return "path printer finished.";
        }
    };



    path_printer(scanner_set &ss_, abstract_image_reader *reader_, std::ostream &out);
    static inline const std::string PRINT {"PRINT"};
    static inline const std::string CONTENT_LENGTH {"Content-Length"};
    static inline const std::string DEFAULT_CONTENT_LENGTH {"4096"};

    static std::string lowerstr(const std::string str);
    static std::string get_and_remove_token(std::string &path);

    void process_sp( const scanner_params &sp ) const; // called recursively by sp.recurse()
    void display_path( std::string path, const PrintOptions &po) const; // entry point for process() command

    void process_path(std::string path) ;     // main entrance point to display a path, output to os
    void process_interactive(std::istream &is) ;          // run an interactive server on is
    void process_http(std::istream &is); // read an HTTP command from is and send result to os
};

#endif
