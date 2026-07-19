/**
 * atomic_unicode_histogram.cpp:
 * Maintain a histogram for Unicode strings provided with either UTF-8 or UTF-16 encodings.
 * Track number of UTF-16 encodings provided.
 *
 * Currently, all operations are done on UTF-8 values, because the C++17 regular expression package
 * does not handle 32-bit regular expressions.
 */

#include "unicode_escape.h"
#include "utf8.h"

#include <cwctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#include "atomic_unicode_histogram.h"

std::ostream& operator<<(std::ostream& os, const AtomicUnicodeHistogram::FrequencyReportVector& rep) {
    for (const auto& it : rep) {
        os << it;
    }
    return os;
}

/* Output is in UTF-8 */
std::ostream& operator<<(std::ostream& os, const AtomicUnicodeHistogram::auh_t::item& e) {
    os << "n=" << e.value->count << "\t" << validateOrEscapeUTF8(e.key, true, false, false);
    if (e.value->count16 > 0) os << "\t(utf16=" << e.value->count16 << ")";
    os << "\n";
    return os;
}

/* Create a histogram report.
 * @param topN - if >0, return only this many.
 * Return only the topN.
 */
std::vector<AtomicUnicodeHistogram::auh_t::item> AtomicUnicodeHistogram::makeReport(size_t topN)
{
    const std::lock_guard<std::mutex> lock(M);
    std::vector<AtomicUnicodeHistogram::auh_t::item> ret = h.items();

    std::sort(ret.begin(), ret.end(), AtomicUnicodeHistogram::histogram_compare); // reverse sort

    /* If we only want some of them, delete the extra */
    if ((topN > 0) && (topN < ret.size())) {
        ret.erase( ret.begin()+topN, ret.end());
    }
    return ret;
}

/**
 * Takes a string (the key) passed in, figure out what it is, and add it to a unicode histogram.
 * Typically it is going to be UTF16 or UTF8.
 * Regular expressions are applied, if requested, in the UTF32 world.
 *
 * @param - key - either a UTF8 or UTF16 string.
 * If the string appears to be UTF16, convert it to UTF-8 and note that it was converted.
 *
 * def.flags.digits - extract the digits first and throw away the rest.
 * def.flags.lower  - also convert to lowercase using Unicode rules.
 */

// https://stackoverflow.com/questions/37989081/how-to-use-unicode-range-in-c-regex

// debug_histogram_malloc_fail_frequency allows us to simulate low-memory situations for testing the code.
uint32_t AtomicUnicodeHistogram::debug_histogram_malloc_fail_frequency = 0;
void AtomicUnicodeHistogram::clear()
{
    const std::lock_guard<std::mutex> lock(M);
    h.clear();
}

// low-level add after key has been converted to UTF8
void AtomicUnicodeHistogram::add0(const std::string& u8key, const std::string &context, bool found_utf16)
{
    std::string displayString;

    if (def.match(u8key, &displayString, context)) {

        if (debug) std::cerr << "  AtomicUnicodeHistogram::add0 match u8key=" << u8key << std::endl;

        /* Escape as necessary */
        displayString = validateOrEscapeUTF8(displayString, true, true, false);

        /* For debugging low-memory handling logic,
         * specify DEBUG_MALLOC_FAIL to make malloc occasionally fail (not yet implemented)
         */
        if (debug_histogram_malloc_fail_frequency) {
            const std::lock_guard<std::mutex> lock(M);
            if ((h.size() % debug_histogram_malloc_fail_frequency) == (debug_histogram_malloc_fail_frequency - 1)) {
                throw std::bad_alloc();
            }
        }

        /* Add the key to the histogram. Note that this is threadsafe */
        const std::lock_guard<std::mutex> lock(M);
        h[displayString].count++;
        if (found_utf16) {
            h[displayString].count16++; // track how many UTF16s were converted
        }
        if (debug) std::cerr << "  AtomicUnicodeHistogram::add0 h[" <<displayString << "].count=" << h[displayString].count << std::endl;
    }
}

void AtomicUnicodeHistogram::add_feature_context(const std::string& key_unknown_encoding, const std::string& context)
{
    if (key_unknown_encoding.size() == 0) return; // don't deal with zero-length keys

    /* On input, the key may be UTF8 or UTF16. See if we can figure it out */
    bool found_utf16   = false;   // did we find a utf16?
    bool little_endian = false; // was it little_endian?
    std::u32string u32key;      // u32key. Doesn't matter if LE or BE, because we never write it out.

    if (looks_like_utf16(key_unknown_encoding, little_endian)) {
        // We have an endian-guessing implementation that converts from 16 to 8, so convert from 16 to 8
        // and then convert it to utf32
        u32key = convert_utf8_to_utf32(convert_utf16_to_utf8(key_unknown_encoding, little_endian));
        found_utf16 = true;
    } else {
        u32key = convert_utf8_to_utf32(key_unknown_encoding);
    }

    /* At this point we have UTF-32, which we treat as raw unicode characters.
     *
     * We would like to process lowercase, numeric and regular expressions in utf32 world.
     * Ideally this would be done with ICU, but we do not want to assume we have ICU.
     * https://stackoverflow.com/questions/34433380/lowercase-of-unicode-character
     * https://stackoverflow.com/questions/313970/how-to-convert-stdstring-to-lower-case/24063783
     * https://en.cppreference.com/w/cpp/string/wide/towlower
     * See: http://stackoverflow.com/questions/1081456/wchar-t-vs-wint-t
     *
     * One possibility is the SRELL library, which is included in this repo.
     *
     * Instead, we just convert to UTF-8 and then treat it with the C++17 8-bit regular expression package.
     *
     * See also:
     * https://www.moria.us/articles/wchar-is-a-historical-accident/?
     */

    std::string u8key = convert_utf32_to_utf8(u32key);
    add0(u8key, context, found_utf16);
}

size_t AtomicUnicodeHistogram::size() const // returns the total number of bytes of the histogram,.
{
    const std::lock_guard<std::mutex> lock(M);
    return h.size();
}

size_t AtomicUnicodeHistogram::bytes() const // returns the total number of bytes of the histogram,.
{
    const std::lock_guard<std::mutex> lock(M);
    return sizeof(*this) + h.bytes();
}
