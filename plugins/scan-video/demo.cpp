#include "../config.h"

#include <stdio.h>
#include <err.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
typedef int (__cdecl *MYPROC)(LPWSTR); 
#endif


int main(int argc,char **argv)
{
    void (*fn)(char*)=0;

#ifdef HAVE_DLOPEN
    const char *fname = "hello.so";

    if(!dlopen_preflight(fname)){
	err(1,"dlopen_preflight - cannot open %s: %s",fname,dlerror());
    }

    void *lib=dlopen(fname, RTLD_LAZY);

    if(lib==0) errx(1,"dlopen: %s\n",dlerror());
    fn=(void (*)(char*))dlsym(lib, "hello");
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

    fn("Sent to demo.");

    dlclose(lib);
    return 0;
}
