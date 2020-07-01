/**
 * Plugin: scan_ntfsindx
 * Purpose: Find all $INDEX_ALLOCATION INDX record into one file
 * Reference: http://www.digital-evidence.org/fsfa/
 * Teru Yamazaki(@4n6ist) - https://github.com/4n6ist/bulk_extractor-rec 
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

static uint32_t ntfsindx_carve_mode = feature_recorder::CARVE_ALL;

using namespace std;

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define FEATURE_FILE_NAME "ntfsindx_carved"


// check $INDEX_ALLOCATION INDX Signature
// return: 1 - valid INDX record, 2 - corrupt INDX record, 0 - not INDX record
int8_t check_indxrecord_signature(size_t offset, const sbuf_t &sbuf) {
    int16_t fixup_offset;
    int16_t fixup_count;
    int16_t fixup_value;
    int16_t i;

    // start with "INDX"
    if (sbuf[offset] == 0x49 && sbuf[offset + 1] == 0x4E &&
        sbuf[offset + 2] == 0x44  && sbuf[offset + 3] == 0x58) {

        fixup_offset = sbuf.get16i(offset + 4);
        if (fixup_offset <= 0 || fixup_offset >= SECTOR_SIZE)
            return 0;
        fixup_count = sbuf.get16i(offset + 6);
        if (fixup_count <= 0 || fixup_count >= SECTOR_SIZE)
            return 0;

        fixup_value = sbuf.get16i(offset + fixup_offset);

        for(i=1;i<fixup_count;i++){
            if (fixup_value != sbuf.get16i(offset + (SECTOR_SIZE * i) - 2))
                return 2;
        }
        return 1;
    } else {
        return 0;
    }
}

// determine type of INDX
// return: 1 - FILENAME INDX record, 2 - ObjId-O INDX record, 0 - Other INDX record (Secure-SDH, Secure-SII, etc.)
int8_t check_indxrecord_type(size_t offset, const sbuf_t &sbuf) {

    // 4 FILETIME pattern
    if (sbuf[offset + 95] == 0x01 && sbuf[offset + 103] == 0x01 &&
        sbuf[offset + 111] == 0x01 && sbuf[offset + 119] == 0x01)        
        return 1;
    // ObjId-O magic number
    else if (sbuf[offset + 64] == 0x20 && sbuf[offset + 72] == 0x58)
        return 2;
    else
        return 0;
}

extern "C"

void scan_ntfsindx(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name            = "ntfsindx";
        sp.info->author          = "Teru Yamazaki";
        sp.info->description     = "Scans for NTFS $INDEX_ALLOCATION INDX record";
        sp.info->scanner_version = "1.0";
        sp.info->feature_names.insert(FEATURE_FILE_NAME);
        return;
    }
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name(FEATURE_FILE_NAME)->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(ntfsindx_carve_mode));
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        const sbuf_t &sbuf = sp.sbuf;
        feature_recorder_set &fs = sp.fs;
        feature_recorder *ntfsindx_recorder = fs.get_name(FEATURE_FILE_NAME);

        // search for NTFS $INDEX_ALLOCATION INDX record in the sbuf
        size_t offset = 0;
        size_t stop = sbuf.pagesize;
        size_t total_record_size=0;
        int8_t result_type, record_type;

        while (offset < stop) {

            result_type = check_indxrecord_signature(offset, sbuf);
            total_record_size = CLUSTER_SIZE;

            if (result_type == 1) {

                record_type = check_indxrecord_type(offset, sbuf);
                if(record_type == 1) {

                    // found one valid INDX record then also checks following valid records and writes all at once
                    while (true) {
                        if (offset+total_record_size >= stop)
                            break;

                        result_type = check_indxrecord_signature(offset+total_record_size, sbuf);

                        if (result_type == 1) {
                            record_type = check_indxrecord_type(offset+total_record_size, sbuf);
                            if (record_type == 1)
                                total_record_size += CLUSTER_SIZE;
                            else
                                break;
                        }
                        else
                            break;
                    }
                    ntfsindx_recorder->carve_records(sbuf,offset,total_record_size,"INDX");
                }
                else if(record_type == 2) {
                    ntfsindx_recorder->carve_records(sbuf,offset,total_record_size,"INDX_ObjId-O");                    
                }
                else { // 0 - Other INDX record (Secure-SDH, Secure-SII, etc.)
                    ntfsindx_recorder->carve_records(sbuf,offset,total_record_size,"INDX_Misc");
                }
            }
            else if (result_type == 2) {
                ntfsindx_recorder->carve_records(sbuf,offset,total_record_size,"INDX_corrupted");
            }
            else { // result_type == 0
            }
            offset += total_record_size;
        }
    }
}
