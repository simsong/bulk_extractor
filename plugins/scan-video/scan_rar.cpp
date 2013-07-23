/**
 *
 * scan_rar:
 * Plug-in provides decoding of RAR files.
 */

#include <iostream>
#include <sys/types.h>

#include "../src/bulk_extractor.h"

extern "C"
void  scan_rar(const class scanner_params &sp,
	       const recursion_control_block &rcb)
{
    if(sp.version!=1){
	cerr << "scan_rar requires sp version 1; got version "
	     << sp.version << "\n";
	exit(1);
    }

    /* Check for phase 0 --- startup */
    if(sp.phase==0){
	sp.info->name  = "rar";
	sp.info->flags = 0;
        return; /* No feature files created */
    }

    /* Check for phase 2 --- shutdown */
    if(sp.phase==2){
	return;
    }
    
    for(size_t i = 0 ; i<sp.sbuf.pagesize; i++){
	/* Data is at sp.sbuf[i] */

    }
}
