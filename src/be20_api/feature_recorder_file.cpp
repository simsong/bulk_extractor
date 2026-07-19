/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "config.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdarg>
#include <regex>
#include <exception>

#include "feature_recorder_file.h"
#include "feature_recorder_set.h"
#include "unicode_escape.h"
#include "utils.h"
#include "word_and_context_list.h"
#include "formatter.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 65536
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef DEBUG_PEDANTIC
#define DEBUG_PEDANTIC 0x0001 // check values more rigorously
#endif

/**
 * Unquote octal-style quoting of a string
 */
std::string feature_recorder_file::unquote_string(const std::string& s)
{
    size_t len = s.size();
    if (len < 4) return s; // too small for a quote

    std::string out;
    for (size_t i = 0; i < len; i++) {
        /* Look for octal coding */
        if (i + 3 < len && s[i] == '\\' && isodigit(s[i + 1]) && isodigit(s[i + 2]) && isodigit(s[i + 3])) {
            uint8_t code = (s[i + 1] - '0') * 64 + (s[i + 2] - '0') * 8 + (s[i + 3] - '0');
            out.push_back(code);
            i += 3; // skip over the digits
            continue;
        }
        out.push_back(s[i]);
    }
    return out;
}


/**
 * Get the feature which is defined as being between a \t and [\t\n]
 */

bool feature_recorder_file::extract_feature_context(const std::string& line, std::string &feature, std::string &context)
{
    size_t tab1 = line.find('\t');
    if (tab1 == std::string::npos) return false; // no feature
    size_t feature_start = tab1 + 1;
    size_t tab2 = line.find('\t', feature_start);
    if (tab2 == std::string::npos) return false;

    feature = line.substr(feature_start, tab2 - feature_start);
    context = unquote_string(line.substr(tab2+1));
    return true;
}


/**
 * Create a feature recorder object. Each recorder records a certain
 * kind of feature.  Features are stored in a file. The filename is
 * permutated based on the total number of threads and the current
 * thread that's recording. Each thread records to a different file,
 * and thus a different feature recorder, to avoid locking
 * problems.
 *
 * @param feature_recorder_set &fs - common information for all of the feature recorders
 * @param name         - the name of the feature being recorded.
 */


/*
 * constructor. Not it is called with the feature_recorder_set to which it belongs.
 *
 */
// TODO - make it register itself with the feature recorder set. and do the stuff that's in init.
feature_recorder_file::feature_recorder_file(class feature_recorder_set& fs_, const feature_recorder_def def_)
    : feature_recorder(fs_, def_) {
    /* If the feature recorder set is disabled, just return. */
    if (fs.flags.disabled) return;

    /* Open the file recorder for output.
     * If the file exists, seek to the end and find the last complete line, and start there.
     */
    const std::lock_guard<std::mutex> lock(Mios);
    std::filesystem::path fname = fname_in_outdir("", NO_COUNT);
    ios.open(fname.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::ate);
    if (ios.is_open()) { // opened existing file
        ios.seekg(0L, std::ios_base::end);
        while (ios.is_open()) {
            /* Get current position.
             * If we are at the beginning, just return.
             */
            if (int(ios.tellg()) == 0) {
                ios.seekp(0L, std::ios_base::beg);
                return;
            }
            /* backup one character and see if the next character is a newline */
            ios.seekg(-1, std::ios_base::cur);              // backup to once less than the end of the file
            if (ios.peek() == '\n') {                       // we are finally on the \n
                ios.seekg(1L, std::ios_base::cur);          // move the getting one forward
                ios.seekp(ios.tellg(), std::ios_base::beg); // put the putter at the getter location
                // count_ = 1;                            // greater than zero
                return;
            }
        }
    }
    /* Just open the stream for output */
    ios.open(fname.c_str(), std::ios_base::out);
    if (!ios.is_open()) {
        throw std::invalid_argument(Formatter()
                                    << "*** feature_recorder_file::open Cannot open feature file for writing "
                                    << fname << ":"
                                    << strerror(errno));

    }
}

/* Exiting: make sure that the stream is closed.
 */
feature_recorder_file::~feature_recorder_file()
{
    if (ios.is_open()) { ios.close(); }
}

