/**
 * plugin_test.cpp:
 * 
 * This program will load a bulk_extractor .so or .dll plug-in and
 * perform a rudimentary test.
 */


#include "config.h"                     // from ../config.h
#include "be13_api/bulk_extractor_i.h"  // from ../src/be13_api/bulk_extractor_i.h
#include "be13_api/beregex.cpp"

#include <stdio.h>
#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
typedef int (__cdecl *MYPROC)(LPWSTR); 
#endif

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

scanner_params::PrintOptions scanner_params::no_options; 
int main(int argc,char **argv)
{
    if(argc!=2){
	fprintf(stderr,"usage: %s scanner.so\n",argv[0]);
        fprintf(stderr,"type 'make plugins' to make available plugins\n");
	exit(1);
    }

    /* Strip extension and path */
    std::string fname = argv[1];
    scanner_t *fn=0;
    std::string name = fname;
    size_t dot = name.rfind('.');
    if(dot==std::string::npos){
	fprintf(stderr,"%s: cannot strip extension\n",name.c_str());
	exit(1);
    }
    name = name.substr(0,dot);         

    /* Strip dir */
    size_t slash = name.rfind('.');
    if(slash!=std::string::npos){
        name = name.substr(slash+1);
    }

#ifdef HAVE_DLOPEN
    if(fname.find('.')==std::string::npos){
        fname = "./" + fname;               // fedora requires a complete path name
    }

#ifdef HAVE_DLOPEN_PREFLIGHT
    if(!dlopen_preflight(fname.c_str())){
	fprintf(stderr,"dlopen_preflight - cannot open %s: %s",fname.c_str(),dlerror());
        exit(1);
    }
#endif

    void *lib=dlopen(fname.c_str(), RTLD_LAZY);
    if(lib==0){
        fprintf(stderr,"fname=%s\n",fname.c_str());
        fprintf(stderr,"dlopen: %s\n",dlerror());
        exit(1);
    }

    fn=(scanner_t *)dlsym(lib, name.c_str());
    if(fn==0){
        fprintf(stderr,"dlsym: %s\n",dlerror());
        exit(1);
    }

#endif
#ifdef HAVE_LOADLIBRARY
    /* Use Win32 LoadLibrary function */
    /* See http://msdn.microsoft.com/en-us/library/ms686944(v=vs.85).aspx */
    HINSTANCE hinstLib = LoadLibrary(TEXT(fname.c_str()));
    if(hinstLib==0){
        fprintf(stderr,"LoadLibrary(%s) failed",fname.c_str());
        exit(1);
    }
    MYPROC fn = (MYPROC)GetProcAddress(hinstLib,name.c_str());
    if(fn==0){
        fprintf(stderr,"GetProcAddress(%s) failed",name.c_str());
        exit(1);
    }
#endif

    feature_recorder_set fs(0,my_hasher,feature_recorder_set::NO_INPUT,feature_recorder_set::NO_OUTDIR);
    uint8_t buf[100];
    pos0_t p0("");
    sbuf_t sbuf(p0,buf,sizeof(buf),sizeof(buf),false);
    scanner_params sp(scanner_params::PHASE_STARTUP,sbuf,fs);
    recursion_control_block rcb(0,"STAND");
    scanner_info si;
    sp.info = &si;
    (*fn)(sp,rcb);
    std::cout << "Loaded scanner '" << si.name << "' by " << si.author << "\n";
#ifdef HAVE_DLOPEN
    dlclose(lib);
#endif
    return 0;
}

/*** bogus feature recorder set ***/
const std::string feature_recorder_set::ALERT_RECORDER_NAME = "alerts";
const std::string feature_recorder_set::DISABLED_RECORDER_NAME = "disabled";
const std::string outdir("outdir");
const std::string feature_recorder_set::NO_INPUT = "<NO-INPUT>";
const std::string feature_recorder_set::NO_OUTDIR = "<NO-OUTDIR>";

bool               feature_recorder_set::check_previously_processed(const uint8_t *buf,size_t bufsize){return false;}
feature_recorder  *feature_recorder_set::get_name(const std::string &name) const { return 0;}
feature_recorder  *feature_recorder_set::get_alert_recorder() const { return 0;}
void               feature_recorder_set::create_name(const std::string &name,bool create_stop_also){}
void               feature_recorder_set::get_feature_file_list(std::vector<std::string> &ret){}
void            feature_recorder_set::db_close(){}
BEAPI_SQLITE3 *feature_recorder_set::db_create_empty(const std::string &s){return 0;}
void            feature_recorder_set::db_send_sql(BEAPI_SQLITE3 *s,char const **val, ...){}
feature_recorder  *feature_recorder_set::create_name_factory(const std::string &name_)
{
    return 0;
}


feature_recorder_set::feature_recorder_set(uint32_t f,const feature_recorder_set::hash_def &hasher_,
                                           const std::string &input_fname_,const std::string &outdir_):
    flags(f),seen_set(),input_fname(),outdir(),frm(),Mscanner_stats(),histogram_defs(),Min_transaction(),in_transaction(),db3(),alert_list(),stop_list(),scanner_stats(),hasher(hasher_)
{
    /* not here */
}


/* http://stackoverflow.com/questions/9406580/c-undefined-reference-to-vtable-and-inheritance 
 * Must provide definitions for all virtual functions
 */

void scanner_info::get_config(const scanner_info::config_t &c, const std::string &n,std::string *val,const std::string &help){}
void scanner_info::get_config(const std::string &n,std::string *val,const std::string &help) {}
void scanner_info::get_config(const std::string &n,uint64_t *val,const std::string &help) {}
void scanner_info::get_config(const std::string &n,int32_t *val,const std::string &help) {}
void scanner_info::get_config(const std::string &n,uint32_t *val,const std::string &help) {}
void scanner_info::get_config(const std::string &n,uint16_t *val,const std::string &help) {}
void scanner_info::get_config(const std::string &n,uint8_t *val,const std::string &help) {}
#ifdef __APPLE__
void scanner_info::get_config(const std::string &n,size_t *val,const std::string &help) {}
#define HAVE_GET_CONFIG_SIZE_T
#endif
void scanner_info::get_config(const std::string &n,bool *val,const std::string &help) {}
