#ifndef _FPOS0_H_
#define _FPOS0_H_

#include <exception>
#include <algorithm>
#include <cinttypes>
#include <cctype>
#include <sstream>
#include <string>
#include <filesystem>

/****************************************************************
 *** pos0_t
 ****************************************************************/

/** \addtogroup bulk_extractor_APIs
 * @{
 */
/** \file */
/**
 * \class pos0_t
 * The pos0_t structure is used to record the forensic path of the
 * first byte of an sbuf. The forensic path can include strings associated
 * with decompressors and ordinals associated with offsets.
 *
 * e.g., 1000-GZIP-300-BASE64-30 means go 1000 bytes into the stream,
 *       unzip, go 300 bytes into the decompressed stream, un-BASE64, and
 *       go 30 bytes into that.
 *
 * pos0_t uses a string to hold the base path and the offset into that path
 * in a 64-bit number.
 */

inline int64_t stoi64(std::string str) {
    int64_t val(0);
    std::istringstream ss(str);
    ss >> val;
    return val;
}

class pos0_t {
    mutable int depth_ {-1};               // if -1, it needs to be calculated. 0 is top.

public:
    inline static const std::string U10001C = "\xf4\x80\x80\x9c"; // default delimeter character in bulk_extractor
    static std::string map_file_delimiter;                        // character placed
    static void set_map_file_delimiter(const std::string new_delim) { map_file_delimiter = new_delim; }
    const std::string path{}; /* forensic path of decoders*/
    const uint64_t offset{0}; /* location of buf[0] */

    explicit pos0_t() {}                                                 // the beginning of a nothing
    explicit pos0_t(std::string s, uint64_t o = 0) : path(s), offset(o) {}        // s can be a full path
    explicit pos0_t(std::filesystem::path fn, std::string s, uint64_t o = 0) :
        path(fn.string() + pos0_t::map_file_delimiter + s), offset(o) {}
    pos0_t(const pos0_t& obj) : path(obj.path), offset(obj.offset) {}    // copy operator

    /* Every new layer is indicated by a "-" followed by a letter.
     * This threadsafe, but it may need to be computed twice if two
     * computations are happening at the same time.
     */
    static unsigned int calc_depth(const std::string& s)  {
        if (s.size()<2) {
            return 0;
        }
        unsigned int cdepth = 0;
        for( size_t i = 0; i<s.size()-1; i++ ){
            if (s[i]=='-' && isupper(s[i+1])) {
                cdepth += 1;
            }
        }
        return cdepth;
    }
    unsigned int depth() const {
        if (depth_ == -1 ){
            depth_ = calc_depth(path);
        }
        return depth_;
    }

    std::string str() const { // convert to a string, with offset included
        std::stringstream ss;
        if (path.size() > 0) { ss << path << "-"; }
        ss << offset;
        return ss.str();
    }
    bool isRecursive() const { // is there a path?
        return path.size() > 0;
    }
    bool contains(const std::string &name) const { // does it contain this name?
        return (path.find(name) != std::string::npos);
    }

    std::string firstPart() const { // the first part of the path
        size_t p = path.find('-');
        if (p == std::string::npos) return std::string("");
        return path.substr(0, p);
    }
    std::string lastAddedPart() const { // the last part of the path, before the offset
        size_t p = path.rfind('-');
        if (p == std::string::npos) return std::string("");
        return path.substr(p + 1);
    }
    std::string alphaPart() const { // return the non-numeric parts, with /'s between each
        std::string desc;
        bool inalpha = false;
        /* Now get the std::string part of pos0 */
        for (const auto &it : path) {
            if ((it) == '-') {
                if (desc.size() > 0 && desc.at(desc.size() - 1) != '/') desc += '/';
                inalpha = false;
            }
            if (isalpha(it) || (inalpha && isdigit(it))) {
                desc += it;
                inalpha = true;
            }
        }
        return desc;
    }
    uint64_t imageOffset() const { // return the offset from start of disk
        if (path.size() > 0) return stoi64(path);
        return offset;
    }

    /**
     * Return a new position that's been shifted by an offset
     */
    pos0_t shift(int64_t s) const {
        if (s == 0) return *this;
        size_t p = path.find('-');
        if (p == std::string::npos) { // no path
            return pos0_t("", offset + s);
        }
        /* Figure out the value of the shift */
        int64_t baseOffset = stoi64(path.substr(0, p - 1));
        std::stringstream ss;
        ss << (baseOffset + s) << path.substr(p);
        return pos0_t(ss.str(), offset);
    }
};

/** iostream support for the pos0_t */
inline std::ostream& operator<<(std::ostream& os, const class pos0_t& pos0) {
    os << "(" << pos0.path << "|" << pos0.offset << ")";
    return os;
}

/** Append a string (subdir).
 * The current offset is a prefix to the subdir.
 */
inline class pos0_t operator+(pos0_t pos, const std::string& subdir) {
    std::stringstream ss;
    ss << pos.path << (pos.path.size() > 0 ? "-" : "") << pos.offset << "-" << subdir;
    return pos0_t(ss.str(), 0);
};

/** Adding an offset */
inline class pos0_t operator+(pos0_t pos, size_t delta) {
    return pos0_t(pos.path, pos.offset + delta);
};

/** Subtracting an offset */
inline class pos0_t operator-(pos0_t pos, size_t delta) {
    if (delta > pos.offset) {
        throw std::runtime_error("attempt to subtract a delta from an pos0_t that is larger that pos.offset");
    }
    return pos0_t(pos.path, pos.offset - delta);
};

/** \name Comparision operations
 * @{
 */
inline bool operator<(const class pos0_t& pos0, const class pos0_t& pos1) {
    if (pos0.path.size() == 0 && pos1.path.size() == 0) return pos0.offset < pos1.offset;
    if (pos0.path == pos1.path) return pos0.offset < pos1.offset;
    return pos0.path < pos1.path;
};

inline bool operator>(const class pos0_t& pos0, const class pos0_t& pos1) {
    if (pos0.path.size() == 0 && pos1.path.size() == 0) return pos0.offset > pos1.offset;
    if (pos0.path == pos1.path) return pos0.offset > pos1.offset;
    return pos0.path > pos1.path;
};

inline bool operator==(const class pos0_t& pos0, const class pos0_t& pos1) {
    return pos0.path == pos1.path && pos0.offset == pos1.offset;
};

inline bool operator!=(const class pos0_t& pos0, const class pos0_t& pos1) { return !(pos0 == pos1); };
/** @} */
#endif
