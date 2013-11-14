/*
 * This program demonstrates the bulk_extractor API.
 */

#include "config.h"                              // from ../config.h
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
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len)
{
    printf("be_cb_demo(flag=0x%x,arg=0x%x,name=%s)\n",flag,arg,feature_recorder_name);
    printf("  feature [len=%zu]: ",feature_len);fwrite(feature,1,feature_len,stdout);fputc('\n',stdout);
    printf("  context [len=%zu]: ",context_len);fwrite(context,1,context_len,stdout);fputc('\n',stdout);
    puts("");
    return 0;
}

int main(int argc,char **argv)
{
    bulk_extractor_open_t be_open=0;
    bulk_extractor_analyze_buf_t be_analyze_buf=0;
    bulk_extractor_close_t be_close=0;

#ifdef HAVE_DLOPEN
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

    be_open = (bulk_extractor_open_t)dlsym(lib, "bulk_extractor_open");
    if(be_open==0){
        fprintf(stderr,"dlsym: %s\n",dlerror());
        exit(1);
    }
    be_analyze_buf = (bulk_extractor_analyze_buf_t)dlsym(lib, "bulk_extractor_analyze_buf");
    if(be_analyze_buf==0){
        fprintf(stderr,"dlsym: %s\n",dlerror());
        exit(1);
    }
    be_close = (bulk_extractor_close_t)dlsym(lib, "bulk_extractor_close");
    if(be_close==0){
        fprintf(stderr,"dlsym: %s\n",dlerror());
        exit(1);
    }
#endif
#ifdef HAVE_LOADLIBRARY
    std::string fname = "bulk_extractor.dll";
    /* Use Win32 LoadLibrary function */
    /* See http://msdn.microsoft.com/en-us/library/ms686944(v=vs.85).aspx */
    HINSTANCE hinstLib = LoadLibrary(TEXT(fname.c_str()));
    if(hinstLib==0){
        fprintf(stderr,"LoadLibrary(%s) failed",fname.c_str());
        exit(1);
    }
    be_open = (bulk_extractor_open_t)GetProcAddress(hinstLib,"bulk_extractor_open");
    if(be_open==0){
        fprintf(stderr,"GetProcAddress(bulk_extractor_open) failed");
        exit(1);
    }
    be_analyze_buf = (bulk_extractor_analyze_buf_t)GetProcAddress(hinstLib,"bulk_extractor_analyze");
    if(be_analyze_buf==0){
        fprintf(stderr,"GetProcAddress(bulk_extractor_analyze) failed");
        exit(1);
    }
    be_close = (bulk_extractor_close_t)GetProcAddress(hinstLib,"bulk_extractor_close");
    if(be_close==0){
        fprintf(stderr,"GetProcAddress(bulk_extractor_close) failed");
        exit(1);
    }
#endif

    BEFILE *bef = (*be_open)(be_cb_demo);
    const char *demo_buf = "ABCDEFG  demo@api.com Just a demo 617-555-1212 ok!";
    (*be_analyze_buf)(bef,(uint8_t *)demo_buf,strlen(demo_buf));
    (*be_close)(bef);

#ifdef HAVE_DLOPEN
    dlclose(lib);
#endif
    return 0;
}

