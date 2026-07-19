/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef SCANNER_PARAMS_H
#define SCANNER_PARAMS_H

/*
 * A reference to the scanner_params.h structure is sole argument of scanner.
 * It includes the sbuf to be scanned, the feature recorder set where results are stored,
 *  and instructions for how recursion should be handled.
 * (Whether recursion involves scanning new sbufs or simply decoding them for path printing.)
 */

#include <cassert>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <memory>

// Note: Do not include scanner_set.h or path_printer.h, because they both need scanner_params.h!
//

#include "utils.h"
#include "histogram_def.h"
#include "feature_recorder.h"
#include "feature_recorder_set.h"
#include "sbuf.h"
#include "scanner_config.h"

//#include "packet_info.h"


/** A scanner is a function that takes a reference to scanner params and a recrusion control block */
typedef void scanner_t(struct scanner_params& sp);

/**
 * \class scanner_params
 * The scanner params class is the primary way that the bulk_extractor framework
 * communicates with the scanners.
 *
 * A pointer to the scanner_params struct is provided as the first argument to each scanner when it is called.
 * @param sc_           - The scanner config
 * @param ss_           - The scanner_set for all current scanners
 * @param pp_           - The path printer object if we are printing paths
 * @param phase_        - the PHASE
 * @param sbuf          - (only valid in the scanning phase) the buffer to be scanned or printed
 *
 * The scanner_params struct is a way for sending parameters to the scanner
 * regarding the particular sbuf to be scanned.
 */
struct scanner_params {
    /**
     * scanner_info is an object created by the scanner when it is initialized.
     * A pointer to the object is passed back to the scanner_set in the scanner_params.
     * Once the object is created, it is never modified.
     *
     */
    struct scanner_info {
        const static inline size_t DEFAULT_MIN_SBUF_SIZE=16; // by default, do not scan sbufs smaller than this.

        struct scanner_flags_t {
            bool default_enabled {true}; //  enabled by default
            bool no_usage {false};       //  do not show scanner in usage
            bool no_all {false};         //  do not enable with -eall
            bool find_scanner {false};   //  this scanner uses the find_list
            bool recurse {false};        //  this scanner will recurse
            bool recurse_expand {false}; //  recurses AND result is >= original size
            bool recurse_always {false};      //  this scanner performs no data validation and ALWAYS recurses.
            bool scan_ngram_buffer {false};   //  Scanner can run even if the entire gets buffer is filled with constant n-grams
            bool scan_seen_before {false};    //  Scanner can run even if buffer has seen before
            bool fast_find {false};           //  This scanner is a very fast FIND scanner
            bool depth0_only {false};         //  scanner only runs at depth 0 by default

            bool scanner_wants_memory {false};         // only run scanner on sbufs that might contain memory
            bool scanner_wants_filesystems {false};    // only run scanners on sbufs that might contain file system structures
            bool scanner_produces_memory {false};      // results of scanner may produce memory
            bool scanner_produces_filesystems {false}; //results of scanner may produce file systems.

            const std::string asString() const {
                std::string ret;
                ret += default_enabled ? "ENABLED" : "DISABLED";
                if (no_usage)          ret += " NO_USAGE";
                if (no_all)            ret += " NO_ALL";
                if (find_scanner)      ret += " FIND_SCANNER";
                if (recurse)           ret += " RECURSE";
                if (recurse_expand)    ret += " RECURSE_EXPAND";
                if (scan_ngram_buffer) ret += " SCAN_NGRAM_BUFFER";
                if (scan_seen_before)  ret += " SCAN_SEEN_BEFORE";
                if (fast_find)         ret += " FAST_FIND";
                if (depth0_only)       ret += " DEPTH0_ONLY";
                return ret;
            }
        } scanner_flags {};

        std::string ToUpper(const std::string input) {
            std::string ret;
            for (auto ch:input) {
                ret.push_back(std::toupper(ch));
            }
            return ret;
        }

        // constructor. We must have the name and the pointer. Everything else is optional
        scanner_info(scanner_t* scanner_) : scanner(scanner_) { };
        /* PASSED FROM SCANNER to API: */
        scanner_t* scanner;            // the scanner
        std::string name {};           // scanner name
        std::string pathPrefix {};      //   this scanner's path prefix for recursive scanners. e.g. "GZIP". Typically name uppercase
        std::string help_options {};         // the help string
        std::string author {};          //   who wrote me?
        std::string description {};     //   what do I do?
        std::string url {};             //   where I come from
        std::string scanner_version {}; //   version for the scanner
        std::vector<feature_recorder_def> feature_defs {}; //   feature files that this scanner needs.
        size_t   min_sbuf_size {DEFAULT_MIN_SBUF_SIZE}; // minimum sbuf to scan
        size_t   min_distinct_chars {4};                // number of distinct characters in sbuf required to use scanner. By default, require 4
        uint64_t flags {};              //   flags
        std::vector<histogram_def> histogram_defs {};      //   histogram definitions that the scanner needs

