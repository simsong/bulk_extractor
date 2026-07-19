/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * unicode_escape.cpp:
 * Escape unicode that is not valid.
 *
 * References:
 * http://www.ietf.org/rfc/rfc3987.txt
 * http://en.wikipedia.org/wiki/UTF-8
 *
 * @author Simson Garfinkel
 *
 * The software provided here is released by the Naval Postgraduate
 * School, an agency of the U.S. Department of Navy.  The software
 * bears no warranty, either expressed or implied. NPS does not assume
 * legal liability nor responsibility for a User's use of the software
 * or the results of such use.
 *
 * Please note that within the United States, copyright protection,
 * under Section 105 of the United States Code, Title 17, is not
 * available for any work of the United States Government and/or for
 * any works created by United States Government employees. User
 * acknowledges that this software contains work which was created by
 * NPS government employees and is therefore in the public domain and
 * not subject to copyright.
 */

#include <cassert>
//#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>

#include "unicode_escape.h"
#include "utf8.h"

std::string octal_escape(unsigned char ch) {
    char buf[8];                        // [5] would be sufficient.
    snprintf(buf, sizeof(buf), "\\%03o", ch);
    return std::string(buf);
}

/** returns true if this is a UTF8 continuation character */
bool utf8cont(unsigned char ch) { return ((ch & 0x80) == 0x80) && ((ch & 0x40) == 0); }

/**
 * After a UTF-8 sequence is decided, this function is called
 * to determine if the character is invalid. The UTF-8 spec now
 * says that if a UTF-8 decoding produces an invalid character, or
 * a surrogate, it is not valid. (There were some nasty security
 * vulnerabilities that were exploited before this came out.)
 * So we do a lot of checks here.
 */
bool valid_utf8codepoint(uint32_t unichar) {
    // Check for invalid characters in the bmp
    switch (unichar) {
    case 0xfffe: return false; // reversed BOM
    case 0xffff: return false;
    default: break;
    }
    if (unichar >= 0xd800 && unichar <= 0xdfff) return false; // high and low surrogates
    if (unichar < 0x10000) return true;                       // looks like it is in the BMP

    // check some regions outside the bmp

    // Plane 1:
    if (unichar > 0x13fff && unichar < 0x16000) return false;
    if (unichar > 0x16fff && unichar < 0x1b000) return false;
    if (unichar > 0x1bfff && unichar < 0x1d000) return false;

    // Plane 2
    if (unichar > 0x2bfff && unichar < 0x2f000) return false;

    // Planes 3--13 are unassigned
    if (unichar >= 0x30000 && unichar < 0xdffff) return false;

    // Above Plane 16 is invalid
    if (unichar > 0x10FFFF) return false; // above plane 16?

    return true; // must be valid
}

/**
 * validateOrEscapeUTF8
 * Input: UTF8 string (possibly corrupt)
 * Input: do_escape, indicating whether invalid encodings shall be escaped.
 * Note:
 *    - if not escaping but an invalid encoding is present and DEBUG_PEDANTIC is set, then assert() is called.
 *    - DO NOT USE wchar_t because it is 16-bits on Windows and 32-bits on Unix.
 * Output:
 *   - UTF8 string.  If do_escape is set, then corruptions are escaped in \xFF notation where FF is a hex character.
 */

