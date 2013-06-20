/**
 * plugin_test.cpp:
 * 
 * This program will load a bulk_extractor .so or .dll plug-in and
 * perform a rudimentary test.
 */


#include "config.h"                     // from ../config.h
#include "bulk_extractor_i.h"           // from ../src/be13_api/bulk_extractor_i.h

#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
typedef int (__cdecl *MYPROC)(LPWSTR); 
#endif

scanner_params::PrintOptions scanner_params::no_options; 
int main(int argc,char **argv)
{
    std::string fname = argv[1];
    scanner_t *fn=0;
    if(argc!=2){
	fprintf(stderr,"usage: %s scanner.so\n",argv[0]);
        fprintf(stderr,"type 'make plugins' to make available plugins\n");
	exit(1);
    }

    /* Strip extension and path */
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
	err(1,"dlopen_preflight - cannot open %s: %s",fname.c_str(),dlerror());
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
    if(hinstLib==0) errx(1,"LoadLibrary(%s) failed",fname.c_str());
    MYPROC fn = (MYPROC)GetProcAddress(hinstLib,name.c_str());
    if(fn==0) errx(1,"GetProcAddress(%s) failed",name.c_str());
#endif

    feature_recorder_set fs(0);
    uint8_t buf[100];
    pos0_t p0("");
    sbuf_t sbuf(p0,buf,sizeof(buf),sizeof(buf),false);
    scanner_params sp(scanner_params::PHASE_STARTUP,sbuf,fs);
    recursion_control_block rcb(0,"STAND",true);
    scanner_info si;
    sp.info = &si;
    (*fn)(sp,rcb);
    std::cout << "Loaded scanner '" << si.name << "' by " << si.author << "\n";
    dlclose(lib);
    return 0;
}

/*** bogus feature recorder set ***/
const string feature_recorder_set::ALERT_RECORDER_NAME = "alerts";
const string feature_recorder_set::DISABLED_RECORDER_NAME = "disabled";
feature_recorder  *feature_recorder_set::alert_recorder = 0; // no alert recorder to start

feature_recorder *feature_recorder_set::get_name(const std::string &name) { return 0;}
feature_recorder *feature_recorder_set::get_alert_recorder() { return 0;}


feature_recorder_set::feature_recorder_set(uint32_t f):flags(f),input_fname(),
                                                       outdir(),frm(),Mlock(),seen_set(),seen_set_lock(),scanner_stats()
{
    /* not here */
}

bool feature_recorder_set::check_previously_processed(const uint8_t *buf,size_t bufsize){return false;}



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
