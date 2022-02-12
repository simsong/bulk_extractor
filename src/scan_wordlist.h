#ifndef SCAN_WORDLIST_H
#define SCAN_WORDLIST_H

#include <filesystem>
#include "be20_api/scanner_params.h"
#include "be20_api/atomic_set.h"


/* NOTE: Wordlist is a singleton!
 * It may be called from multiple threads, so we cannot make sp a class variable.
 *
 * However we do not need to make seen_wordlist atomic because it is only used
 * during the single-threaded uniquify pass. We use that pass so we do not need to
 * hold the entire wordlist in memory during processing, but it does add to processing time.
 */
class Scan_Wordlist {
    /* SHUTDOWN PASS - SINGLE-THREADED INSTANCE VARIABLES */
    struct WordlistSorter {
        bool operator()(const std::string &a,const std::string &b) const {
            if (a.size() < b.size()) return true;
            if (a.size() > b.size()) return false;
            return a<b;
        }
    };
    std::set<std::string, WordlistSorter> seen_wordlist {};
    int wordlist_segment {1};
    std::ofstream *wordlist_out {nullptr};
    void dump_seen_wordlist();
    bool wordchar[256] {};              // map of characters that break words
    Scan_Wordlist(const Scan_Wordlist &)=delete; // no copy
    Scan_Wordlist & operator=(const Scan_Wordlist &)=delete; // no assignment

public:;
    inline static const std::string WORDLIST {"wordlist"};
    Scan_Wordlist(scanner_params &sp, bool strings_);
    ~Scan_Wordlist() {
        if (wordlist_out){
            wordlist_out->close();
            delete wordlist_out;
        }
    }
    std::filesystem::path flat_wordlist_path {}; //
    feature_recorder *flat_wordlist = nullptr;

    static const inline uint32_t WORD_MIN_DEFAULT = 6;
    static const inline uint32_t WORD_MAX_DEFAULT = 16;
    static const inline uint64_t MAX_OUTPUT_FILE_SIZE = 100*1000*1000;

    bool     strings {false};           // report all strings, not words. Do not uniquify
    uint32_t word_min  {WORD_MIN_DEFAULT};
    uint32_t word_max {WORD_MAX_DEFAULT};
    uint64_t max_output_file_size {MAX_OUTPUT_FILE_SIZE};

    /* wordlist support for SQL.  Note that the SQL-based wordlist is
     * faster than the file-based wordlist.
     */

    bool wordlist_use_flatfiles {true};
    bool wordlist_use_sql {false};
    /* MULTI-THREADED - sp cannot be an class variable */
    void process_sbuf(scanner_params &sp);
    /* SINGLE-THREADED */
    void shutdown(scanner_params &sp);
};




#endif