        void set_name(std::string name_) {
            name = name_;
            pathPrefix = ToUpper(name_);
        }

        // Derrived:
        // void              *packet_user {};        //   data for network callback
        // be20::packet_callback_t *packet_cb {};    //   callback for processing network packets, or NULL

        // Move constructor
        scanner_info(scanner_info&& source)
            : scanner(source.scanner), name(source.name), pathPrefix(source.pathPrefix),
              help_options(source.help_options), description(source.description),
              url(source.url), scanner_version(source.scanner_version),
              feature_defs(source.feature_defs), flags(source.flags), histogram_defs(source.histogram_defs) {}

    };

    const int SCANNER_PARAMS_VERSION {20211201}; // allow loadable scanners to validate version number
    int scanner_params_version {SCANNER_PARAMS_VERSION};
    void check_version() { assert(this->scanner_params_version == SCANNER_PARAMS_VERSION); }

    // phase_t specifies when the scanner is being called.
    // the scans are implemented in the scanner set
    enum phase_t {
        PHASE_INIT=1,    // called in main thread when scanner loads
        PHASE_INIT2=2,    // called in main thread to get the feature recorders and so forth.
        PHASE_ENABLED=3, // enable/disable commands called
        PHASE_SCAN=4,    // called in worker thread for every ENABLED scanner to scan an sbuf
        PHASE_SHUTDOWN=5, // called in main thread for every ENABLED scanner when scanner is shutting down. Allows XML closing.
        PHASE_CLEANUP=6,   // tells scanners to deallocate anything they have allocated. Goes to all loaded scanners
        PHASE_CLEANED=7    // after CLEANUP. Nothing else can happen
    };

    /*
     * CONSTRUCTORS
     *
     * Scanner_params are made whenever a scanner needs to be called. These constructors cover all of those cases.
     */
    scanner_params(const scanner_params&) = delete;
    scanner_params& operator=(const scanner_params&) = delete;

    /* A scanner params with all of the instance variables, typically for scanning  */
    scanner_params(struct scanner_config &sc_, class scanner_set  *ss_, const class path_printer *pp_, phase_t phase_, const sbuf_t* sbuf_);

    /** Construct a scanner_params for recursion from an existing sp and a new sbuf, and a new pp_path. */
    scanner_params(const scanner_params& sp_existing, const sbuf_t* sbuf_, std::string pp_path_);

    virtual ~scanner_params() {};

    /* User-servicable parts; these are here for scanners */
    virtual feature_recorder& named_feature_recorder(const std::string feature_recorder_name) const;

    struct scanner_config &sc;          // configuration.
    class scanner_set *ss {nullptr};    // null in PHASE_INIT.
    const class path_printer *pp {nullptr};   // null in PHASE_INIT and if not path printing
    const phase_t phase {};              // what scanner should do
    const sbuf_t* sbuf {nullptr};        // what to scan / only valid in SCAN_PHASE
    std::string  pp_path {};                  // if we are path printing, this is the forensic path being decoded
    const struct PrintOptions *pp_po {nullptr}; // if we are path printing, this is the print options.

    /* output variables */
    struct scanner_info *info {nullptr};  // filled in by callback in PHASE_INIT
    virtual void recurse(const sbuf_t* sbuf) const; // recursive call by scanner. Calls either scanner_set or path_printer.
    virtual bool check_previously_processed(const sbuf_t &sbuf) const;
    std::filesystem::path const get_input_fname() const {return sc.input_fname;}; // not sure why this is needed?

    // These methods are implemented in the plugin system for the scanner to get config information.
    // The get_scanner_config methods should be called on the si object during PHASE_STARTUP (or when help is printed)
    /* When we are asked to get the config:
     * - first build the help string
     * - Second, if an option was specified, then set it as requested.
     */
    template <typename T> void get_scanner_config(const std::string& name, T* val, const std::string& help) const {
        std::stringstream s;
        s << "     -S " << name << "=" << *val << "    " << help << std::endl;
        info->help_options += s.str(); // add the help in

        if (val) {
            std::string v = sc.get_nameval(name);
            if (v.size()) {
                set_from_string(val, v);
            }
        }
    }
    std::string help() const { return info->help_options;}
};

inline std::ostream& operator<<(std::ostream& os, const scanner_params& sp) {
    os << "scanner_params(" << sp.sbuf << ")";
    return os;
};

#endif