void feature_recorder_file::banner_stamp(std::ostream& os, const std::string& header) const {
    int banner_lines = 0;
    if (fs.banner_filename.size() > 0) {
        std::ifstream i(fs.banner_filename.c_str());
        if (i.is_open()) {
            std::string line;
            while (getline(i, line)) {
                if (line.size() > 0 && ((*line.end() == '\r') || (*line.end() == '\n'))) {
                    line.erase(line.end()); /* remove the last character while it is a \n or \r */
                }
                os << "# " << line << std::endl;
                banner_lines++;
            }
            i.close();
        }
    }
    if (banner_lines == 0) { os << "# BANNER FILE NOT PROVIDED (-b option)\n"; }

    os << bulk_extractor_version_header;
    os << "# Feature-Recorder: " << name << std::endl;

    if (!fs.get_input_fname().empty()) {
        os << "# Filename: " << fs.get_input_fname().string() << std::endl;
    }
    if (feature_recorder_file::debug != 0) {
        os << "# DEBUG: " << debug << " (";
        if (feature_recorder_file::debug & DEBUG_PEDANTIC) os << " DEBUG_PEDANTIC ";
        os << ")\n";
    }
    os << header;
}

/* statics */
void feature_recorder_file::flush()    { ios.flush(); }
void feature_recorder_file::shutdown() { ios.flush(); }

/**
 * We now have three kinds of histograms:
 * 1 - In-memory histograms (used primarily by beapi)
 * 1 - Traditional post-processing histograms specified by the histogram library
 *   1a - feature-file based traditional ones
 *   1b - SQL-based traditional ones.
 */

/****************************************************************
 *** WRITING SUPPORT
 ****************************************************************/

/* write to the file.
 * this is the only place where writing happens.
 * so it's an easy place to do utf-8 validation in debug mode.
 */
void feature_recorder_file::write0(const std::string& str)
{
    feature_recorder::write0(str);      // call super class
    if (fs.flags.pedantic && (utf8::find_invalid(str.begin(), str.end()) != str.end())) {
        throw std::runtime_error(
            Formatter()
            << "******************************************\n"
            << "feature recorder: " << name
            << "invalid utf-8 in write: " << str
            << "Invalid utf-8 in write");
    }

    /* this is where the writing happens. lock the output and write */
    if (fs.flags.disabled) { return; }

    const std::lock_guard<std::mutex> lock(Mios);
    if (ios.is_open()) {
        /* If there is no banner, add it */
        if (ios.tellg() == 0) {
            banner_stamp(ios, feature_file_header);
        }

        /* Output the feature */
        ios << str << '\n';
        if (ios.fail()) {
            throw DiskWriteError("");
        }
    }
}

/**
 * Combine the pos0, feature and context into a single line and write it to the feature file.
 * This must be called for every feature
 *
 * @param feature - The feature, which is valid UTF8 (but may not be exactly the bytes on the disk)
 * @param context - The context, which is valid UTF8 (but may not be exactly the bytes on the disk)
 *
 * Interlocking is done in write().
 */

void feature_recorder_file::write0(const pos0_t& pos0, const std::string& feature, const std::string& context) {
    feature_recorder::write0(pos0, feature, context); // call super to increment counter
    if (fs.flags.disabled) { return; }
    std::stringstream ss;
    ss << pos0.shift(fs.offset_add).str() << '\t' << feature;
    if ((def.flags.no_context == false) && (context.size() > 0)) {
        ss << '\t' << context;
    }

    write0(ss.str());                                 // and do the actual write
}

/****************************************************************
 *** INCREMENTAL HISTOGRAMS
 ****************************************************************/

/**
 * add a new histogram to this feature recorder.
 * @param def - the histogram definition
 *
 * The SQL implementation might keep histograms in an SQL table.
 */
void feature_recorder_file::histogram_add(const struct histogram_def& hdef)
{
    if (features_written != 0) {
        throw std::runtime_error("Cannot add histograms after features have been written.");
    }
    histograms.push_back( std::make_unique<AtomicUnicodeHistogram>(hdef) );
    histograms.back()->debug = debug_histograms;

}


// how many histograms it has
size_t feature_recorder_file::histogram_count()
{
    return histograms.size();
}

