/**
 * image_process.cpp:
 *
 * 64-bit file support.
 * Linux: See http://www.gnu.org/s/hello/manual/libc/Feature-Test-Macros.html
 *
 * MacOS & FreeBSD: Not needed; off_t is a 64-bit value.
 */

#include "bulk_extractor.h"
#include "image_process.h"

#ifdef HAVE_FTS_H
#include <fts.h>
#endif

/****************************************************************
 *** Directory Recursion
 ****************************************************************/

process_dir::process_dir(const std::string &image_dir):image_process(image_dir),files(0) 
{
#if defined(HAVE_FTS_H) && defined(HAVE_FTS_OPEN)
    bool have_E01=false;
    bool have_E02=false;
    string E01_file="";
    FTS *fts;
    char *path[2];
    path[0] = (char *)strdup(image_dir.c_str());
    path[1] = 0;
    fts = fts_open(path,FTS_COMFOLLOW|FTS_LOGICAL|FTS_NOCHDIR,0);
    FTSENT *ent;
    while((ent = fts_read(fts))!=0){
	switch (ent->fts_info) {
	case FTS_D:
	    cout << ent->fts_accpath << "...\n"; // show where we are
	    break;
	case FTS_F:
	    files.push_back(ent->fts_accpath);
	    if(strstr(ent->fts_accpath,".E01")){
		have_E01=true;
		E01_file = ent->fts_accpath;
	    }
	    if(strstr(ent->fts_accpath,".E02")) have_E02=true;
	    break;
	}
    }
    fts_close(fts);
    free(path[0]);
    if(have_E01 && have_E02){
	cerr << "It looks like you are attempting to process a directory of EnCase files\n";
	cerr << "with the -R option. You should not do this. Instead, run bulk_extractor\n";
	cerr << "on the file " << E01_file << "\n";
	cerr << "The other EnCase files in the volume will be processed automatically\n";
	files.empty();
    }

    cout << "Total files: " << files.size() << "\n";
#else
    cerr << "support for process_dir not compiled in.\n";
#endif    
}

