/**
 *
 * scan_bulk:
 * plug-in demonstration that shows how to write a simple plug-in scanner.
 * This scanner tabulates the percentage of blocks that are null.
 * It also generates a feature report for every likely JPEG.
 */

#include <iostream>
#include <sys/types.h>

#include "../src/bulk_extractor.h"

static int cnt = 0;

extern "C"
void  scan_hello(const class scanner_params &sp,const recursion_control_block &rcb)
{
    if(sp.version!=1){
	cerr << "scan_hello requires sp version 1; got version " << sp.version << "\n";
	exit(1);
    }

    /* Check for phase 0 --- startup */
    if(sp.phase==0){
	sp.info->name  = "hello";
	sp.info->flags = 0;
        return; /* No feature files created */
    }

    /* Check for phase 2 --- shutdown */
    if(sp.phase==2){
//	feature_recorder *alert = sp.fs->get_alert_recorder();
//	alert->printf("Hello World! - Phase 2");
	return;
    }
    
    cnt++;
    cerr << "scan_hello: Hello World! (" << cnt << ")" << endl;
    
    const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;
    
	const unsigned char* s = sbuf.buf;
	size_t l = sbuf.pagesize;

    cerr << "Path: " << pos0.path << endl;
    cerr << "Offset: " << pos0.offset << endl;
    cerr << "Page size: " << l << endl;

}
