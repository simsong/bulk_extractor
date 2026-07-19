/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * Tools for working with Unicode
 */

#ifndef UNICODE_ESCAPE_H
#define UNICODE_ESCAPE_H

#include <cstdint>
#include <cstring>
#include <cwctype>
#include <iostream>
#include <locale>
#include <string>

#include "utf8.h"

/** \addtogroup bulk_extractor_APIs
 * @{
 */
/** \file */

/* Our standard escaping is \\ for backslash and \000 for null, \001 for control-a, etc. */

std::string octal_escape(unsigned char ch);       // escape this character
bool utf8cont(unsigned char ch);            // true if a UTF8 continuation character
bool valid_utf8codepoint(uint32_t unichar); // not all unichars are valid codepoints

/* Our internal, testable, somewhat broken Unicode handling */
const std::u32string utf32_lowercase(const std::u32string& str);
const std::u32string utf32_extract_numeric(const std::u32string& str);

struct unicode {
    static const uint16_t INTERLINEAR_ANNOTATION_ANCHOR = 0xFFF9;
    static const uint16_t INTERLINEAR_ANNOTATION_SEPARATOR = 0xFFFA;
    static const uint16_t INTERLINEAR_ANNOTATION_TERMINATOR = 0xFFFB;
    static const uint16_t OBJECT_REPLACEMENT_CHARACTER = 0xFFFC;
    static const uint16_t REPLACEMENT_CHARACTER = 0xFFFD;
    static const uint16_t BOM = 0xFEFF;
};

/* Create safe UTF8 from unsafe UTF8.
 * if validate is true and the others are false, throws an exception with bad UTF8.
 */
class BadUnicode : public std::exception {
    std::string bad_string{};
public:
    BadUnicode(std::string_view bad) : bad_string(bad) {};
    const char *what() const noexcept override { return bad_string.c_str(); };
};

std::string validateOrEscapeUTF8(const std::string& input, bool escape_bad_UTF8, bool escape_backslash, bool validate);

/* Guess if this is valid utf16 and return likely endian */
bool looks_like_utf16(const std::string& str, bool& little_endian);

/* These return the string. If no conversion is possible,
 * they throw const utf8::invalid_utf16.
 * catch with 'catch (const utf8::invalid_utf16 &)'
 */

std::string convert_utf16_to_utf8(const std::string& str, bool little_endian); // request specific conversion
std::string convert_utf16_to_utf8(const std::string& str);                     // guess for best

// std::u32string convert_utf16_to_utf32(const std::string &str,bool little_endian); // request specific conversion

// std::u32string convert_utf8_to_utf32(const std::string &str);
// std::string convert_utf32_to_utf8(const std::u32string &str);
// std::string convert_utf32_to_utf8(const std::u32string &str);
std::u32string convert_utf16_to_utf32(const std::string& str);
std::u16string convert_utf32_to_utf16(const std::u32string& str);
std::string make_utf8(const std::string& str); // returns valid, escaped UTF8 for utf8 or utf16

inline const std::u32string utf32_lowercase(const std::u32string& str) {
    std::u32string output;
    for (auto& ch : str) { output.push_back(ch < 0xffff ? tolower(ch) : ch); }
    return output;
}

inline const std::u32string utf32_extract_numeric(const std::u32string& str) {
    std::u32string output;
    for (auto& ch : str) {
        if (iswdigit(ch)) { output.push_back(ch); }
    }
    return output;
}

/* Now we just pass through to utf8 */
inline const std::u16string convert_utf8_to_utf16(const std::string& utf8) {
    return utf8::utf8to16(utf8);
}

inline const std::u32string convert_utf8_to_utf32(const std::string utf8) {
    return utf8::utf8to32(utf8);
}

inline const std::string convert_utf32_to_utf8(const std::u32string& u32s) {
    return utf8::utf32to8(u32s);
}

inline std::string safe_utf16to8(std::wstring s) { // needs to be cleaned up
    std::string utf8_line;
    try {
        utf8::utf16to8(s.begin(), s.end(), back_inserter(utf8_line));
    } catch (const utf8::invalid_utf16&) {
        /* Exception thrown: bad UTF16 encoding */
        utf8_line = "";
    }
    return utf8_line;
}

// This needs to be cleaned up:
inline std::wstring safe_utf8to16(std::string s) {
    std::wstring utf16_line;
    try {
        utf8::utf8to16(s.begin(), s.end(), back_inserter(utf16_line));
    } catch (const utf8::invalid_utf8&) {
        /* Exception thrown: bad UTF8 encoding */
        utf16_line = L"";
    }
    return utf16_line;
}



#endif