std::string validateOrEscapeUTF8(const std::string& input, bool escape_bad_utf8, bool escape_backslash,
                                 bool validateOrEscapeUTF8_validate) {
    // skip the validation if not escaping and not DEBUG_PEDANTIC
    if (escape_bad_utf8 == false && escape_backslash == false && validateOrEscapeUTF8_validate == false) {
        return input;
    }

    // validate or escape input
    std::string output;
    for (std::string::size_type i = 0; i < input.length();) {
        uint8_t ch = (uint8_t)input.at(i);

        // utf8 1 byte prefix (0xxx xxxx)
        if ((ch & 0x80) == 0x00) {                // 00 .. 0x7f
            if (ch == '\\' && escape_backslash) { // escape the escape character as \x92
                output += octal_escape(ch);
                i++;
                continue;
            }

            if (ch < ' ') { // not printable are escaped
                output += octal_escape(ch);
                i++;
                continue;
            }
            output += ch; // printable is not escaped
            i++;
            continue;
        }

        // utf8 2 bytes  (110x xxxx) prefix
        if (((ch & 0xe0) == 0xc0) // 2-byte prefix
            && (i + 1 < input.length()) && utf8cont((uint8_t)input.at(i + 1))) {
            uint32_t unichar = (((uint8_t)input.at(i) & 0x1f) << 6) | (((uint8_t)input.at(i + 1) & 0x3f));

            // check for valid 2-byte encoding
            if (valid_utf8codepoint(unichar) && ((uint8_t)input.at(i) != 0xc0) && (unichar >= 0x80)) {
                output += (uint8_t)input.at(i++); // byte1
                output += (uint8_t)input.at(i++); // byte2
                continue;
            }
        }

        // utf8 3 bytes (1110 xxxx prefix)
        if (((ch & 0xf0) == 0xe0) && (i + 2 < input.length()) && utf8cont((uint8_t)input.at(i + 1)) &&
            utf8cont((uint8_t)input.at(i + 2))) {
            uint32_t unichar = (((uint8_t)input.at(i) & 0x0f) << 12) | (((uint8_t)input.at(i + 1) & 0x3f) << 6) |
                               (((uint8_t)input.at(i + 2) & 0x3f));

            // check for a valid 3-byte code point
            if (valid_utf8codepoint(unichar) && unichar >= 0x800) {
                output += (uint8_t)input.at(i++); // byte1
                output += (uint8_t)input.at(i++); // byte2
                output += (uint8_t)input.at(i++); // byte3
                continue;
            }
        }

        // utf8 4 bytes (1111 0xxx prefix)
        if (((ch & 0xf8) == 0xf0) && (i + 3 < input.length()) && utf8cont((uint8_t)input.at(i + 1)) &&
            utf8cont((uint8_t)input.at(i + 2)) && utf8cont((uint8_t)input.at(i + 3))) {
            uint32_t unichar = ((((uint8_t)input.at(i) & 0x07) << 18) | (((uint8_t)input.at(i + 1) & 0x3f) << 12) |
                                (((uint8_t)input.at(i + 2) & 0x3f) << 6) | (((uint8_t)input.at(i + 3) & 0x3f)));

            if (valid_utf8codepoint(unichar) && unichar >= 0x1000000) {
                output += (uint8_t)input.at(i++); // byte1
                output += (uint8_t)input.at(i++); // byte2
                output += (uint8_t)input.at(i++); // byte3
                output += (uint8_t)input.at(i++); // byte4
                continue;
            }
        }

        if (escape_bad_utf8) {
            // Just escape the next byte and carry on
            output += octal_escape((uint8_t)input.at(i++));
        } else {
            // fatal if we are debug pedantic, otherwise just ignore
            // note: we shouldn't be here anyway, since if we are not escaping and we are not
            // pedantic we should have returned above
            if (validateOrEscapeUTF8_validate) {
                throw BadUnicode(input);
            }
        }
    }
    return output;
}

bool looks_like_utf16(const std::string& str, bool& little_endian) {
    /* first check for BOMs */
    if (static_cast<uint8_t>(str[0]) == 0xff && static_cast<uint8_t>(str[1]) == 0xfe) {
        little_endian = true;
        return true; // begins with FFFE
    }
    if (static_cast<uint8_t>(str[0]) == 0xfe && static_cast<uint8_t>(str[1]) == 0xff) {
        little_endian = false;
        return true; // begins with FEFF
    }
    /* If none of the even characters are NULL and some of the odd
     * characters are NULL, it's UTF-16
     */
    uint32_t even_null_count = 0;
    uint32_t odd_null_count = 0;
    for (size_t i = 0; i + 1 < str.size(); i += 2) {
        if (str[i] == 0)     even_null_count++;
        if (str[i + 1] == 0) odd_null_count++;
        /* TODO: Should we look for FFFE or FEFF and act accordingly ? */
    }
    if (even_null_count == 0 && odd_null_count > 1) {
        little_endian = true;
        return true;
    }
    if (odd_null_count == 0 && even_null_count > 1) {
        little_endian = false;
        return true;
    }
    return false;
}

