/*
 * This program demonstrates the bulk_extractor API.
 */

#include "../config.h"                              // from ../config.h
#include "bulk_extractor_api.h"

#include <string>

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

int be_cb_demo(int32_t flag,
                        uint32_t arg,
                        const char *feature_recorder_name,
                        const char *pos, // forensic path of the feature
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len)
{
    printf("be_cb_demo(flag=0x%x,arg=0x%x,name=%s)\n",flag,arg,feature_recorder_name);
    printf("  feature [len=%zu]: ",feature_len);fwrite(feature,1,feature_len,stdout);fputc('\n',stdout);
    printf("  context [len=%zu]: ",context_len);fwrite(context,1,context_len,stdout);fputc('\n',stdout);
    puts("");
    return 0;
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

int main(int argc,char **argv)
{
    std::string fname = "bulk_extractor.so";
    if(fname.find('/')==std::string::npos){
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

    bulk_extractor_set_enabled_t be_set_enabled = (bulk_extractor_set_enabled_t)getsym(lib, BULK_EXTRACTOR_SET_ENABLED);
    bulk_extractor_open_t be_open = (bulk_extractor_open_t)getsym(lib, BULK_EXTRACTOR_OPEN);
    bulk_extractor_analyze_buf_t be_analyze_buf = (bulk_extractor_analyze_buf_t)getsym(lib,BULK_EXTRACTOR_ANALYZE_BUF);
    bulk_extractor_close_t be_close = (bulk_extractor_close_t)getsym(lib, BULK_EXTRACTOR_CLOSE);
    (*be_set_enabled)("bulk",1);               // enable the bulk scanner

    BEFILE *bef = (*be_open)(be_cb_demo);
    const char *demo_buf = "ABCDEFG  demo@api.com Just a demo 617-555-1212 ok!";
    (*be_analyze_buf)(bef,(uint8_t *)demo_buf,strlen(demo_buf));
    (*be_close)(bef);

#ifdef HAVE_DLOPEN
    dlclose(lib);
#endif
    return 0;
}

