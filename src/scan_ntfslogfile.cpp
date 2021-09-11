/**
 * Plugin: scan_ntfslogfile
 * Purpose: Find all $LogFile RCRD record into one file
 * Reference: https://flatcap.org/linux-ntfs/ntfs/files/logfile.html
 * Teru Yamazaki(@4n6ist) - https://github.com/4n6ist/bulk_extractor-rec
 **/

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
//#include <cerrno>
#include <sstream>
#include <vector>

#include "config.h"
#include "be13_api/scanner_params.h"

#include "utf8.h"

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

void scan_ntfslogfile(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.info->set_name("ntfslogfile");
        sp.info->author          = "Teru Yamazaki";
        sp.info->description     = "Scans for NTFS $LogFile RCRD record";
        sp.info->scanner_version = "1.1";
        sp.info->feature_defs.push_back( feature_recorder_def(FEATURE_FILE_NAME));
        return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        const sbuf_t &sbuf = (*sp.sbuf);
        feature_recorder &ntfslogfile_recorder = sp.named_feature_recorder(FEATURE_FILE_NAME);

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
                ntfslogfile_recorder.carve( sbuf_t(sbuf,offset,total_record_size), ".LogFile-RCRD");
            }
            else if (result_type == 2) {
                ntfslogfile_recorder.carve( sbuf_t(sbuf,offset,total_record_size), ".LogFile-RCRD_corrupted");
            }
            else if (result_type == 3) {
                ntfslogfile_recorder.carve( sbuf_t(sbuf,offset,total_record_size), ".LogFile-RSTR");
            }
            else if (result_type == 4) {
                ntfslogfile_recorder.carve( sbuf_t(sbuf,offset,total_record_size),".LogFile-RSTR_corrupted");
            }
            else { // result_type == 0 - not RCRD record
            }
            offset += total_record_size;
        }
    }
}