/**
 * Converts a utf16 with a byte order to utf8.
 *
 */
/* static */
std::string convert_utf16_to_utf8(const std::string& key, bool little_endian) {
    /* re-image this string as UTF16*/
    std::u16string utf16;
    for (size_t i = 0; i < key.size(); i += 2) {
        if (little_endian)
            utf16.push_back(key[i] | (key[i + 1] << 8));
        else
            utf16.push_back(key[i] << 8 | (key[i + 1]));
    }
    /* Now convert it to a UTF-8;
     * set tempKey to be the utf-8 string that will be erased.
     */
    std::string tempKey;
    utf8::utf16to8(utf16.begin(), utf16.end(), std::back_inserter(tempKey));
    /* Erase any nulls if present */
    while (tempKey.size() > 0) {
        size_t nullpos = tempKey.find('\000');
        if (nullpos == std::string::npos) break;
        tempKey.erase(nullpos, 1);
    }
    return tempKey;
}

std::string convert_utf16_to_utf8(const std::string& key) {
    bool little_endian = false;
    if (looks_like_utf16(key, little_endian)) {
        return convert_utf16_to_utf8(key, little_endian);
    }
    throw utf8::invalid_utf16(0);
}

std::string make_utf8(const std::string& str) {
    try {
        return convert_utf16_to_utf8(str);
    } catch (const utf8::invalid_utf16&) {
        return validateOrEscapeUTF8(str, true, true, true);
    }
}

/*
 * BULK_EXTRACTOR 2 code uses UTF32, but the utf8 package doesn't provide utf16 to utf32 conversion
 */

// https://stackoverflow.com/questions/23919515/how-to-convert-from-utf-16-to-utf-32-on-linux-with-std-library
inline int is_surrogate(uint16_t uc) { return (uc - 0xd800u) < 2048u; }
inline int is_high_surrogate(uint16_t uc) { return (uc & 0xfffffc00) == 0xd800; }
inline int is_low_surrogate(uint16_t uc) { return (uc & 0xfffffc00) == 0xdc00; }

inline uint32_t surrogate_to_utf32(uint16_t high, uint16_t low) { return (high << 10) + low - 0x35fdc00; }

std::u32string convert_utf16_to_utf32(const std::u16string& input) {
    constexpr uint16_t REPLACEMENT_CHARACTER = 0xfffd;
    std::u32string output;
    for (size_t i = 0; i < input.size(); i++) {
        uint16_t uc = input[i];
        if (!is_surrogate(uc)) {
            output.push_back(uc);
            continue;
        }
        if (is_high_surrogate(uc) && (i + 1) < input.size() && is_low_surrogate(input[i + 1])) {
            output.push_back(surrogate_to_utf32(uc, input[++i]));
            continue;
        }
        output.push_back(REPLACEMENT_CHARACTER);
    }
    return output;
}

/*
 * https://stackoverflow.com/questions/42899410/convert-utf-32-wide-char-to-utf-16-wide-char-in-linux-for-supplementary-plane-ch
 */
std::u16string convert_utf32_to_utf16(const std::u32string& str) {
    /* The conversion from UTF-32 to UTF-16 might possibly require surrogates.
     * A surrogate pair suffices to represent all wide characters, because all
     * characters outside the range will be mapped to the sentinel value
     * U+FFFD.  Add one character for the terminating NUL.
     */
    std::u16string result;

    for (auto ch : str) {
        if (ch < 0 || ch > 0x10FFFF || (ch >= 0xD800 && ch <= 0xDFFF)) {
            // Invalid code point.  Replace with sentinel, per Unicode standard:
            constexpr char16_t sentinel = u'\uFFFD';
            result.push_back(sentinel);
        } else if (ch < 0x10000UL) { // In the BMP.
            result.push_back(static_cast<char16_t>(ch));
        } else {
            const char16_t leading = static_cast<char16_t>(((ch - 0x10000UL) / 0x400U) + 0xD800U);
            const char16_t trailing = static_cast<char16_t>(((ch - 0x10000UL) % 0x400U) + 0xDC00U);
            result.append({leading, trailing});
        }
    }
    return result;
}
