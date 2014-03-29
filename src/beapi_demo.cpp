/*
 * This program demonstrates the bulk_extractor API.
 */

#include "../config.h"                              // from ../config.h
#include "bulk_extractor_api.h"

#include <iostream>
#include <string>
#include <getopt.h>

#include <stdio.h>
#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
typedef int (__cdecl *MYPROC)(LPWSTR); 
#endif

std::vector<std::string> feature_files;
int be_cb_demo(void *user,
               uint32_t flag,
               uint64_t arg,
               const char *name,        // typically the feature recorder name
               const char *pos, // forensic path of the feature
               const char *feature,size_t feature_len,
               const char *context,size_t context_len)
{
    switch(flag){
    case BULK_EXTRACTOR_API_CODE_FEATURE:
        std::cout << "   feature: " << feature << " len=" << feature_len
                  << " context:" << context << " len=" << context_len << "\n";
        return 0;
        
    case BULK_EXTRACTOR_API_CODE_HISTOGRAM:
        if(feature_len==0 && arg==0){
            std::cout << "Feature recorder: " << name << "\n";
        } else {
            std::cout << "\t" << feature << "\tn=" << arg << "\n";
        }
        return 0;

    case BULK_EXTRACTOR_API_CODE_FEATURELIST:
        feature_files.push_back(name);
        return 0;

    default:
        std::cout << "UNKNOWN be_cb_demo(flag=" << flag << ",arg=" << arg << ",name=" << name << ")\n";
        return 0;
    }
}

#ifdef HAVE_DLOPEN
void *getsym(void *lib,const char *name)
{
    void *ptr = dlsym(lib,name);
    if(ptr == 0){
        fprintf(stderr,"dlsym('%s'): %s\n",name,dlerror());
        exit(1);
    }
    return ptr;
}
#endif

__attribute__((noreturn))  void usage(const char *progname)
{
    std::cerr << "usage: " << progname << " <filename or device>\n";
    std::cerr << "This program runs the bulk_extractor_api in the manner\n";
    std::cerr << "that it should be run for the insider threat project.\n";
    exit(1);
}

int main(int argc,char **argv)
{
    const char *progname = argv[0];
    int ch;
    while ((ch = getopt(argc,argv,"h")) != -1) {
        switch(ch){
        case 'h': 
        default: usage(progname);
        }
    }
    if(argc!=2) usage(progname);

    const char *fname = argv[1];

    std::string libname = "libbulkextractor.so";
    if(libname.find('/')==std::string::npos){
        libname = "./" + libname;               // fedora requires a complete path name
    }

#ifdef HAVE_DLOPEN_PREFLIGHT
    if(!dlopen_preflight(libname.c_str())){
	fprintf(stderr,"dlopen_preflight - cannot open %s: %s",libname.c_str(),dlerror());
        exit(1);
    }
#endif

    void *lib=dlopen(libname.c_str(), RTLD_LAZY);
    if(lib==0){
        fprintf(stderr,"libname=%s\n",libname.c_str());
        fprintf(stderr,"dlopen: %s\n",dlerror());
        exit(1);
    }

    bulk_extractor_open_t        be_open        = (bulk_extractor_open_t)getsym(lib, BULK_EXTRACTOR_OPEN);
    bulk_extractor_config_t      be_config      = (bulk_extractor_config_t)getsym(lib, BULK_EXTRACTOR_CONFIG);
    bulk_extractor_analyze_dev_t be_analyze_dev = (bulk_extractor_analyze_dev_t)getsym(lib,BULK_EXTRACTOR_ANALYZE_DEV);
    bulk_extractor_analyze_buf_t be_analyze_buf = (bulk_extractor_analyze_buf_t)getsym(lib,BULK_EXTRACTOR_ANALYZE_BUF);
    bulk_extractor_close_t       be_close       = (bulk_extractor_close_t)getsym(lib, BULK_EXTRACTOR_CLOSE);

    /* Get a handle */
    BEFILE *bef = (*be_open)(0,be_cb_demo);

    /* Now configure the scanners */
    (*be_config)(bef,BEAPI_DISABLE_ALL,     "bulk",0); // turn off all scanners

    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "email",0);        // 
    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "accts",0);        // 
    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "exif",0);         // 
    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "zip",0);          // 
    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "gzip",0);         // 
    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "rar",0);          // 

    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "bulk",0);        // enable the bulk scanner

    (*be_config)(bef,BEAPI_PROCESS_COMMANDS,"",0);          // process the enable/disable commands

    /* Let's get a list of all the feature files in use and tell them that they are all using memory histograms */
    (*be_config)(bef,BEAPI_FEATURE_LIST,   "", 0);   
    (*be_config)(bef,BEAPI_MEMHIST_ENABLE, "", 0);   // enable in-memory histograms
    
    /* Now enable memory histograms for each */
    for(std::vector<std::string>::const_iterator it=feature_files.begin();
        it!=feature_files.end();++it){
        const char *name = (*it).c_str();
        (*be_config)(bef,BEAPI_FEATURE_DISABLE,  name, 0);   // don't give me features
        (*be_config)(bef,BEAPI_MEMHIST_LIMIT,    name, 10);   // don't give me features
    }

    const char *demo_buf = "Test Dataa  demo@bulk_extractor.org Just a demo 617-555-1212 ok!";
    (*be_analyze_buf)(bef,(uint8_t *)demo_buf,strlen(demo_buf));  // analyze the buffer

    /* analyze the file */
    (*be_analyze_dev)(bef,fname,.01,65536);                                 // analyze the file
    (*be_close)(bef);

#ifdef HAVE_DLOPEN
    dlclose(lib);
#endif
    return 0;
}

