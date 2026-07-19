/****************************************************************
 *** utils.h
 ***
 *** To use utils.c/utils.h, be sure this is in your configure.ac file:
      m4_include([be20_api/be20_configure.m4])
 ***
 ****************************************************************/

#ifndef UTILS_H
#define UTILS_H

#ifndef PACKAGE_NAME
#error utils.h requires that autoconf-generated config.h be included first
#endif

#include <array>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

bool getenv_debug(const char *name);    // look for an environment variable and return TRUE if it is set and not 0 or FALSE
bool starts_with(const std::string& buf, const std::string& with);
bool ends_with(const std::string& buf, const std::string& with);
bool ends_with(const std::wstring& buf, const std::wstring& with);
std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems);
std::vector<std::string> split(const std::string& s, char delim);

/* Read all of the lines of a file and return them as a vector */
std::vector<std::string> getLines(const std::filesystem::path path);
std::string getLast(const std::vector<std::string> &v); // returns the last line if v has more than one line, otherwise ''

inline void truncate_at(std::string& line, char ch) {
    size_t pos = line.find(ch);
    if (pos != std::string::npos) line.resize(pos);
};

inline void set_from_string(int *ret, std::string v) { *ret = std::stoi(v); };
inline void set_from_string(unsigned int *ret, std::string v) { *ret = std::stoul(v); };
inline void set_from_string(uint64_t *ret, std::string v) { *ret = std::stoull(v); };
inline void set_from_string(uint8_t *ret, std::string v) { *ret = std::stoul(v); };

inline void set_from_string(std::string *ret, std::string v) { *ret =  v; };
inline void set_from_string(bool *ret, std::string v) {
    *ret = (v.size()>0 && (v[0]=='Y' || v[0]=='y' || v[0]=='T' || v[0]=='t' || v[0]=='1'));
};



#ifndef HAVE_LOCALTIME_R
#ifdef __MINGW32__
#undef localtime_r
#endif
void localtime_r(time_t* t, struct tm* tm);
#endif

#ifndef HAVE_GMTIME_R
#ifdef __MINGW32__
#undef gmtime_r
#endif
void gmtime_r(time_t* t, struct tm* tm);
#endif

int64_t get_filesize(int fd);

#ifndef HAVE_ISHEXNUMBER
inline int ishexnumber(int c) {
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f': return 1;
    }
    return 0;
}
#endif

#ifndef HAVE_ISXDIGIT
inline int isxdigit(int c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
#endif

/* Useful functions for scanners */
#define ONE_HUNDRED_NANO_SEC_TO_SECONDS 10000000
#define SECONDS_BETWEEN_WIN32_EPOCH_AND_UNIX_EPOCH 11644473600LL
/*
 * 11644473600 is the number of seconds between the Win32 epoch
 * and the Unix epoch.
 *
 * http://arstechnica.com/civis/viewtopic.php?f=20&t=111992
 * gmtime_r() is Linux-specific. You'll find a copy in util.cpp for Windows.
 */

inline std::string microsoftDateToISODate(const uint64_t& time) {
    time_t tmp = (time / ONE_HUNDRED_NANO_SEC_TO_SECONDS) - SECONDS_BETWEEN_WIN32_EPOCH_AND_UNIX_EPOCH;

    struct tm time_tm;
    gmtime_r(&tmp, &time_tm);
    char buf[256];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &time_tm); // Zulu time
    return std::string(buf);
}

/* Convert Unix timestamp to ISO format */
inline std::string unixTimeToISODate(const uint64_t t) {
    struct tm time_tm;
    time_t tmp = t;
    gmtime_r(&tmp, &time_tm);
    char buf[256];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &time_tm); // Zulu time
    return std::string(buf);
}

/* Many internal windows and Linux structures require a valid printable name in ASCII */
inline bool validASCIIName(const std::string name) {
    for (auto ch : name) {
        if (ch & 0x80) return false;  // high bit should not be set
        if (ch < ' ') return false;   // should not be control character
        if (ch == 0x7f) return false; // DEL is not printable
    }
    return true;
}

// https://stackoverflow.com/questions/3379956/how-to-create-a-temporary-directory-in-c
inline std::filesystem::path NamedTemporaryDirectory(unsigned long long max_tries = 1000) {
    std::random_device dev;
    std::mt19937 prng(dev());
    std::uniform_int_distribution<uint64_t> rand(0);
    std::filesystem::path path;
    for (unsigned int i=0; i<max_tries; i++ ){
        std::stringstream ss;
        ss << "be_tmp" << std::hex << rand(prng);
        path = std::filesystem::temp_directory_path() / ss.str();
        if (std::filesystem::create_directory(path)) {
            return path;
        }
    }
    throw std::runtime_error("could not create NamedTemporaryDirectory");
}

inline bool directory_empty(std::filesystem::path path) {
    namespace fs = std::filesystem;
    if (fs::is_directory(path)) {
        for (const auto& it : fs::directory_iterator(path)) {
            (void)it;
            return false;
        }
    }
    return true;
}

uint64_t scaled_stoi64(const std::string &str);

// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
inline std::string subprocess_call(const char* cmd) {
    std::array<char, 4096> buffer;
    std::stringstream ss;
    std::unique_ptr<FILE, int(*)(FILE *)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        ss << buffer.data();
    }
    return ss.str();
}


#ifndef HAVE_STRPTIME
// https://stackoverflow.com/questions/321849/strptime-equivalent-on-windows
inline char* strptime(const char* s, const char* f, struct tm* tm) {
    // Isn't the C++ standard lib nice? std::get_time is defined such that its
    // format parameters are the exact same as strptime. Of course, we have to
    // create a string stream first, and imbue it with the current C locale, and
    // we also have to make sure we return the right things if it fails, or
    // if it succeeds, but this is still far simpler an implementation than any
    // of the versions in any of the C standard libraries.
    std::istringstream input(s);
    input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
    input >> std::get_time(tm, f);
    if (input.fail()) {
        return nullptr;
    }
    return (char*)(s + input.tellg());
}
#endif



#endif
