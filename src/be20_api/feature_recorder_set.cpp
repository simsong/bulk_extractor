/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "config.h" // needed for hash_t and feature_recorder_sql.h

#include "feature_recorder_file.h"
#include "feature_recorder_set.h"
#include "feature_recorder_sql.h"
#include "scanner_config.h"

#include "dfxml_cpp/src/dfxml_writer.h"
#include "dfxml_cpp/src/hash_t.h"

#ifndef HAVE_LIBSQLITE3
#ifdef HAVE_SQLITE3_H
#warn Disabling HAVE_SQLITE3_H because HAVE_LIBSQLITE3 is not true
#undef HAVE_SQLITE3_H
#endif
#endif

/**
 * feature_recorder_set:
 * Manage the set of feature recorders.
 * Handles both file-based feature recorders and the SQLite3 feature recorder.
 */

const std::string feature_recorder_set::ALERT_RECORDER_NAME = "alerts";
// const std::string feature_recorder_set::DISABLED_RECORDER_NAME = "disabled";

/* be_hash. Currently this just returns the MD5 of the sbuf,
 * but eventually it will allow the use of different hashes.
 */
std::string feature_recorder_set::hash_def::md5_hasher(const uint8_t* buf, size_t bufsize) {
    return dfxml::md5_generator::hash_buf(buf, bufsize).hexdigest();
}

std::string feature_recorder_set::hash_def::sha1_hasher(const uint8_t* buf, size_t bufsize) {
    return dfxml::sha1_generator::hash_buf(buf, bufsize).hexdigest();
}

std::string feature_recorder_set::hash_def::sha256_hasher(const uint8_t* buf, size_t bufsize) {
    return dfxml::sha256_generator::hash_buf(buf, bufsize).hexdigest();
}

feature_recorder_set::hash_func_t feature_recorder_set::hash_def::hash_func_for_name(const std::string& name) {
    if (name == "md5" || name == "MD5") { return md5_hasher; }
    if (name == "sha1" || name == "SHA1" || name == "sha-1" || name == "SHA-1") { return sha1_hasher; }
    if (name == "sha256" || name == "SHA256" || name == "sha-256" || name == "SHA-256") { return sha256_hasher; }
    throw std::invalid_argument("invalid hasher name: " + name);
}

/**
 * Constructor.
 * Create an empty recorder with no outdir.
 */
feature_recorder_set::feature_recorder_set(const flags_t& flags_, const scanner_config& sc_)
    : flags(flags_), sc(sc_), hasher(hash_def(sc_.hash_algorithm, hash_def::hash_func_for_name(sc_.hash_algorithm))) {
    namespace fs = std::filesystem;
    if (sc.outdir.empty()) {
        throw std::invalid_argument("feature_recorder_set::feature_recorder_set(): output directory not provided");
    }

    /* Make sure we can write to the outdir if one is provided */
    if (sc.outdir == scanner_config::NO_OUTDIR) {
        flags.disabled = true;
    } else {
        /* Create the output directory if it doesn't exist */
        if (!fs::is_directory(sc.outdir)) { fs::create_directory(sc.outdir); }

        /* Make sure it is a directory */
        if (!fs::is_directory(sc.outdir)) {
            throw std::runtime_error("Could not create directory " + sc.outdir.string());
        }

        /* Make sure it is writable */
        std::filesystem::path testfile = sc.outdir/"test";
        std::ofstream tmp;
        tmp.open( testfile );
        if (!tmp.is_open()) {
            throw std::invalid_argument("output directory " + sc.outdir.string() + " not writable");
        }
        tmp.close();
        std::filesystem::remove( testfile );
    }

#if 0
#if defined(HAVE_SQLITE3_H)
    if (flag_set(ENABLE_SQLITE3_RECORDERS)) {
        db_create();
    }
#endif
#endif

}

/*
 * deallocator
 */
feature_recorder_set::~feature_recorder_set() {
    frm.clear();                        // be sure all elements are deleted
#if 0
#if defined(HAVE_SQLITE3_H)
    db_close();
#endif
#endif
}

/**
 *
 * Initialize a properly functioning feature recorder set.
 * If disabled, create a disabled feature_recorder that can respond to functions as requested.
 */

/****************************************************************
 *** adding and removing feature recorders
 ****************************************************************/

void feature_recorder_set::create_alert_recorder() {
    /* Create an alert recorder if necessary */
    if (!flags.no_alert) {
        create_feature_recorder(feature_recorder_set::ALERT_RECORDER_NAME); // make the alert recorder
    }
}

/**
 * Create a requested feature recorder. Do not create it if it already exists.
 */

