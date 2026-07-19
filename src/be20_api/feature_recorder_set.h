/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef FEATURE_RECORDER_SET_H
#define FEATURE_RECORDER_SET_H

#include <exception>
#include <filesystem>

#if defined(HAVE_SQLITE3_H)
#include <sqlite3.h>
#endif

#include "atomic_map.h"
#include "atomic_set.h"
#include "feature_recorder.h"
#include "sbuf.h"
#include "scanner_config.h"

/** \addtogroup internal_interfaces
 * @{
 */
/** \file */

/**
 * \class feature_recorder_set
 * The feature_recorder_set is an object that controls output. It knows where the output goes (outdir),
 * the various feature recorders that write to that output, and provides for synchronization.
 * It also has the factory method for new feature_recorders. Therefore if you want a different feature_recorder,
 * this set should be subclassed as well.
 *
 * NOTE: plugins can only call virtual functions!
 *
 */

/* Define a map of feature recorders with atomic access. */
/* TODO: This should probably be a unique_ptr */
typedef atomic_map<std::string, class feature_recorder> feature_recorder_map_t;
inline std::ostream& operator<<(std::ostream& os, const feature_recorder_map_t& m) {
    m.write(os);
    return os;
}

class word_and_context_list;
class feature_recorder_set {
private:
    // neither copying nor assignment is implemented
    feature_recorder_set(const feature_recorder_set& fs) = delete;
    feature_recorder_set& operator=(const feature_recorder_set& fs) = delete;

    friend class feature_recorder;

    //const std::string input_fname{}; // input file; copy for convenience.
    //const std::string outdir{};      // where output goes; must know.

    /* map of feature recorders, name->feature recorder It is
     * read-write when BE is running single-threaded. After we go into
     * multi-threaded mode, it is read-only.
     */
    feature_recorder_map_t frm{};
    bool frm_frozen {false};            // once the frm is frozen, it is read-only.
    feature_recorder* stop_list_recorder{nullptr}; // where stopped features get written (if there is one)
#if defined(HAVE_SQLITE3_H) and defined(HAVE_LIBSQLITE3)
    /* If we are compiled with SQLite3, this is the handle to the open database */
    sqlite3* db3{};
#endif

public:
    void frm_freeze() { assert(frm_frozen==false); frm_frozen=true;}
    size_t feature_recorder_count() const { return frm.size(); }
    /* Flags for feature recorders. This used to be a bitmask, but Stroustrup (2013) recommends just having
     * a bunch of bools.
     */
    struct flags_t {
        bool disabled{false};                   // do not record anything! This is is just used for a path-printer
        bool pedantic{false};                   // make sure that all features written are valid utf-8
        bool no_alert{false};                   // no alert recorder
        bool only_alert{false};                 //  always return the alert recorder
        bool create_stop_list_recorders{false}; // static const uint32_t CREATE_STOP_LIST_RECORDERS= 0x04;  //
        bool debug{false};                      // enable debug printing
        bool record_files{true};                // record to files
        bool record_sql{false};                 // record to SQL
    } flags;

    static flags_t flags_disabled() {           // return a frs that is disabled
        flags_t f;
        f.disabled = true;
        return f;
    }

    /** Constructor:
     * create an emptry feature recorder set. If disabled, create a disabled recorder.
     * @param flags_ = config flags
     * @param hash_algorithm - which algorithm to use for de-duplication
     * @param input_fname_ = where input comes from
     * @param outdir_ = output directory (passed to feature recorders). "" if disabled.
     * This clearly needs work.
     */
    feature_recorder_set(const flags_t& flags_, const scanner_config& sc);
    virtual ~feature_recorder_set();

    /* Configuration. This is a copy; it should be a reference, but that caused an AddressSanitizer error. */
    const scanner_config sc;

    /* Read-only functions for the scanner-config file management variables */
    virtual std::filesystem::path get_input_fname() const { return sc.input_fname; }
    virtual std::filesystem::path get_outdir() const { return sc.outdir; }

