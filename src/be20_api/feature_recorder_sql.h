#include <cassert>
#include <cinttypes>

#include <atomic>
#include <map>
#include <regex>
#include <string>
#include <thread>

#include "feature_recorder.h"
#include "pos0.h"

#ifdef HAVE_SQLITE3_H
#include <sqlite3.h>

class feature_recorder_sql : public feature_recorder {
    struct besql_stmt {
        besql_stmt(const besql_stmt&) = delete;
        besql_stmt& operator=(const besql_stmt&) = delete;
        std::mutex Mstmt{};
        sqlite3_stmt* stmt{}; // the prepared statement
        besql_stmt(sqlite3* db3, const char* sql);
        virtual ~besql_stmt();
        void insert_feature(const pos0_t& pos, // insert it into this table!
                            const std::string& feature, const std::string& feature8, const std::string& context);
    };
#if defined(HAVE_SQLITE3_H) and defined(HAVE_LIBSQLITE3)
    // virtual void dump_histogram_sqlite3(const histogram_def &def,void *user,feature_recorder::dump_callback_t cb)
    // const;
#endif
public:
    feature_recorder_sql(class feature_recorder_set& fs, feature_recorder_def def);
    virtual ~feature_recorder_sql();
    //virtual void histogram_write(AtomicUnicodeHistogram& h) override; // flush a specific histogram
};
#endif