feature_recorder& feature_recorder_set::create_feature_recorder(const feature_recorder_def def) {
    if (frm_frozen) {
        throw std::runtime_error("attempt to create new feature recorder " + def.name + " after frm is frozen.");
    }

    if (flags.record_files and flags.record_sql) {
        throw std::runtime_error("currently can only record to files or SQL, not both");
    }
    if (!flags.record_files and !flags.record_sql) { throw std::runtime_error("Must record to either files or SQL"); }
    if (def.name.size() == 0) {
        throw FeatureRecorderNullName();
    }

    auto it = frm.find(def.name);
    if (it != frm.end()) {
        // we have a feature recorder with the same name. See if it has the same definition!
        feature_recorder &fr = *it->second;
        if (fr.def == def){
            return fr;
        }
        throw FeatureRecorderAlreadyExists{std::string("feature recorder '")
                + def.name + "' already exists but with a different definition."};
    }

    feature_recorder* fr = nullptr;
    if (flags.record_files) { fr = new feature_recorder_file(*this, def); }
#if defined(HAVE_SQLITE3_H) && defined(USE_SQLITE3)
    if (flags.record_sql) { fr = new feature_recorder_sql(*this, def); }
#endif
    fr->context_window = sc.context_window_default;
    fr->carve_mode = def.default_carve_mode; // set the default
    frm.insert(def.name, fr);
    return *fr; // as a courtesy
}

/* convenience constructor for feature recorder with default def */
feature_recorder& feature_recorder_set::create_feature_recorder(std::string name) {
    return create_feature_recorder(feature_recorder_def(name));
}

/*
 * Get the named feature recorder.
 * If the feature recorder doesn't exist and create is true, create it.
 * Otherwise raise an exception.
 */
feature_recorder& feature_recorder_set::named_feature_recorder(const std::string name) const
{
    auto it = frm.find(name);
    if (it == frm.end()) { throw NoSuchFeatureRecorder{std::string("No such feature recorder: ") + name}; }
    return *it->second;
}

/*
 * The alert recorder is special. It's provided for all of the feature recorders.
 * If one doesn't exist, create it.
 */
feature_recorder& feature_recorder_set::get_alert_recorder() const
{
    return named_feature_recorder(feature_recorder_set::ALERT_RECORDER_NAME);
}

/*
 * set_carve_defaults():
 * Called after PHASE_INIT2 before PHASE_ENABLED from scanner_set::apply_scanner_commands.
 */
void feature_recorder_set::set_carve_defaults()
{
    for (const auto &name : frm.keys()){
        int carve_mode = sc.get_carve_mode(name);
        if (carve_mode>0) {
            feature_recorder &fr = frm.get(name);
            std::cerr << "setting carve mode " << carve_mode << " for feature recorder " << name << "\n";
            fr.carve_mode = feature_recorder_def::carve_mode_t( carve_mode );
        }
    }
}

// send every enabled scanner the phase message
void feature_recorder_set::feature_recorders_shutdown() {
    for (auto const& it : frm.values()) {
        it->shutdown();
    }
}

/****************************************************************
 *** Stats
 ****************************************************************/

void feature_recorder_set::dump_name_count_stats(dfxml_writer& writer) const
{
    writer.push("feature_files");
    for (auto name : frm.keys()) {
        writer.set_oneline(true);
        writer.push("feature_file");
        writer.xmlout("name", name);
        writer.xmlout("count", frm.get(name).features_written);
        writer.pop("feature_file");
        writer.set_oneline(false);      // generates the newline
    }
    writer.pop();
}

/****************************************************************
 *** Histogram Support - Called during shutdown of scanner_set.
 ****************************************************************/

/**
 * We now have three kinds of histograms:
 * 1 - Traditional post-processing histograms specified by the histogram library
     1a - feature-file based traditional ones
     1b - SQL-based traditional ones.
 * 2 - In-memory histograms (used primarily by beapi)
 */

void feature_recorder_set::histogram_add(const histogram_def& def) {
    named_feature_recorder(def.feature).histogram_add(def);
}

size_t feature_recorder_set::histogram_count() const {
    /* Ask each feature recorder to count the number of histograms it can produce */
    size_t count = 0;
    for (auto *frp : frm.values()) {
        count += frp->histogram_count();
    }
    return count;
}

/**
 * Have every feature recorder generate all of its histograms.
 */
void feature_recorder_set::histograms_generate() {
    for (auto *frp : frm.values()) {
        frp->histograms_write_all();
    }
}

std::vector<std::string> feature_recorder_set::feature_file_list() const {
    return frm.keys();
}

void feature_recorder_set::info_feature_recorders( std::ostream &os) const
{
    os << std::endl << "Options for setting carve mode in feature recorders that support carving:" << std::endl;
    for (const auto &name : frm.keys()){
        feature_recorder &fr = frm.get(name);
        if (fr.def.flags.carve){
            os << "   -S "<<name<<"_carve_mode=n  where n=[0,1,2]" << std::endl;
        }
    }
    os << "Carve mode 0: do not carve; mode 1: carve encoded data; mode 2: carve everything." << std::endl;
}



