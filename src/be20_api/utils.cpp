/**
 * A collection of utility functions that are typically provided,
 * but which are missing in some implementations.
 */

// Just for this module
//#define _FILE_OFFSET_BITS 64

#include "config.h"
#include "utils.h"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <filesystem>

#include <mutex>
#include <sstream>

/** Extract a buffer...
 * @param buf - the buffer to extract;
 * @param buflen - the size of the page to extract
 * @param pos0 - the byte position of buf[0]
 */

#ifndef HAVE_LOCALTIME_R
/* locking localtime_r implementation */
std::mutex localtime_mutex;
void localtime_r(time_t* t, struct tm* tm) {
    const std::lock_guard<std::mutex> lock(localtime_mutex);
    *tm = *localtime(t);
}
#endif

#ifndef HAVE_GMTIME_R
/* locking gmtime_r implementation */
std::mutex gmtime_mutex;
void gmtime_r(time_t* t, struct tm* tm) {
    if (t && tm) {
        const std::lock_guard<std::mutex> lock(gmtime_mutex);
        struct tm* tmret = gmtime(t);
        if (tmret) {
            *tm = *tmret;
        } else {
            memset(tm, 0, sizeof(*tm));
        }
    }
}
#endif

bool getenv_debug(const char *name)
{
    const char *e = std::getenv(name);
    if (e==nullptr) return false;
    if (e[0]=='1' || e[0]=='t' || e[0]=='T' || e[0]=='y' || e[0]=='Y') return true;
    return false;
}

bool starts_with(const std::string& buf, const std::string& with) {
    size_t buflen = buf.size();
    size_t withlen = with.size();
    return buflen > withlen && buf.substr(0,withlen) == with;
}

bool ends_with(const std::string& buf, const std::string& with) {
    size_t buflen = buf.size();
    size_t withlen = with.size();
    return buflen > withlen && buf.substr(buflen - withlen, withlen) == with;
}

bool ends_with(const std::wstring& buf, const std::wstring& with) {
    size_t buflen = buf.size();
    size_t withlen = with.size();
    return buflen > withlen && buf.substr(buflen - withlen, withlen) == with;
}

/****************************************************************/
/* C++ string splitting code from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) { elems.push_back(item); }
    return elems;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

/* Read all of the lines of a file and return them as a vector */
std::vector<std::string> getLines(const std::filesystem::path path)
{
    std::vector<std::string> lines;
    std::string line;
    std::ifstream inFile;
    inFile.open( path );
    if (!inFile.is_open()) {
        std::cerr << "getLines: Cannot open file: " << path << "\n";
        std::string cmd("ls -l " + path.parent_path().string());
        std::cerr << cmd << "\n";
        if (system( cmd.c_str())) {
            std::cerr << "error\n";
        }
        throw std::runtime_error("test_be:getLines");
    }
    while (std::getline(inFile, line)){
        if (line.size()>0){
            lines.push_back(line);
        }
    }
    return lines;
}

// returns the last line if v has more than one line, otherwise ''
std::string getLast(const std::vector<std::string> &v)
{
    if (v.size() > 0) return v[v.size()-1];
    return std::string();
}


uint64_t scaled_stoi64(const std::string &str)
{
    std::stringstream ss(str);
    uint64_t val;
    ss >> val;
    if(str.find('k')!=std::string::npos  || str.find('K')!=std::string::npos) val *= 1024LL;
    if(str.find('m')!=std::string::npos  || str.find('m')!=std::string::npos) val *= 1024LL * 1024LL;
    if(str.find('g')!=std::string::npos  || str.find('g')!=std::string::npos) val *= 1024LL * 1024LL * 1024LL;
    if(str.find('t')!=std::string::npos  || str.find('T')!=std::string::npos) val *= 1024LL * 1024LL * 1024LL * 1024LL;
    return val;
}
