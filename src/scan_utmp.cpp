/**
 * Plugin: scan_utmp
 * Purpose: Find all utmp record into one file
 * Reference: http://man7.org/linux/man-pages/man5/utmp.5.html
 * Contributed by https://github.com/4n6ist/bulk_extractor-rec 
 **/
#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <sstream>
#include <vector>
#include "sbuf_stream.h"

#include "utf8.h"

static uint32_t utmp_carve_mode = feature_recorder::CARVE_ALL;

using namespace std;

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define UTMP_RECORD 384
#define FEATURE_FILE_NAME "utmp_carved"

bool check_utmprecord_signature(size_t offset, const sbuf_t &sbuf) {
    int ut_type; // defined as short at man page but I have seen 4 byte type on real system
    char line, user, host;    

    ut_type = sbuf.get32i(offset);
    if(ut_type < 1 || ut_type > 8) // not search for ut_type 0 'UT_UNKNOWN' and 9 "ACCOUNTING"
        return false;

    line = sbuf[offset+8];
    if (line != 0 && (line < 32 || line > 126))
        return false;
    for (int i=0; i<32; i++)
        if (sbuf[offset+8+i] == 0) // 0x00 found then it should be continued 0x00 at the end of string
            for (int j=0; j<32-i; j++)
                if (sbuf[offset+8+i+j] != 0)
                    return false;

    user = sbuf[offset+44];
    if (user != 0 && (user < 32 || user > 126))
        return false;
    for (int i=0; i<32; i++)
        if (sbuf[offset+44+i] == 0) // 0x00 found then it should be continued 0x00 at the end of string
            for (int j=0; j<32-i; j++)
                if (sbuf[offset+44+i+j] != 0)
                    return false;

    host = sbuf[offset+76];
    if (host != 0 && (host < 35 || host > 126)
        && host != 33 && host != 37 && host != 60 && host != 62 && host != 92
        && host != 94 && host != 123 && host != 124 && host != 125) // use RFC3986 for soft restriction 
        return false;
    for (int i=0; i<256; i++)
        if (sbuf[offset+76+i] == 0) // 0x00 found then it should be continued 0x00 at the end of string
            for (int j=0; j<256-i; j++)
                if (sbuf[offset+76+i+j] != 0)
                    return false;

    if (sbuf.get32i(offset+340) <= 0) //tv_sec
        return false;

    if (sbuf.get32i(offset+344) < 0 || sbuf.get32i(offset+344) >= 1000000) //tv_usec
        return false;

    for (int i=0; i<20; i++) { 
        if (sbuf[offset+364+i] != 0) // unused
            return false;
    }                   

    return true;
}

extern "C"

void scan_utmp(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name            = "utmp";
        sp.info->author          = "Teru Yamazaki";
        sp.info->description     = "Scans for utmp record";
        sp.info->scanner_version = "1.0";
        sp.info->feature_names.insert(FEATURE_FILE_NAME);
        //        sp.info->get_config("utmp_carve_mode",&utmp_carve_mode,"0=carve none; 1=carve encoded; 2=carve all");
        return;
    }
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name(FEATURE_FILE_NAME)->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(utmp_carve_mode));
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        const sbuf_t &sbuf = sp.sbuf;
        feature_recorder_set &fs = sp.fs;
        feature_recorder *utmp_recorder = fs.get_name(FEATURE_FILE_NAME);

        size_t offset = 0;
        size_t stop = sbuf.pagesize;

        if(stop < UTMP_RECORD)
            return;

        // search for utmp record in the sbuf
        while (offset < stop-UTMP_RECORD) {
            if (check_utmprecord_signature(offset, sbuf)) {
                utmp_recorder->carve_records(sbuf,offset,UTMP_RECORD,"utmp");
                offset += UTMP_RECORD;
            } else {
                offset += 8;
            }
        }
    }
}
