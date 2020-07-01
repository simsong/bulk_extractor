/**
 * Plugin: scan_ntfslogfile
 * Purpose: Find all $LogFile RCRD record into one file
 * Reference: https://flatcap.org/linux-ntfs/ntfs/files/logfile.html
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

static uint32_t ntfslogfile_carve_mode = feature_recorder::CARVE_ALL;

using namespace std;

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define FEATURE_FILE_NAME "ntfslogfile_carved"


// check $LogFile RCRD Signature
// return: 1 - valid RCRD record, 2 - corrupt RCRD record,
// retrun: 3 - valid RSRT record, 4 - corrupt RSTR record, 0 - not RCRD record
int8_t check_logfilerecord_signature(size_t offset, const sbuf_t &sbuf) {
    int16_t fixup_offset;
    int16_t fixup_count;
    int16_t fixup_value;
    int16_t i;

    if (sbuf[offset] == 0x52 && sbuf[offset + 1] == 0x43 &&
        sbuf[offset + 2] == 0x52  && sbuf[offset + 3] == 0x44) { // RCRD

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
    } else if (sbuf[offset] == 0x52 && sbuf[offset + 1] == 0x53 &&
               sbuf[offset + 2] == 0x54  && sbuf[offset + 3] == 0x52) { // RSTR

        fixup_offset = sbuf.get16i(offset + 4);
        if (fixup_offset <= 0 || fixup_offset >= SECTOR_SIZE)
            return 0;
        fixup_count = sbuf.get16i(offset + 6);
        if (fixup_count <= 0 || fixup_count >= SECTOR_SIZE)
            return 0;

        fixup_value = sbuf.get16i(offset + fixup_offset);

        for(i=1;i<fixup_count;i++){
            if (fixup_value != sbuf.get16i(offset + (SECTOR_SIZE * i) - 2))
                return 4;
        }
        return 3;
    }
    else {
        return 0;
    }
}

extern "C"

void scan_ntfslogfile(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name            = "ntfslogfile";
        sp.info->author          = "Teru Yamazaki";
        sp.info->description     = "Scans for NTFS $LogFile RCRD record";
        sp.info->scanner_version = "1.0";
        sp.info->feature_names.insert(FEATURE_FILE_NAME);
        return;
    }
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name(FEATURE_FILE_NAME)->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(ntfslogfile_carve_mode));
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        const sbuf_t &sbuf = sp.sbuf;
        feature_recorder_set &fs = sp.fs;
        feature_recorder *ntfslogfile_recorder = fs.get_name(FEATURE_FILE_NAME);

        // search for NTFS $LogFile RCRD record in the sbuf
        size_t offset = 0;
        size_t stop = sbuf.pagesize;
        size_t total_record_size=0;
        int8_t result_type;

        while (offset < stop) {

            result_type = check_logfilerecord_signature(offset, sbuf);
            total_record_size = CLUSTER_SIZE;

            if (result_type == 1) {

                // found one valid record then also checks following valid records and writes all at once
                while (true) {
                    if (offset+total_record_size >= stop)
                        break;

                    result_type = check_logfilerecord_signature(offset+total_record_size, sbuf);

                    if (result_type == 1)
                        total_record_size += CLUSTER_SIZE;
                    else
                        break;
                }
                ntfslogfile_recorder->carve_records(sbuf,offset,total_record_size,"LogFile-RCRD");
            }
            else if (result_type == 2) {
                ntfslogfile_recorder->carve_records(sbuf,offset,total_record_size,"LogFile-RCRD_corrupted");
            }
            else if (result_type == 3) {
                ntfslogfile_recorder->carve_records(sbuf,offset,total_record_size,"LogFile-RSTR");
            }
            else if (result_type == 4) {
                ntfslogfile_recorder->carve_records(sbuf,offset,total_record_size,"LogFile-RSTR_corrupted");
            }            
            else { // result_type == 0 - not RCRD record
            }
            offset += total_record_size;
        }
    }
}
