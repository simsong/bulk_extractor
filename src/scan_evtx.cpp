/**
 * Plugin: scan_evtx
 * Purpose: Find all of evtx component, carve out and reconstruct evtx file
 * Reference: https://github.com/libyal/libevtx/blob/master/documentation/Windows%20XML%20Event%20Log%20(EVTX).asciidoc
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

static uint32_t evtx_carve_mode = feature_recorder::CARVE_ALL;

using namespace std;

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define ELFFILE_SIZE 4096
#define ELFCHNK_SIZE 65536
#define FEATURE_FILE_NAME "evtx_carved"

struct elffile {
    struct elffilepart {
        char magic[8];
        int64_t first_chunk;
        int64_t last_chunk;
        int64_t next_record;
        int32_t header_size;
        int16_t minor_version;
        int16_t major_version;
        int16_t header_block_size;
        int16_t number_of_chunks;
        char unknown1[76];
    } part;
    int32_t flags;
    int32_t crc32;
    char unknown2[3968];
};

// https://gist.github.com/timepp/1f678e200d9e0f2a043a9ec6b3690635
// usage: the following code generates crc for 2 pieces of data
// uint32_t table[256];
// crc32::generate_table(table);
// uint32_t crc = crc32::update(table, 0, data_piece1, len1);
// crc = crc32::update(table, crc, data_piece2, len2);
// output(crc);
struct crc32 {
    static void generate_table(uint32_t(&table)[256]) {
        uint32_t polynomial = 0xEDB88320;
		for (size_t i = 0; i < 256; i++) {
			uint32_t c = i;
			for (size_t j = 0; j < 8; j++) {
				if (c & 1)
					c = polynomial ^ (c >> 1);
				else
					c >>= 1;
			}
			table[i] = c;
		}
    }
    static uint32_t update(uint32_t (&table)[256], uint32_t initial, const void* buf, size_t len) {
        uint32_t c = initial ^ 0xFFFFFFFF;
        const uint8_t* u = static_cast<const uint8_t*>(buf);
        for (size_t i = 0; i < len; ++i) {
            c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
        }
        return c ^ 0xFFFFFFFF;
	}
};

// check EVTX Header Signature
// return: > 0 - valid header and number of chunks, 0 - not header, -1 - valid header but invalid num of chunk 
int64_t check_evtxheader_signature(size_t offset, const sbuf_t &sbuf) {
    int16_t num_of_chunks;  
    // \x45\x6c\x66\x46\x69\x6c\x65 ElfFile
    if (sbuf[offset] == 0x45 &&
        sbuf[offset + 1] == 0x6c &&
        sbuf[offset + 2] == 0x66 &&
        sbuf[offset + 3] == 0x46 &&
        sbuf[offset + 4] == 0x69 &&
        sbuf[offset + 5] == 0x6c &&
        sbuf[offset + 6] == 0x65 &&
        sbuf[offset + 7] == 0x00) {
        if (sbuf[offset + 32] == 128 && sbuf.get16i(offset + 40) == 4096) {// Header Size and Header Block Size
            num_of_chunks = sbuf.get16i(offset + 42); // Number of Chunks
            if (num_of_chunks > 0)
                return num_of_chunks;
            else
                return -1;
        }
    }
    return 0;
}

// check EVTX Chunk Signature
// return: > 0 - valid chunk and record id, 0 - not chunk, -1 - valid chunk but invalid record id
int64_t check_evtxchunk_signature(size_t offset, const sbuf_t &sbuf) {
    int64_t last_record_id;
    // \x45\x6c\x66\x43\x68\x6e\x6b ElfChnk
    if (sbuf[offset] == 0x45 &&
        sbuf[offset + 1] == 0x6c &&
        sbuf[offset + 2] == 0x66 &&
        sbuf[offset + 3] == 0x43 &&
        sbuf[offset + 4] == 0x68 &&
        sbuf[offset + 5] == 0x6e &&
        sbuf[offset + 6] == 0x6b &&
        sbuf[offset + 7] == 0x00) {
        if (sbuf[offset + 40] == 128) { // Header Size
            last_record_id = sbuf.get64i(offset + 32); // Last Record ID
            if (last_record_id > 0)
                return last_record_id;
            else
                return -1;
        }
    }
    return 0;
}

// check EVTX Record Signature
// return: > 0 - record size, 0 - not record, 
int64_t check_evtxrecord_signature(size_t offset, const sbuf_t &sbuf) {
    int64_t record_size;
    // \x2a\x2a\x00\x00
    if (sbuf[offset] == 0x2a &&
        sbuf[offset + 1] == 0x2a &&
        sbuf[offset + 2] == 0x00 &&
        sbuf[offset + 3] == 0x00) {
        record_size = sbuf.get32i(offset + 4); // Size
        if (record_size > 0 && record_size < ELFCHNK_SIZE
	    && record_size == sbuf.get32i(offset+record_size-4)) // Copy of size
            return record_size;
    }
    return 0;
}

extern "C"

void scan_evtx(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name            = "evtx";
        sp.info->author          = "Teru Yamazaki";
        sp.info->description     = "Scans for EVTX Chunks and generates valid EVTX file";
        sp.info->scanner_version = "1.0";
        sp.info->feature_names.insert(FEATURE_FILE_NAME);
        return;
    }
    if(sp.phase==scanner_params::PHASE_INIT){
        sp.fs.get_name(FEATURE_FILE_NAME)->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(evtx_carve_mode));
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
        const sbuf_t &sbuf = sp.sbuf;
        feature_recorder_set &fs = sp.fs;
        feature_recorder *evtx_recorder = fs.get_name(FEATURE_FILE_NAME);

        // search for EVTX chunk in the sbuf
        size_t offset = 0;
        size_t stop = sbuf.pagesize;
        size_t total_size=0;

        while (offset < stop) {
            int64_t result_num_of_chunks = check_evtxheader_signature(offset, sbuf);
            int64_t result_last_record_id = 0;
            int64_t last_record_id;
            // ElfFile
            if (result_num_of_chunks > 0) {
                total_size = ELFFILE_SIZE;
                result_last_record_id = check_evtxchunk_signature(offset+total_size, sbuf);
                // check if ElfChnk continues
                if (result_last_record_id > 0) {
                    int32_t actual_num_of_chunk = 1;
                    total_size += ELFCHNK_SIZE;
                    result_last_record_id = check_evtxchunk_signature(offset+total_size, sbuf);
                    while (result_last_record_id > 0 && offset+total_size < stop) {
                        ++actual_num_of_chunk;
                        total_size += ELFCHNK_SIZE;
                        result_last_record_id = check_evtxchunk_signature(offset+total_size, sbuf);
                    }
                    std::string filename = (sbuf.pos0+offset).str() + "_valid_header_" +
                        std::to_string(result_num_of_chunks) + "chunks_" + 
                        std::to_string(actual_num_of_chunk) + "actual.evtx";                
                    evtx_recorder->carve_records(sbuf, offset, total_size, filename);
                } else if (result_last_record_id == -1) {
                    // If valid ElfChnk and invalid record then skip
                    total_size += ELFCHNK_SIZE;
                }
                offset += total_size;
                continue;
            } 
            result_last_record_id = check_evtxchunk_signature(offset, sbuf);
            // ElfChnk
            if (result_last_record_id > 0) {
                int32_t last_chunk = 0;
                last_record_id = result_last_record_id; 
                int64_t first_record_id = sbuf.get64i(offset + 24); // First Record ID
                int64_t num_of_records = last_record_id - first_record_id +1;
                total_size += ELFCHNK_SIZE;
                result_last_record_id = check_evtxchunk_signature(offset+total_size, sbuf);
                while (result_last_record_id > 0 && offset+total_size < stop) {
                    first_record_id = sbuf.get64i(offset+ total_size + 24); // First Record ID
                    last_record_id = result_last_record_id; 
                    num_of_records += last_record_id - first_record_id +1;
                    ++last_chunk;
                    total_size += ELFCHNK_SIZE;
                    result_last_record_id = check_evtxchunk_signature(offset+total_size, sbuf);
                }
                struct elffile header;
                // set header values for found ElfChnk records
                strcpy(header.part.magic, "ElfFile");
                header.part.first_chunk = 0;
                header.part.last_chunk = last_chunk;
                header.part.next_record = last_record_id;
                header.part.header_size = 128;
                header.part.minor_version = 1;
                header.part.major_version = 3;
                header.part.header_block_size = 4096;
                header.part.number_of_chunks = last_chunk+1;
                memset(header.part.unknown1,'\0', sizeof(header.part.unknown1));
                header.flags = 0;
                uint32_t table[256];
                crc32::generate_table(table);
                // CRC32 of the first 120 bytes == header.part struct
                header.crc32 = crc32::update(table, 0, &header.part, 120); 
                memset(header.unknown2,'\0', sizeof(header.unknown2));
                std::string filename = (sbuf.pos0+offset).str() + "_" +
                    std::to_string(header.part.number_of_chunks) + "chunks_" +
                    std::to_string(num_of_records) + "records.evtx";                
                // generate evtx header based on elfchnk information
                evtx_recorder->write_data((unsigned char *)&header,sizeof(elffile),filename);
                evtx_recorder->carve_records(sbuf, offset, total_size, filename);
                offset += total_size;
            } else { // scans orphan record
                size_t i=0;
                while (i < CLUSTER_SIZE) {
                    int64_t result_record_size = check_evtxrecord_signature(offset+i, sbuf);
                    if (result_record_size > 0) {
                        evtx_recorder->carve_records(sbuf,offset+i,result_record_size,"evtx_orphan_record");
                        i += result_record_size;
                    } else {
                        i += 8;
                    }

                }
                offset += CLUSTER_SIZE;
	    }
        } // end while
    } // end PHASE_SCAN
}


