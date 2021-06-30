/**
 * Plugin: scan_sqlite
 * Purpose: Find sqlite databases and carve them.
 * File format described at http://www.sqlite.org/fileformat.html
 */

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
//#include <cerrno>
#include <sstream>


#include "config.h"
#include "be13_api/scanner_params.h"


/**
 * NOTE: Although it is a simple matter to automatically validate the
 * carved databases and delete those that do not contain recoverable
 * data using the SQLite3 API, doing so would not be advised. These
 * files invariably contain some data from the beginning of the
 * SQLite3 database file: the data appear unrecoverable because the
 * end of the SQLite3 database is not entact. It is highly likely that
 * some of the data in these files can be recovered using forensic
 * means.
 */


#include "utf8.h"

#define FEATURE_FILE_NAME "sqlite_carved"

extern "C"
void scan_sqlite(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.info = new scanner_params::scanner_info( scan_sqlite, "sqlite" );
        sp.info->author          = "Simson Garfinkel";
        sp.info->description     = "Scans for SQLITE3 data";
        sp.info->scanner_version = "1.1";
	sp.info->feature_defs.push_back( feature_recorder_def(FEATURE_FILE_NAME));
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = *(sp.sbuf);
	feature_recorder &sqlite_recorder = sp.ss.named_feature_recorder(FEATURE_FILE_NAME);

	// Search for BEGIN:SQLITE\r in the sbuf
	// we could do this with a loop, or with
	for (size_t i = 0;  i + 512 <= sbuf.bufsize;)	{
	    ssize_t begin = sbuf.find("SQLite format 3\000",i);
	    if (begin==-1) return;		// no more

	    /* We found the header */
            uint32_t pagesize = sbuf.get16uBE(begin+16);
            if (pagesize==1) pagesize=65536;

            /* Pagesize must be a power of two between 512 and 32768 */
            if ((pagesize == 512) || (pagesize == 1024) || (pagesize == 2048) ||
                (pagesize == 4096) || (pagesize == 8192) || (pagesize == 16384) ||
                (pagesize == 32768) || (pagesize == 65536)) {

                uint32_t dbsize_in_pages = sbuf.get32uBE(begin+28);
                size_t   dbsize = pagesize * dbsize_in_pages;

                if (dbsize>0 && dbsize_in_pages>=1){

                    /* Write it out! */
                    sqlite_recorder.carve(sbuf_t(sbuf,begin,begin+dbsize),".sqlite3");

                    /* Worry about overflow */
                    if (( i+begin+dbsize-1) <= i) return; // would send us backwards or avoid movement
                    i = begin + dbsize;
                    continue;
                }
            }
            i = begin + 512;            // skip forward past this block
        }
    }
}