    /* the feature recorder set automatically hashes all of the sbuf's that it processes. */
    typedef std::string (*hash_func_t)(const uint8_t* buf, size_t bufsize);
    struct hash_def {
        hash_def(std::string name_, hash_func_t func_) : name(name_), func(func_){};
        std::string name; // name of hash
        hash_func_t func; // hash function
        static std::string md5_hasher(const uint8_t* buf, size_t bufsize);
        static std::string sha1_hasher(const uint8_t* buf, size_t bufsize);
        static std::string sha256_hasher(const uint8_t* buf, size_t bufsize);
        static hash_func_t hash_func_for_name(const std::string& name);
    };

    const word_and_context_list* alert_list{}; /* shold be flagged */
    const word_and_context_list* stop_list{};  /* should be ignored */

    /** hashing system */
    const hash_def hasher; // name and function that perform hashing; set by allocator

    static const std::string ALERT_RECORDER_NAME; // the name of the alert recorder
    // static const std::string   DISABLED_RECORDER_NAME; // the fake disabled feature recorder

    void set_stop_list(const word_and_context_list* alist) { stop_list = alist; }
    void set_alert_list(const word_and_context_list* alist) { alert_list = alist; }

    /** Initialize a feature_recorder_set. Previously this was a constructor, but it turns out that
     * virtual functions for the create_name_factory aren't honored in constructors.
     *
     * init() is called after all of the scanners have been loaded. It
     * tells each feature file about its histograms (among other things)
     */

    /* feature_recorder_set flags */
    /* Flags are now implemented as booleans per stroustrup 2013 */

    int64_t offset_add{0};         // added to every reported offset, for use with hadoop
    std::string banner_filename{}; // banner for top of every file

    /* histogram support */
    void histogram_add(const histogram_def& def); // adds it to a local set or to the specific feature recorder
    size_t histogram_count() const;               // counts histograms in all feature recorders

    void set_carve_defaults();

    // called when scanner_set shuts down:
    void feature_recorders_shutdown();
    void histograms_generate(); // make the histograms in the output directory (and optionally in the database)

    //typedef  void (*xml_notifier_t)(const std::string &xmlstring);

    /* support for creating and finding feature recorders
     * Previously called create_name().
     * functions must be virtual so they can be called by plug-in.
     * All return a reference to the named (or created) feature recorder, or else throw exception indicated
     */
    class NoSuchFeatureRecorder : public std::exception {
        std::string m_error{};

    public:
        NoSuchFeatureRecorder(std::string_view error) : m_error(error) {}
        const char* what() const noexcept override { return m_error.c_str(); }
    };

    class FeatureRecorderAlreadyExists : public std::exception {
        std::string m_error{};

    public:
        FeatureRecorderAlreadyExists(std::string_view error) : m_error(error) {}
        const char* what() const noexcept override { return m_error.c_str(); }
    };

    class FeatureRecorderNullName : public std::exception {
    public:
        FeatureRecorderNullName() {}
        const char* what() const noexcept override { return "FeatureRecorderNullName"; }
    };

    /* create a feature recorder, and return it as well */
    virtual void create_alert_recorder();
    virtual feature_recorder& create_feature_recorder(feature_recorder_def def); // create a feature recorder
    virtual feature_recorder& create_feature_recorder(std::string name);         // convenience function

    // Just return it
    virtual feature_recorder& named_feature_recorder(const std::string name) const;       // returns the named feature recorder
    virtual feature_recorder& get_alert_recorder() const;       // returns the alert recorder
    virtual std::vector<std::string> feature_file_list() const; // returns a list of feature file names

    void dump_name_count_stats(class dfxml_writer& writer) const; // dumps the standard dfxml

    void info_feature_recorders( std::ostream &os) const;

    /****************************************************************
     *** DB interface
     ****************************************************************/

#if 0
#if defined(HAVE_SQLITE3_H) and defined(HAVE_LIBSQLITE3)
    virtual  void db_send_sql(sqlite3 *db3,const char **stmts, ...) ;
    virtual  sqlite3 *db_create_empty(const std::string &name) ;
    void     db_create_table(const std::string &name) ;
    void     db_create() ;
    void     db_transaction_begin() ;
    void     db_transaction_commit() ;               // commit current transaction
    void     db_close() ;                            //
#endif
#endif
    /****************************************************************
     *** External Functions
     ****************************************************************/
};

#endif
