/*
 * bulk_extractor.cpp:
 * Feature Extraction Framework...
 *
 */

#include "config.h"
#include "bulk_extractor.h"
#include "bulk_extractor_api.h"
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
#include <sys/stat.h>

/****************************************************************
 *** Here is the bulk_extractor API
 *** It is under development.
 ****************************************************************/

/* The API relies on a special version of the feature recorder set
 * that writes to a callback function instead of a
 */

class callback_feature_recorder;
class callback_feature_recorder_set;

/* a special feature_recorder_set that calls a callback rather than writing to a file.
 * Typically we will instantiate a single object called the 'cfs' for each BEFILE.
 * It creates multiple named callback_feature_recorders, but they all callback through the same
 * callback function using the same set of locks
 */ 


static std::string hash_name("md5");
static std::string hash_func(const uint8_t *buf,size_t bufsize)
{
    if(hash_name=="md5" || hash_name=="MD5"){
        return md5_generator::hash_buf(buf,bufsize).hexdigest();
    }
    if(hash_name=="sha1" || hash_name=="SHA1" || hash_name=="sha-1" || hash_name=="SHA-1"){
        return sha1_generator::hash_buf(buf,bufsize).hexdigest();
    }
    if(hash_name=="sha256" || hash_name=="SHA256" || hash_name=="sha-256" || hash_name=="SHA-256"){
        return sha256_generator::hash_buf(buf,bufsize).hexdigest();
    }
    std::cerr << "Invalid hash name: " << hash_name << "\n";
    std::cerr << "This version of bulk_extractor only supports MD5, SHA1, and SHA256\n";
    exit(1);
}

static feature_recorder_set::hash_def my_hasher(hash_name,hash_func);

class callback_feature_recorder_set: public feature_recorder_set {
    // neither copying nor assignment are implemented
    callback_feature_recorder_set(const callback_feature_recorder_set &cfs);
    callback_feature_recorder_set &operator=(const callback_feature_recorder_set &cfs);
    histogram_defs_t histogram_defs;        

public:
    void            *user;
    be_callback_t   *cb;
    mutable cppmutex Mcb;               // mutex for the callback

    virtual void heartbeat() {
        (*cb)(user, BULK_EXTRACTOR_API_CODE_HEARTBEAT,0,0,0,0,0,0,0);
    }

    virtual feature_recorder *create_name_factory(const std::string &name_);
    callback_feature_recorder_set(void *user_,be_callback_t *cb_):
        feature_recorder_set(feature_recorder_set::DISABLE_FILE_RECORDERS,my_hasher,
                             feature_recorder_set::NO_INPUT,feature_recorder_set::NO_OUTDIR),
        histogram_defs(),user(user_),cb(cb_),Mcb(){
    }

    virtual void init_cfs(){
        feature_file_names_t feature_file_names;
        be13::plugin::scanners_process_enable_disable_commands();
        be13::plugin::get_scanner_feature_file_names(feature_file_names);
        init(feature_file_names); // creates the feature recorders
        be13::plugin::add_enabled_scanner_histograms_to_feature_recorder_set(*this);
        be13::plugin::scanners_init(*this); // must be done after feature recorders are created
    }

    virtual void write(const std::string &feature_recorder_name,const std::string &str){
        cppmutex::lock lock(Mcb);
        (*cb)(user,BULK_EXTRACTOR_API_CODE_FEATURE,0,
              feature_recorder_name.c_str(),"",str.c_str(),str.size(),"",0);
    }
    virtual void write0(const std::string &feature_recorder_name,
                        const pos0_t &pos0,const std::string &feature,const std::string &context){
        cppmutex::lock lock(Mcb);
        (*cb)(user,BULK_EXTRACTOR_API_CODE_FEATURE,0,
              feature_recorder_name.c_str(),pos0.str().c_str(),
              feature.c_str(),feature.size(),context.c_str(),context.size());
    }

    /* The callback function that will be used to dump a histogram line.
     * it will in turn call the callback function
     */
    static int histogram_dump_callback(void *user,const feature_recorder &fr,
                                       const histogram_def &def,
                                       const std::string &str,const uint64_t &count) {
        callback_feature_recorder_set *cfs = (callback_feature_recorder_set *)(user);
        assert(cfs!=0);
        assert(cfs->cb!=0);
        std::string name = fr.name;
        if(def.suffix.size()) name+= "_" + def.suffix;
        return (*cfs->cb)(user,BULK_EXTRACTOR_API_CODE_HISTOGRAM,
                          count,name.c_str(),"",str.c_str(),str.size(),"",0);
    }
};


class callback_feature_recorder: public feature_recorder {
    // neither copying nor assignment are implemented
    callback_feature_recorder(const callback_feature_recorder &cfr);
    callback_feature_recorder &operator=(const callback_feature_recorder&cfr);
    be_callback_t *cb;
public:
    callback_feature_recorder(be_callback_t *cb_,
                              class feature_recorder_set &fs_,const std::string &name_):
        feature_recorder(fs_,name_),cb(cb_){
    }
    virtual std::string carve(const sbuf_t &sbuf,size_t pos,size_t len, 
                              const std::string &ext){ // appended to forensic path
        return("");                      // no file created
    }
    virtual void open(){}                // we don't open
    virtual void close(){}               // we don't open
    virtual void flush(){}               // we don't open

    /** write 'feature file' data to the callback */
    virtual void write(const std::string &str){
        dynamic_cast<callback_feature_recorder_set *>(&fs)->write(name,str);
    }
    virtual void write0(const pos0_t &pos0,const std::string &feature,const std::string &context){
        dynamic_cast<callback_feature_recorder_set *>(&fs)->write0(name,pos0,feature,context);
    }
    virtual void write(const pos0_t &pos0,const std::string &feature,const std::string &context){
        feature_recorder::write(pos0,feature,context); // pass up
    }

};


