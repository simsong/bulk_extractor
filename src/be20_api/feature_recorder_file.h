#ifndef FEATURE_RECORDER_FILE_H
#define FEATURE_RECORDER_FILE_H

#include "config.h"

#include <cassert>
#include <cinttypes>

#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <mutex>

#include "feature_recorder.h"
#include "pos0.h"
#include "sbuf.h"

class feature_recorder_file : public feature_recorder {
public:
    inline static const std::string feature_file_header   {"# Feature-File-Version: 1.1\n"};
    inline static const std::string histogram_file_header {"# Histogram-File-Version: 1.1\n"};
    inline static const std::string bulk_extractor_version_header {
        "# " PACKAGE_NAME "-Version: " PACKAGE_VERSION "\n"};

    static std::string unquote_string(const std::string& s);

    feature_recorder_file(class feature_recorder_set& fs, const feature_recorder_def def);
    virtual ~feature_recorder_file();
    virtual void flush() override;
    static bool extract_feature_context(const std::string& line, std::string &feature, std::string &context); // extract feature and context, return true if successful
    static bool isodigit(uint8_t ch){
        return ch>='0' && ch<='7';
    }

private:
    std::mutex Mios{};  // mutex for IOS
    std::fstream ios{}; // where features are written

    void banner_stamp(std::ostream& os, const std::string& header) const; // stamp banner, and header

    //static const std::string histogram_file_header;
    //static const std::string feature_file_header;
    //static const std::string bulk_extractor_version_header;

    virtual void shutdown() override;

public:
    /* these are not threadsafe and should only be called in startup */
    // void set_carve_ignore_encoding( const std::string &encoding ){ MAINTHREAD();ignore_encoding = encoding;}
    /* End non-threadsafe */

    // add i to file_number and return the result
    // fetch_add() returns the original number

    /* where stopped items (on stop_list or context_stop_list) get recorded:
     * Cannot be made inline becuase it accesses fs.
     */
    virtual void write0(const std::string& str) override;
    virtual void write0(const pos0_t& pos0, const std::string& feature, const std::string& context) override;

    /* histogram support.
    * The file based feature recorder can store the histogram incrementally in memory or it can make it at the end in a second pass.
    */
    static const inline int MAX_HISTOGRAM_FILES = 10; // don't make more than 10 files in low-memory conditions

    // the histograms are made in memory with the AtomicUnicodeHistogram object.
    // Each one contains the histogram_def.
    std::vector<std::unique_ptr<AtomicUnicodeHistogram>> histograms{};

    virtual size_t histogram_count() override;                 // how many histograms it has
    virtual void histogram_add(const struct histogram_def& def) override;   // add a new histogram

    // Adding features to the histogram

    virtual void histogram_write_from_memory(AtomicUnicodeHistogram& h); // actually write this histogram
    virtual void histogram_write_from_file(AtomicUnicodeHistogram& h); // actually write this histogram
    virtual void histogram_write(AtomicUnicodeHistogram& h); // write this histogram
    virtual void histograms_incremental_add_feature_context(const std::string& feature, const std::string& context) override;
    virtual bool histograms_write_largest() override;
    virtual void histograms_write_all() override;
};

/** @} */

#endif