#if 0

/*** SQL Support ***/

/*
 * Time results with ubnist1 on R4:
 * no SQL - 79 seconds
 * no pragmas - 651 seconds
 * "PRAGMA synchronous =  OFF", - 146 second
 * "PRAGMA synchronous =  OFF", "PRAGMA journal_mode=MEMORY", - 79 seconds
 *
 * Time with domexusers:
 * no SQL -
 */

#define SQLITE_EXTENSION ".sqlite"
#ifndef SQLITE_DETERMINISTIC
#define SQLITE_DETERMINISTIC 0
#endif

static int debug  = 0;

static const char *schema_db[] = {
    "PRAGMA synchronous =  OFF",
    "PRAGMA journal_mode=MEMORY",
    //"PRAGMA temp_store=MEMORY",  // did not improve performance
    "PRAGMA cache_size = 200000",
    "CREATE TABLE IF NOT EXISTS db_info (schema_ver INTEGER, bulk_extractor_ver INTEGER)",
    "INSERT INTO  db_info (schema_ver, bulk_extractor_ver) VALUES (1,1)",
    "CREATE TABLE IF NOT EXISTS be_features (tablename VARCHAR,comment TEXT)",
    "CREATE TABLE IF NOT EXISTS be_config (name VARCHAR,value VARCHAR)",
    0};

/* Create a feature table and note that it has been created in be_features */
static const char *schema_tbl[] = {
    "CREATE TABLE IF NOT EXISTS f_%s (offset INTEGER(12), path VARCHAR, feature_eutf8 TEXT, feature_utf8 TEXT, context_eutf8 TEXT)",
    "CREATE INDEX IF NOT EXISTS f_%s_idx1 ON f_%s(offset)",
    "CREATE INDEX IF NOT EXISTS f_%s_idx2 ON f_%s(feature_eutf8)",
    "CREATE INDEX IF NOT EXISTS f_%s_idx3 ON f_%s(feature_utf8)",
    "INSERT INTO be_features (tablename,comment) VALUES ('f_%s','')",
    0};

static const char *begin_transaction[] = {"BEGIN TRANSACTION",0};
static const char *commit_transaction[] = {"COMMIT TRANSACTION",0};
void feature_recorder_set::db_send_sql(sqlite3 *db,const char **stmts, ...)
{
    assert(db!=0);
    for (int i=0;stmts[i];i++){
        char *errmsg = 0;
        char buf[65536];

        va_list ap;
        va_start(ap,stmts);
        vsnprintf(buf,sizeof(buf),stmts[i],ap);
        va_end(ap);
        if(debug) std::cerr << "SQL: " << buf << "\n";
        // Don't error on a PRAGMA
        if((sqlite3_exec(db,buf,NULL,NULL,&errmsg) != SQLITE_OK)  && (strncmp(buf,"PRAGMA",6)!=0)) {
            fprintf(stderr,"Error executing '%s' : %s\n",buf,errmsg);
            exit(1);
        }
    }
}

void feature_recorder_set::db_create_table(const std::string &name)
{
    std::cerr << "db_create_table\n";
    assert(name.size()>0);
    assert(db3!=NULL);
    db_send_sql(db3,schema_tbl,name.c_str(),name.c_str());
}

sqlite3 *feature_recorder_set::db_create_empty(const std::string &name)
{
    assert(name.size()>0);
    std::string dbfname  = sc.outdir + "/" + name +  SQLITE_EXTENSION;
    if(debug) std::cerr << "create_feature_database " << dbfname << "\n";
    sqlite3 *db=0;
    if (sqlite3_open_v2(dbfname.c_str(), &db,
                        SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,
                        0)!=SQLITE_OK) {
        std::cerr << "Cannot create database '" << dbfname << "': " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        exit(1);
    }
    return db;
}

void feature_recorder_set::db_create()
{
    assert(db3==0);
    db3 = db_create_empty("report");
    db_send_sql(db3,schema_db);
    std::cout << "in db_create called\n";
}

void feature_recorder_set::db_close()
{
    if(db3){
        if(debug) std::cerr << "db_close()\n";
        sqlite3_close(db3);
        db3 = 0;
    }
}

void feature_recorder_set::db_transaction_begin()
{
    const std::lock_guard<std::mutex> lock(Min_transaction);
    if(!in_transaction){
        db_send_sql(db3,begin_transaction);
        in_transaction = true;
    }
}

void feature_recorder_set::db_transaction_commit()
{
    const std::lock_guard<std::mutex> lock(Min_transaction);
    if(in_transaction){
        db_send_sql(db3,commit_transaction);
        in_transaction = false;
    } else {
        std::cerr << "No transaction to commit\n";
    }
}


#endif