/* create_name_factory must be here, after the feature_recorder class is defined. */
feature_recorder *callback_feature_recorder_set::create_name_factory(const std::string &name_){
    return new callback_feature_recorder(cb,*this,name_);
}

struct BEFILE_t {
    BEFILE_t(void *user,be_callback_t cb):fd(),cfs(user,cb),cfg(){};
    int                            fd;
    callback_feature_recorder_set  cfs;
    BulkExtractor_Phase1::Config   cfg;
};

typedef struct BEFILE_t BEFILE;
extern "C" 
BEFILE *bulk_extractor_open(void *user,be_callback_t cb)
{
    histogram_defs_t histograms;
    feature_recorder::set_main_threadid();
    scanner_info::scanner_config   s_config; // the bulk extractor config

#if defined(HAVE_SRANDOM) && !defined(HAVE_SRANDOMDEV)
    srandom(time(0));
#endif
#if defined(HAVE_SRANDOMDEV)
    srandomdev();               // if we are sampling initialize
#endif

    s_config.debug       = 0;           // default debug

    be13::plugin::load_scanners(scanners_builtin,s_config);

    BEFILE *bef = new BEFILE_t(user,cb);
    return bef;
}
    
extern "C" void bulk_extractor_config(BEFILE *bef,uint32_t cmd,const char *name,int64_t arg)
{
    switch(cmd){
    case BEAPI_PROCESS_COMMANDS:
        bef->cfs.init_cfs();
        break;

    case BEAPI_SCANNER_DISABLE:
        be13::plugin::scanners_disable(name);
        break;

    case BEAPI_SCANNER_ENABLE:
        be13::plugin::scanners_enable(name);
        break;

    case BEAPI_FEATURE_DISABLE: {
        feature_recorder *fr = bef->cfs.get_name(name);
        if(fr) fr->set_flag(feature_recorder::FLAG_NO_FEATURES);
        break;
    }

    case BEAPI_FEATURE_ENABLE:{
        feature_recorder *fr = bef->cfs.get_name(name);
        assert(fr);
        if(fr) fr->unset_flag(feature_recorder::FLAG_NO_FEATURES);
        break;
    }

    case BEAPI_MEMHIST_ENABLE:          // enable memory histograms
        bef->cfs.set_flag(feature_recorder_set::MEM_HISTOGRAM);
        break;

    case BEAPI_MEMHIST_LIMIT:{ 
        feature_recorder *fr = bef->cfs.get_name(name);
        assert(fr);
        fr->set_memhist_limit(arg);
        break;
    }

    case BEAPI_DISABLE_ALL:
        be13::plugin::scanners_disable_all();
        break;

    case BEAPI_FEATURE_LIST: {
        /* Get a list of the feature files and send them to the callback */
        std::vector<std::string> ret;
        bef->cfs.get_feature_file_list(ret);
        for(std::vector<std::string>::const_iterator it = ret.begin();it!=ret.end();it++){
            (*bef->cfs.cb)(bef->cfs.user,BULK_EXTRACTOR_API_CODE_FEATURELIST,0,(*it).c_str(),"","",0,"",0);
        }
        break;
    }
    default:
        assert(0);
    }
}


extern "C" 
int bulk_extractor_analyze_buf(BEFILE *bef,uint8_t *buf,size_t buflen)
{
    pos0_t pos0("");
    const sbuf_t sbuf(pos0,buf,buflen,buflen,false);
    be13::plugin::process_sbuf(scanner_params(scanner_params::PHASE_SCAN,sbuf,bef->cfs));
    return 0;
}

extern "C" 
int bulk_extractor_analyze_dev(BEFILE *bef,const char *fname,float frac,int pagesize)
{
    bool sampling_mode = frac < 1.0; // are we in sampling mode or full-disk mode?

    /* A single-threaded sampling bulk_extractor.
     * It may be better to do this with two threads---one that does the reading (and seeking),
     * the other that doe the analysis.
     * 
     * This looks like the code in phase1.cpp.
     */
    BulkExtractor_Phase1::blocklist_t blocks_to_sample;
    BulkExtractor_Phase1::blocklist_t::const_iterator si = blocks_to_sample.begin(); // sampling iterator

    image_process *p = image_process::open(fname,false,pagesize,pagesize);
    image_process::iterator it = p->begin(); // get an iterator

    if(sampling_mode){
        BulkExtractor_Phase1::make_sorted_random_blocklist(&blocks_to_sample, it.max_blocks(),frac);
        si = blocks_to_sample.begin();    // get the new beginning
    }

    while(true){
        if(sampling_mode){             // sampling; position at the next block
            if(si==blocks_to_sample.end()){
                break;
            }
            it.seek_block(*si);
        } else {
            if (it == p->end()){    // end of regular image
                break;
            }
        }

        try {
            sbuf_t *sbuf = it.sbuf_alloc();
            if(sbuf==0) break;      // eof
            be13::plugin::process_sbuf(scanner_params(scanner_params::PHASE_SCAN,*sbuf,bef->cfs));
            delete sbuf;
        }
        catch (const std::exception &e) {
            (*bef->cfs.cb)(bef->cfs.user,BULK_EXTRACTOR_API_EXCEPTION,0,
                           e.what(),it.get_pos0().str().c_str(),"",0,"",0);
        }
        if(sampling_mode){
            ++si;
        } else {
            ++it;
        }
    }
    return 0;
}

extern "C" 
int bulk_extractor_close(BEFILE *bef)
{
    bef->cfs.dump_histograms((void *)&bef->cfs,
                             callback_feature_recorder_set::histogram_dump_callback,0); 
    delete bef;
    return 0;
}


    