bool feature_recorder_file::histograms_write_largest()
{
    std::cerr << "TODO: Implement histograms_write_largest\n";
    return false;                       // need to implement
}

/* Write all of the histograms associated with this feature recorder. */
void feature_recorder_file::histograms_write_all()
{
    for (auto& h : histograms) {
        this->histogram_write(*h);
    }
}


/** Flush a specific histogram.
 * This is how the feature recorder triggers the histogram to be written.
 */
void feature_recorder_file::histogram_write_from_memory(AtomicUnicodeHistogram& h)
{
    if (debug_histograms){
        std::cerr << std::endl
                  << "feature_recorder_file::histogram_write_from_memory " << h.def << std::endl
                  << " h.size=" << h.size() << " h.bytes=" << h.bytes() << std::endl;
    }

    /* Get the next filename */
    auto fname = fname_in_outdir(h.def.suffix, NEXT_COUNT);
    std::fstream hfile;
    hfile.open(fname.c_str(), std::ios_base::out);
    if (!hfile.is_open()) {
        throw std::runtime_error("Cannot open feature histogram file " + fname.string());
    }

    bool first = true;
    auto r = h.makeReport(0); // sorted and clear
    for(const auto &it : r){
        if ( first ) {
            banner_stamp( hfile, histogram_file_header );
            first = false;
        }
        hfile << it;
    }
    hfile.close();
    h.clear();                          // free up the memory
}

void feature_recorder_file::histogram_write_from_file(AtomicUnicodeHistogram& h)
{
    /* Read each line of the feature file and add it to the histogram.
     * If we run out of memory, dump that histogram to a file and start
     * on the next histogram.
     */

    if (debug_histograms) std::cerr << "feature_recorder_file::histogram_write_from_file " << h.def << std::endl;


    /* This is a file based histogram. We will be reading from one file and writing to another */
    std::filesystem::path ifname = fname_in_outdir("", NO_COUNT);  // source of features
    std::ifstream f(ifname.c_str());
    if(!f.is_open()){
        std::cerr << "Cannot open histogram input file: " << ifname << std::endl;
        return;
    }
    for (int histogram_counter = 0 ; histogram_counter<MAX_HISTOGRAM_FILES; histogram_counter++){
        std::string line;
        while(getline(f,line)){
            if(line.size()==0) continue; // empty line
            if(line[0]=='#') continue;   // comment line
            truncate_at(line,'\r');      // truncate at a \r if there is one.

            std::string feature;
            std::string context;
            if (extract_feature_context(line, feature, context)) {
                /* if the feature is in the context, feature is in utf8, otherwise it was utf16 and converted */
                bool found_utf16 = (context.find(feature) == std::string::npos);
                try {
                    h.add0( feature, context, found_utf16 );
                }
                catch (const std::bad_alloc &e) {
                    std::cerr << "MEMORY OVERFLOW GENERATING HISTOGRAM  "
                              << name << ". Dumping Histogram " << histogram_counter << std::endl;
                    histogram_write_from_memory(h);
                }
            }
        }
    }
    f.close();
    histogram_write_from_memory(h);                // write out the histogram
}

void feature_recorder_file::histogram_write(AtomicUnicodeHistogram& h)
{
    /* If 'h' actually has data, write it with histogram_write0().
     * If 'h' does not have data, read the feature files and dump 'h' write write0 each time there is an out-of-memory
     */
    if (debug_histograms) std::cerr << std::endl << "feature_recorder_file::histogram_write " << h.def << std::endl;
    if (disable_incremental_histograms) {
        histogram_write_from_file(h);
    } else {
        histogram_write_from_memory(h);
    }
}


/**
 * add a feature to all of the feature recorder's histograms
 * @param feature - the feature to add.
 * @param context - the context
 */
void feature_recorder_file::histograms_incremental_add_feature_context(const std::string& feature, const std::string& context)
{
    if (debug_histograms) {
        std::cerr << "feature_recorder_file::histograms_incremental_add_feature_context feature="
                  << feature << " context=" << context << std::endl;
    }
    for (auto &h : histograms) {
        h->add_feature_context(feature, context); // add the original feature
    }
}
