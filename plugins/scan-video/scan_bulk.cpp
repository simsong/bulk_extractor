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
void  scan_bulk(const class scanner_params &sp,const recursion_control_block &rcb)
{
    if(sp.version!=1){
	cerr << "scan_bulk requires sp version 1; got version " << sp.version << "\n";
	exit(1);
    }

    /* Check for phase 0 --- startup */
    if(sp.phase==0){
	sp.info->name  = "bulk";
	sp.info->flags = 0;
        return; /* No feature files created */
    }

    /* Check for phase 2 --- shutdown */
    if(sp.phase==2){
	feature_recorder *alert = sp.fs.get_alert_recorder();
	alert->printf("total sectors: %" PRId64,count_total);
	alert->printf("total jpegs: %" PRId64,count_jpeg);
	alert->printf("total nulls: %" PRId64,count_null512);
	return;
    }
    
    for(const u_char *b0 = sp.sbuf.buf ;
	b0<sp.sbuf.buf+sp.sbuf.pagesize-blocksize; b0+=blocksize){
	
	__sync_fetch_and_add(&count_total,1);
	if(is_zero(b0,512)) __sync_fetch_and_add(&count_null512,1);
	if(b0[0]==0377 && b0[1]==0330 && b0[2]==0377 && b0[3]==0341){
	    __sync_fetch_and_add(&count_jpeg,1);
	}
    }
}
