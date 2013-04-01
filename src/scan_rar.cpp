#include "bulk_extractor.h"
#include "xml.h"
#include "utf8.h"
#include "md5.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <cassert>

#define RAR_FILE_MAGIC 0x74
#define RAR_FILE_HEAD_MIN_LEN 32

#define RAR_OFFSET_HEAD_CRC 0
#define RAR_OFFSET_HEAD_TYPE 2
#define RAR_OFFSET_HEAD_FLAGS 3
#define RAR_OFFSET_HEAD_SIZE 5
#define RAR_OFFSET_PACK_SIZE 7
#define RAR_OFFSET_UNP_SIZE 11
#define RAR_OFFSET_HOST_OS 15
#define RAR_OFFSET_FILE_CRC 16
#define RAR_OFFSET_FTIME 20
#define RAR_OFFSET_UNP_VER 24
#define RAR_OFFSET_METHOD 25
#define RAR_OFFSET_NAME_SIZE 26
#define RAR_OFFSET_ATTR 28
#define RAR_OFFSET_HIGH_PACK_SIZE 32
#define RAR_OFFSET_HIGH_UNP_SIZE 36

#define RAR_MANDATORY_FLAGS 0x8000
#define RAR_UNUSED_FLAGS 0x6000

#define RAR_FLAG_CONT_PREV 0x0001
#define RAR_FLAG_CONT_NEXT 0x0002
#define RAR_FLAG_ENCRYPTED 0x0004
#define RAR_FLAG_COMMENT 0x0008
#define RAR_FLAG_SOLID 0x0010
#define RAR_MASK_DICT 0x00E0
#define RAR_FLAG_BIGFILE 0x0100
#define RAR_FLAG_UNICODED 0x0200
#define RAR_FLAG_SALTED 0x0400
#define RAR_FLAG_OLD_VER 0x0800
#define RAR_FLAG_EXTIME 0x1000

#define SUSPICIOUS_HEADER_LEN 1024

using namespace std;

inline int int2(const u_char *cc)
{
    return (cc[1]<<8) + cc[0];
}

inline int int4(const u_char *cc)
{
    return (cc[3]<<24) + (cc[2]<<16) + (cc[1]<<8) + (cc[0]);
}

/* See:
 * http://gcc.gnu.org/onlinedocs/gcc-4.5.0/gcc/Atomic-Builtins.html
 * for information on on __sync_fetch_and_add
 *
 * When rar_max_depth_count>=rar_max_depth_count_bypass,
 * hash the buffer before decompressing and do not decompress if it has already been decompressed.
 */

int scan_rar_name_len_max = 1024;
int rar_show_all=1;
uint32_t rar_max_depth_count = 0;
const uint32_t rar_max_depth_count_bypass = 5;
set<md5_t>rar_seen_set;
pthread_mutex_t rar_seen_set_lock;
extern "C"
void scan_rar(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "rar";
	sp.info->feature_names.insert("rar");
	pthread_mutex_init(&rar_seen_set_lock,NULL);
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;
	feature_recorder_set &fs = sp.fs;
	feature_recorder *rar_recorder = fs.get_name("rar");
	rar_recorder->set_flag(feature_recorder::FLAG_XML); // because we are sending through XML
	for(const unsigned char *cc=sbuf.buf;
                cc < sbuf.buf+sbuf.pagesize && cc < sbuf.buf + sbuf.bufsize - RAR_FILE_HEAD_MIN_LEN;
                cc++) {
            // Initial RAR file block anchor is 0x74 magic byte
            if(cc[RAR_OFFSET_HEAD_TYPE] != RAR_FILE_MAGIC) {
                continue;
            }
            // check for invalid flags
            uint16_t flags = (uint16_t) int2(cc + RAR_OFFSET_HEAD_FLAGS);
            if(!(flags & RAR_MANDATORY_FLAGS) || (flags & RAR_UNUSED_FLAGS)) {
                continue;
            }
            // ignore split files and encrypted files
            if(flags & (RAR_FLAG_CONT_PREV | RAR_FLAG_CONT_NEXT | RAR_FLAG_ENCRYPTED)) {
                continue;
            }

            // ignore impossible or improbable header lengths
            uint16_t header_len = (uint16_t) int2(cc + RAR_OFFSET_HEAD_SIZE);
            if(header_len < RAR_FILE_HEAD_MIN_LEN || header_len > SUSPICIOUS_HEADER_LEN) {
                continue;
            }

            // ignore huge filename lengths
            uint16_t filename_len = (uint16_t) int2(cc + RAR_OFFSET_NAME_SIZE);
            if(filename_len > SUSPICIOUS_HEADER_LEN) {
                continue;
            }

            // ignore strange file sizes
            uint64_t packed_size = (uint64_t) int4(cc + RAR_OFFSET_PACK_SIZE);
            uint64_t unpacked_size = (uint64_t) int4(cc + RAR_OFFSET_UNP_SIZE);
            if(flags & RAR_FLAG_BIGFILE) {
                packed_size += ((uint64_t) int4(cc + RAR_OFFSET_HIGH_PACK_SIZE)) << 32;
                unpacked_size += ((uint64_t) int4(cc + RAR_OFFSET_HIGH_UNP_SIZE)) << 32;
            }
            if(packed_size == 0 || unpacked_size == 0 || packed_size * 0.95 > unpacked_size) {
                continue;
            }

            ssize_t pos = cc-sbuf.buf; // position of the buffer
            rar_recorder->write(pos0+pos,".","<rarinfo></rarinfo>");
	}
    }
}
