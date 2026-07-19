/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * regex_vector.h:
 *
 * Now this covers Google's RE2 library.
 * Note:
 *  1 - RE2 and the objects are not move insertable, so we need to manually manage creating and deleting them.
 *  2 - RE2's PartialMatch function wont' return the position of a match unless it is wrapped in a group " () ",
        so we do that.
 */

#ifndef REGEX_VECTOR_H
#define REGEX_VECTOR_H

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <cstdlib>
#include <re2/re2.h>            // it's always here.

#include "config.h"

/**
 * The regex_vector is a vector of character regexes with a few additional convenience functions.
 * We might want to change this to handle ASCII, UTF-16 and UTF-8 characters simultaneously.
 * Only RE2 is supported because it is the only regular expression library that doesn't die on large segments.
 * See: https://swtch.com/~rsc/regexp/regexp3.html#caveats
 */

class regex_vector {
    std::vector<std::string> regex_strings; // the original regex strings
    std::vector<RE2 *> re2_regex_comps;     // the compiled regular expressions
    regex_vector(const regex_vector&) = delete;
    regex_vector& operator=(const regex_vector&) = delete;
    static const std::string RE_ENGINE;

public:
    static bool engine_enabled(const std::string engine) {
        /** each engine is enabled if it is the first to check, or if it is specified */
        return std::getenv(RE_ENGINE.c_str()) == nullptr ||
            std::getenv(RE_ENGINE.c_str())==engine;
    }
    regex_vector() : regex_strings() , re2_regex_comps() {};
    ~regex_vector();

    // is this a regular expression with meta characters?
    static bool has_metachars(const std::string& str);
    const std::string regex_engine(); // which engine is in use

    /* Add a string */
    void push_back(const std::string& val);
    // Empty the vectors. For the compiled, be sure to delete them
    void clear();
    size_t size() const;        // the number of regular expressions in the vector

    /**
     * Read regular expressions from a file: returns 0 if successful, -1 if failure.
     * @param fname - the file to read.
     */
    int readfile(const std::string& fname); // read a file of regexes, one per line

    /** Run Return true if any of the regexes match.
     * search_all() is threadsafe.
     * @param probe  - the string we are searching.
     * *found - set to the found string if something is found.
     */

    bool search_all(const std::string& probe,
                    std::string* found,
                    size_t* offset = nullptr,
                    size_t* len = nullptr) const;
    void dump(std::ostream& os) const;
};

std::ostream& operator<<(std::ostream& os, const class regex_vector& rv);

#endif
