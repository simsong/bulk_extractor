/**
 *
 * scan_demo:
 *
 * demonstration that shows how to write a simple plug-in scanner.
 *
 * This scanner tabulates the percentage of blocks that are null and possible JPEGs.
 * It also generates a feature report for every likely JPEG.
 */

#include "../config.h"
#include "be13_api/bulk_extractor_i.h"

#include <iostream>
#include <sys/types.h>

static uint64_t count_null512=0;
static uint64_t count_jpeg=0;
static uint64_t count_total=0;

const ssize_t blocksize = 512;

static bool is_zero(const u_char *buf,size_t count)
{
    while(count>0){
	if(buf[0]) return false;
	count--;
	buf++;
    }
    return true;
}



extern "C"
void  scan_demo(const class scanner_params &sp,const recursion_control_block &rcb)
{
    if(sp.sp_version!=scanner_params::CURRENT_SP_VERSION){
	std::cerr << "scan_demo requires sp version " << scanner_params::CURRENT_SP_VERSION << "; "
		  << "got version " << sp.sp_version << "\n";
	exit(1);
    }

    /* Check for phase 0 --- startup */
    if(sp.phase==scanner_params::PHASE_STARTUP){
	sp.info->name     = "demo";
	sp.info->author  = "Simson L. Garfinkel";
	sp.info->flags = 0;
        return; /* No feature files created */
    }

    /* Check for phase 2 --- shutdown */
    if(sp.phase==scanner_params::PHASE_SHUTDOWN){
	feature_recorder *alert = sp.fs.get_alert_recorder();
	alert->printf("total sectors: %" PRId64,count_total);
	alert->printf("total jpegs: %" PRId64,count_jpeg);
	alert->printf("total nulls: %" PRId64,count_null512);
	return;
    }
    
    if(sp.phase==scanner_params::PHASE_SCAN){
	for(const u_char *b0 = sp.sbuf.buf ;
	    b0<sp.sbuf.buf+sp.sbuf.pagesize-blocksize; b0+=blocksize){
	    
	    __sync_fetch_and_add(&count_total,1);
	    if(is_zero(b0,512)) __sync_fetch_and_add(&count_null512,1);
	    if(b0[0]==0377 && b0[1]==0330 && b0[2]==0377 && b0[3]==0341){
		__sync_fetch_and_add(&count_jpeg,1);
	    }
	}
    }
}
