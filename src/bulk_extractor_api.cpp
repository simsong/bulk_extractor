/*
 * bulk_extractor.cpp:
 * Feature Extraction Framework...
 *
 */

#include "bulk_extractor.h"
#include "image_process.h"
#include "threadpool.h"
#include "be13_api/aftimer.h"
#include "histogram.h"
#include "dfxml/src/dfxml_writer.h"
#include "dfxml/src/hash_t.h"

#include "phase1.h"

#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <set>
#include <setjmp.h>
#include <vector>
#include <queue>
#include <unistd.h>
#include <ctype.h>

/****************************************************************
 *** 
 *** Here is the bulk_extractor API
 *** It is under development.
 ****************************************************************/

/* The API relies on a special version of the feature recorder set
 * that writes to a callback function instead of a
 */

class callback_feature_recorder: public feature_recorder {
    // neither copying nor assignment are implemented
    callback_feature_recorder(const callback_feature_recorder &cfr);
    callback_feature_recorder &operator=(const callback_feature_recorder&cfr);
public:
    callback_feature_recorder(const std::string &outdir_,string input_fname_,string name_):feature_recorder(outdir_,input_fname_,name_){
    }
    virtual std::string carve(const sbuf_t &sbuf,size_t pos,size_t len, 
                              const std::string &ext, // appended to forensic path
                        const struct be13::hash_def &hasher){
        return("");                     // no file created
    }
    virtual void open(){}               // we don't open
    virtual void close(){}               // we don't open
    virtual void flush(){}               // we don't open
    virtual void make_histogram(const class histogram_def &def){} 

    virtual void write(const std::string &str){
        cppmutex::lock lock(Mf);
        std::cerr << "callback_feature_recorder::write: " << name << ": " << str << "\n";
    }
};

class callback_feature_recorder_set: public feature_recorder_set {
    // neither copying nor assignment are implemented
    callback_feature_recorder_set(const callback_feature_recorder_set &cfs);
    callback_feature_recorder_set &operator=(const callback_feature_recorder_set&cfs);

    //

public:
    virtual feature_recorder *create_name_factory(const std::string &outdir_,const std::string &input_fname_,const std::string &name_){
        return new callback_feature_recorder(outdir_,input_fname_,name_);
    }
    callback_feature_recorder_set():feature_recorder_set(0){
        feature_file_names_t feature_file_names;
        be13::plugin::get_scanner_feature_file_names(feature_file_names);
        init(feature_file_names,"cfrs_input","cfrs_outdir");
    }
};


struct BEFILE_t {
    BEFILE_t():fd(),cfs(),cfg(){};
    int fd;
    callback_feature_recorder_set  cfs;
    BulkExtractor_Phase1::Config   cfg;
};

typedef struct BEFILE_t BEFILE;
typedef int be_callback(int32_t flag,
                        uint32_t arg,
                        const char *feature_recorder_name,
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len);

extern "C" {
    BEFILE *bulk_extractor_open()
    {
        feature_recorder::set_main_threadid();
        scanner_info::scanner_config   s_config; // the bulk extractor config
        be13::plugin::load_scanners(scanners_builtin,s_config);
        be13::plugin::scanners_process_enable_disable_commands();
        BEFILE *bef = new BEFILE_t();
        /* How do we enable or disable individual scanners? */
        /* How do we set or not set a find pattern? */
        /* We want to disable carving, right? */
        /* How do we create the feature recorder with a callback? */
        return bef;
    }
    
    int bulk_extractor_analyze_buf(BEFILE *bef,be_callback cb,uint8_t *buf,size_t buflen)
    {
        pos0_t pos0("");
        const sbuf_t sbuf(pos0,buf,buflen,buflen,false);
        be13::plugin::process_sbuf(scanner_params(scanner_params::PHASE_SCAN,sbuf,bef->cfs));
        cb(0,1,"name","feature",7,"context",7);
        return 0;
    }
    
    int bulk_extractor_close(BEFILE *bef)
    {
        delete bef;
        return 0;
    }
}

    
