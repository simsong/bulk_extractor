/**
 * Plugin: scan_ntfsmft
 * Purpose: Find all MFT file record into one file
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

static uint32_t ntfsmft_carve_mode = feature_recorder::CARVE_ALL;

using namespace std;

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define MFT_RECORD_SIZE 1024
#define FEATURE_FILE_NAME "ntfsmft_carved"


// check MFT Record Signature
// return: 1 - valid MFT record, 2 - corrupt MFT record, 0 - not MFT record
int8_t check_mftrecord_signature(size_t offset, const sbuf_t &sbuf) {
    int16_t fixup_offset;
    int16_t fixup_count;
    int16_t fixup_value;

    if (sbuf[offset] == 0x46 && sbuf[offset + 1] == 0x49 &&
        sbuf[offset + 2] == 0x4c  && sbuf[offset + 3] == 0x45) {

        fixup_offset = sbuf.get16i(offset + 4);
        if (fixup_offset <= 0 || fixup_offset >= SECTOR_SIZE)
            return 0;
        fixup_count = sbuf.get16i(offset + 6);
        if (fixup_count <= 0 || fixup_count >= SECTOR_SIZE)
            return 0;

        fixup_value = sbuf.get16i(offset + fixup_offset);

        for(int i=1;i<fixup_count;i++){
            if (fixup_value != sbuf.get16i(offset + (SECTOR_SIZE * i) - 2))
                return 2;
        }
        return 1;
    } else {
        return 0;
    }
}

extern "C"

void scan_ntfsmft(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name            = "ntfsmft";
        sp.info->author          = "Teru Yamazaki";
        sp.info->description     = "Scans for NTFS MFT record";
        sp.info->scanner_version = "1.0";
        sp.info->feature_names.insert(FEATURE_FILE_NAME);
        return;
    }
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name(FEATURE_FILE_NAME)->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(ntfsmft_carve_mode));
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        const sbuf_t &sbuf = sp.sbuf;
        feature_recorder_set &fs = sp.fs;
        feature_recorder *ntfsmft_recorder = fs.get_name(FEATURE_FILE_NAME);

        // search for NTFS MFT record in the sbuf
        size_t offset = 0;
        size_t stop = sbuf.pagesize;
        size_t total_record_size=0;
        int8_t result_type;

        while (offset < stop) {

            result_type = check_mftrecord_signature(offset, sbuf);
            total_record_size = MFT_RECORD_SIZE;

            if (result_type == 1) {

                // found one valid record then also checks following valid records and writes all at once
                while (true) {
                    if (offset+total_record_size >= stop)
                        break;

                    result_type = check_mftrecord_signature(offset+total_record_size, sbuf);

                    if (result_type == 1)
                        total_record_size += MFT_RECORD_SIZE;
                    else
                        break;
                }
                ntfsmft_recorder->carve_records(sbuf,offset,total_record_size,"MFT");
            }
            else if (result_type == 2) {
                ntfsmft_recorder->carve_records(sbuf,offset,total_record_size,"MFT_corrputed");
            }
            else { // result_type == 0 - not MFT record
            }
            offset += total_record_size;
        }
    }
}
