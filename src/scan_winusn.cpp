/**
 * Plugin: scan_winusn
 * Purpose: Find all USN_RECORD v2/v4 record into one file
 * USN_RECORD_V2 format https://msdn.microsoft.com/ja-jp/library/windows/desktop/aa365722(v=vs.85).aspx
 * USN_RECORD_V4 format https://msdn.microsoft.com/ja-jp/library/windows/desktop/mt684964(v=vs.85).aspx
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
#include <vector>
#include "sbuf_stream.h"

#include "utf8.h"

static uint32_t winusn_carve_mode = feature_recorder::CARVE_ALL;

using namespace std;

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define FEATURE_FILE_NAME "winusn_carved"

size_t check_usnrecordv2_signature(size_t offset, const sbuf_t &sbuf) {
    size_t record_size;
    if (sbuf[offset + 2] == 0x00 && sbuf[offset + 3] == 0x00 && sbuf[offset + 4] == 0x02 
        && sbuf[offset + 5] == 0x00 && sbuf[offset + 6] == 0x00 && sbuf[offset + 7] == 0x00
        && sbuf[offset + 58] == 0x3c && sbuf[offset + 59] == 0x00) {
        record_size = sbuf.get16i(offset);
        if (record_size >= 64 && record_size <= 600)
            return record_size;
        else
            return 0;
    }
    return 0;
}

size_t check_usnrecordv4_signature(size_t offset, const sbuf_t &sbuf) {
    size_t record_size;
    if (sbuf[offset + 2] == 0x00 && sbuf[offset + 3] == 0x00 && sbuf[offset + 4] == 0x04 
        && sbuf[offset + 5] == 0x00 && sbuf[offset + 6] == 0x00 && sbuf[offset + 7] == 0x00
        && sbuf[offset + 62] == 0x10 && sbuf[offset + 63] == 0x00) {
        record_size = sbuf.get16i(offset);
        if (record_size >= 64 && record_size <= 600)
            return record_size;
        else
            return 0;
    }
    return 0;
}

size_t check_usnrecordv2_or_v4_signature(size_t offset, const sbuf_t &sbuf) {
    size_t record_size;
    record_size = check_usnrecordv2_signature(offset, sbuf);
    if (record_size != 0)
        return record_size;

    record_size = check_usnrecordv4_signature(offset, sbuf);
    if (record_size != 0)
        return record_size;

    return 0;
}

extern "C"

void scan_winusn(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name            = "winusn";
        sp.info->author          = "Teru Yamazaki";
        sp.info->description     = "Scans for USN_RECORD v2/v4 record";
        sp.info->scanner_version = "1.0";
        sp.info->feature_names.insert(FEATURE_FILE_NAME);
        //        sp.info->get_config("winusn_carve_mode",&winusn_carve_mode,"0=carve none; 1=carve encoded; 2=carve all");
        return;
    }
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name(FEATURE_FILE_NAME)->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(winusn_carve_mode));
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        const sbuf_t &sbuf = sp.sbuf;
        feature_recorder_set &fs = sp.fs;
        feature_recorder *winusn_recorder = fs.get_name(FEATURE_FILE_NAME);

        // Search for USN_RECORD_V2 Structure in the sbuf
        size_t offset = 0;
        size_t stop = sbuf.pagesize;
        size_t record_size=0;
        size_t total_record_size=0;

        while (offset < stop-60) {
            // skip $LogFile RCRD Record
            /*
              if (sbuf[offset + 0] == 0x52 && sbuf[offset + 1] == 0x43 && sbuf[offset + 2] == 0x52
              && sbuf[offset + 3] == 0x44 && sbuf[offset + 4] == 0x28 && sbuf[offset + 5] == 0x00) {
              offset = offset + 4096 - (offset % 4096);
              continue;
              }
            */
            record_size = check_usnrecordv2_signature(offset,sbuf);
            if (record_size == 0) {	      
                offset += 8;
                continue;
            }
            total_record_size = record_size;
            //		printf("first record_size %d\n", record_size);
            while (true) {
                record_size = check_usnrecordv2_or_v4_signature(offset+total_record_size,sbuf);
                if (record_size != 0) {
                    if (offset+total_record_size+record_size < stop) {
                        //  			    printf("next record %d \n", record_size);
                        total_record_size += record_size;
                    }
                    else {
                        //    printf("reach stop offset %d \n", record_size);
                        total_record_size = stop-offset;
                        break;
                    }
                }
                else {
                    if ((offset+total_record_size) % SECTOR_SIZE != 0) {
                        size_t next_boundary_offset;
                        next_boundary_offset = offset + total_record_size + SECTOR_SIZE - (total_record_size % SECTOR_SIZE);
                        record_size = check_usnrecordv2_or_v4_signature(next_boundary_offset,sbuf);
                        if (record_size != 0) {
                            //    printf("next record is USN at next boundary\n");
                            total_record_size = next_boundary_offset - offset;
                            continue;
                        }
                        else {
                            //    printf("next record is not USN\n");
                            break;
                        }
                    }
                    else {
                        //    printf("next record is not USN record\n");
                        break;
                    }
                }
            }
            //		printf("offset %d, total record size: %d\n", offset, total_record_size);
            if (total_record_size > CLUSTER_SIZE)
                winusn_recorder->carve(sbuf,offset,total_record_size,".usn");
            else
                winusn_recorder->carve_records(sbuf,offset,total_record_size,"fragment_records_set.usn");
            offset += total_record_size - 8;
        }
        offset += 8;
    }
}
