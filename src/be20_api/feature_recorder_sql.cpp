/*
 * Feature recorder mods for writing features into an SQLite3 database.
 */

/* http://blog.quibb.org/2010/08/fast-bulk-inserts-into-sqlite/ */

#include "config.h"

#ifdef HAVE_SQLITE3_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "feature_recorder_set.h"
#include "feature_recorder_sql.h"
#include "sbuf.h"

feature_recorder_sql::feature_recorder_sql(class feature_recorder_set& fs_, const feature_recorder_def def_)
    : feature_recorder(fs_, def_) {
    /*
     * If the feature recorder set is disabled, just return.
     */
    if (fs.flags.disabled) return;
        /* write to a database? Create tables if necessary and create a prepared statement */

#if 0
    char buf[1024];
    fs.db_create_table(name);
    snprintf( buf, sizeof(buf), db_insert_stmt,name.c_str() );
    bs = new besql_stmt( fs.db3, buf );
#endif
}

feature_recorder_sql::~feature_recorder_sql() {}

#if 0
#define DB_INSERT_STMT                                                                                                 \
    "INSERT INTO f_%s (offset,path,feature_eutf8,feature_utf8,context_eutf8) VALUES (?1, ?2, ?3, ?4, ?5)"
const char *feature_recorder::db_insert_stmt = DB_INSERT_STMT;

void feature_recorder::besql_stmt::insert_feature(const pos0_t &pos,
                                                        const std::string &feature,
                                                        const std::string &feature8, const std::string &context)
{
    assert(stmt!=0);
    const std::lock_guard<std::mutex> lock(Mstmt);           // grab a lock
    const std::string &path = pos.str();
    sqlite3_bind_int64(stmt, 1, pos.imageOffset()); // offset
    sqlite3_bind_text(stmt, 2, path.data(), path.size(), SQLITE_STATIC); // path
    sqlite3_bind_text(stmt, 3, feature.data(), feature.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, feature8.data(), feature8.size(), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, context.data(), context.size(), SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr,"sqlite3_step failed\n");
    }
    sqlite3_reset(stmt);
};

feature_recorder::besql_stmt::besql_stmt(sqlite3 *db3,const char *sql):Mstmt(),stmt()
{
    assert(db3!=0);
    assert(sql!=0);
    sqlite3_prepare_v2(db3,sql, strlen(sql), &stmt, NULL);
    assert(stmt!=0);
}

feature_recorder::besql_stmt::~besql_stmt()
{
    assert(stmt!=0);
    sqlite3_finalize(stmt);
    stmt = 0;
}

/* Hook for writing feature to SQLite3 database */
void feature_recorder::write0_sqlite3(const pos0_t &pos0,const std::string &feature,const std::string &context)
{
    /**
     * Note: this is not very efficient, passing through a quoted feature and then unquoting it.
     * We could make this more efficient.
     */
    std::string *feature8 = AtomicUnicodeHistogram::convert_utf16_to_utf8(feature_recorder::unquote_string(feature));
    assert(bs!=0);
    bs->insert_feature(pos0,feature,
                         feature8 ? *feature8 : feature,
                         flag_set(feature_recorder::FLAG_NO_CONTEXT) ? "" : context);
    if (feature8) delete feature8;
}

/*** SQL Routines Follow ***
 *
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

/* This creates the base histogram. Note that the SQL fails if the histogram exists */
static const char *schema_hist[] = {
    "CREATE TABLE h_%s (count INTEGER(12), feature_utf8 TEXT)",
    "CREATE INDEX h_%s_idx1 ON h_%s(count)",
    "CREATE INDEX h_%s_idx2 ON h_%s(feature_utf8)",
    0};

/* This performs the histogram operation */
static const char *schema_hist1[] = {
    "INSERT INTO h_%s select COUNT(*),feature_utf8 from f_%s GROUP BY feature_utf8",
    0};

static const char *schema_hist2[] = {
    "INSERT INTO h_%s select sum(count),BEHIST(feature_utf8) from h_%s where BEHIST(feature_utf8)!='' GROUP BY BEHIST(feature_utf8)",
    0};



void feature_recorder::dump_histogram_sqlite3(const histogram_def &def,void *user,feature_recorder::dump_callback_t cb) const
{
    /* First check to see if there exists a feature histogram summary. If not, make it */
    std::string query = "SELECT name FROM sqlite_master WHERE type='table' AND name='h_" + def.feature +"'";
    char *errmsg=0;
    int rowcount=0;
    if ( sqlite3_exec(fs.db3,query.c_str(),callback_counter,&rowcount,&errmsg)){
        std::cerr << "sqlite3: " << errmsg << "\n";
        return;
    }
    if (rowcount==0){
        const char *feature = def.feature.c_str();
        fs.db_send_sql( fs.db3, schema_hist, feature, feature); // creates the histogram
        fs.db_send_sql( fs.db3, schema_hist1, feature, feature); // creates the histogram
    }
    /* Now create the summarized histogram for the regex, if it is not existing, but only if we have
     * sqlite3_create_function_v2
     */
    if (def.pattern.size()>0){
        /* Create the database where we will add the histogram */
        std::string hname = def.feature + "_" + def.suffix;

        /* Remove any "-" characters if present */
        for(size_t i=0;i<hname.size();i++){
            if (hname[i]=='-') hname[i]='_';
        }

        if(debug) std::cerr << "CREATING TABLE = " << hname << "\n";
        if (sqlite3_create_function_v2(fs.db3,"BEHIST",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,
                                       (void *)&def,dump_hist,0,0,0)) {
            std::cerr << "could not register function BEHIST\n";
            return;
        }
        const char *fn = def.feature.c_str();
        const char *hn = hname.c_str();
        fs.db_send_sql(fs.db3,schema_hist, hn , hn); // create the table
        fs.db_send_sql(fs.db3,schema_hist2, hn , fn); // select into it from a function of the old histogram table

        /* erase the user defined function */
        if (sqlite3_create_function_v2(fs.db3,"BEHIST",1,SQLITE_UTF8|SQLITE_DETERMINISTIC,
                                       (void *)&def,0,0,0,0)) {
            std::cerr << "could not remove function BEHIST\n";
            return;
        }
    }
}

void feature_recorder::write(const pos0_t &pos0, const std::string &feature, const std::string &context)
{
    if ( fs.flag_set(feature_recorder_set::ENABLE_SQLITE3_RECORDERS ) &&
         this->flag_notset(feature_recorder::FLAG_NO_FEATURES_SQL) ) {
        write0_sqlite3( pos0, feature, context);
    }
}
#endif

#endif
