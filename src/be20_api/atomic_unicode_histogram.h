#ifndef ATOMIC_UNICODE_HISTOGRAM_H
#define ATOMIC_UNICODE_HISTOGRAM_H

/** A simple class for making histograms of strings.
 * Histograms are kept in printable UTF-8 representation, not in UTF32 internally.
 * In part this us due to the legacy code base.
 * This part this allows the scanners to determine what the printout should look like, rather than having
 * to pass presentation flags.
 *
 * Histogram maker implement:
 * - Counting
 * - Determining how much memory is in use by histogram.
 * - Writing histogram to a stream (for example, when memory is filled.)
 * - Merging multiple histogram files to a single file.
 *
 * Note - case transitions and text extraction is performed in UTF-32.
 *      - regular expression are then run on the UTF-8. (Not the best, but it works for now.)
 */

#include "atomic_map.h"
#include "histogram_def.h"
#include "unicode_escape.h"
#include <atomic>

struct AtomicUnicodeHistogram {
    static uint32_t debug_histogram_malloc_fail_frequency; // for debugging, make malloc fail sometimes
    struct HistogramTally {
        uint32_t count{0};   // total strings seen
        uint32_t count16{0}; // total utf16 strings seen
        HistogramTally(const HistogramTally& a) {
            this->count = a.count;
            this->count16 = a.count16;
        }
        HistogramTally& operator=(const HistogramTally& a) {
            this->count = a.count;
            this->count16 = a.count16;
            return *this;
        }

        HistogramTally(){};
        virtual ~HistogramTally(){};

        bool operator==(const HistogramTally& a) const { return this->count == a.count && this->count16 == a.count16; };
        bool operator!=(const HistogramTally& a) const { return !(*this == a); }
        bool operator<(const HistogramTally& a) const {
            return (this->count < a.count) || ((this->count == a.count && (this->count16 < a.count16)));
        }
        size_t bytes() const {
            return sizeof(*this);
        }
    };

    /* A FrequencyReportVector is a vector of report elements when the report is generated.*/
    typedef atomic_map<std::string, struct AtomicUnicodeHistogram::HistogramTally> auh_t;
    typedef std::vector<auh_t::item> FrequencyReportVector;

    /* Returns true if a<b for sort order.
     * Sort high counts before low counts, but if the count is the same sort in alphabetical order.
     */
    static bool histogram_compare(const auh_t::item &a, const auh_t::item &b) {
        if (a.value->count > b.value->count) return true;
        if (a.value->count < b.value->count) return false;
        if (a.key < b.key) return true;
        return false;
    }

    AtomicUnicodeHistogram(const struct histogram_def& def_) : def(def_) {}
    virtual ~AtomicUnicodeHistogram(){};

    // is it empty?
    bool empty() {
        const std::lock_guard<std::mutex> lock(M);
        return h.size()==0;
    }
    void clear();                       // empties the histogram
    // low-level add, directly to what we display, if the match function checks out.
    void add0(const std::string& u8key, const std::string &context, bool found_utf16);

     // adds Unicode string to the histogram count. context is used for histogram_def
    void add_feature_context(const std::string& feature, const std::string&context);
    size_t size()  const;              // returns the number of entries in the historam
    size_t bytes() const;              // returns the number of bytes used by the histogram

    /** makeReport() makes a report and returns a
     * FrequencyReportVector.
     */
    std::vector<auh_t::item> makeReport(size_t topN=0);          // returns items of <count,key>
    const struct histogram_def def;            // the definition we are making
    bool  debug {false};                        // set to enable debugging

private:
    mutable std::mutex M {};                    // mutex for the histogram, used to lock individual elements.
    auh_t h {};                         // the histogram
};

std::ostream& operator<<(std::ostream& os, const AtomicUnicodeHistogram::FrequencyReportVector& rep);
std::ostream& operator<<(std::ostream& os, const AtomicUnicodeHistogram::auh_t::item& e);

#endif
