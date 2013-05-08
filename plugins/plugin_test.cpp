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
    scanner_t *fn;
    if(argc!=2){
	fprintf(stderr,"usage: %s scanner.so\n",argv[0]);
	exit(1);
    }

#ifdef HAVE_DLOPEN
    const char *fname = argv[1];
    char *name = strdup(fname);
    char *cc = strrchr(name,'.');
    if(cc){
	cc[0] = 0;
    } else {
	fprintf(stderr,"%s: cannot strip extension\n",name);
	exit(1);
    }

    if(!dlopen_preflight(fname)){
	err(1,"dlopen_preflight - cannot open %s: %s",fname,dlerror());
    }

    void *lib=dlopen(fname, RTLD_LAZY);

    if(lib==0) errx(1,"dlopen: %s\n",dlerror());
    fn=(scanner_t *)dlsym(lib, name);
    if(fn==0) errx(1,"dlsym: %s\n",dlerror());
#else
#ifdef HAVE_LOADLIBRARY
    /* Use Win32 LoadLibrary function */
    /* See http://msdn.microsoft.com/en-us/library/ms686944(v=vs.85).aspx */
    const char *fname = "hello.DLL";
    HINSTANCE hinstLib = LoadLibrary(TEXT(fname));
    if(hinstLib==0) errx(1,"LoadLibrary(%s) failed",fname);
    MYPROC fn = (MYPROC)GetProcAddress(hinstLib,"hello");
    if(fn==0) errx(1,"GetProcAddress(%s) failed","hello");
#endif
#endif

    uint8_t buf[100];
    pos0_t p0("");
    sbuf_t sbuf(p0,buf,sizeof(buf),sizeof(buf),false);
    feature_recorder_set fs(0);
    scanner_params sp(scanner_params::PHASE_STARTUP,sbuf,fs);
    recursion_control_block rcb(0,"STAND",true);
    scanner_info si;
    sp.info = &si;
    (*fn)(sp,rcb);
    std::cout << "Loaded scanner '" << si.name << "' by " << si.author << "\n";
    dlclose(lib);
    return 0;
}

feature_recorder *feature_recorder_set::get_name(const std::string &name) 
{
    return 0;
}

feature_recorder *feature_recorder_set::get_alert_recorder() 
{
    return 0;
}
