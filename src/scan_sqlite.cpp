/**
 * Plugin: scan_sqlite
 * Purpose: Find sqlite databases and carve them.
 * File format described at http://www.sqlite.org/fileformat.html
 */
#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sstream>

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

static uint32_t sqlite_carve_mode = feature_recorder::CARVE_ALL;

using namespace std;

#define FEATURE_FILE_NAME "sqlite_carved"

extern "C"
void scan_sqlite(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name            = "sqlite";
        sp.info->author          = "Simson Garfinkel";
        sp.info->description     = "Scans for SQLITE3 data";
        sp.info->scanner_version = "1.0";
	sp.info->feature_names.insert(FEATURE_FILE_NAME);
        sp.info->get_config("sqlite_carve_mode",&sqlite_carve_mode,"0=carve none; 1=carve encoded; 2=carve all");
	return;
    }
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name(FEATURE_FILE_NAME)->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(sqlite_carve_mode));
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = sp.sbuf;
	feature_recorder_set &fs = sp.fs;
	feature_recorder *sqlite_recorder = fs.get_name(FEATURE_FILE_NAME);

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
                    sqlite_recorder->carve(sbuf,begin,begin+dbsize,".sqlite3");
            
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
