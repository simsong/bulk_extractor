#include "bulk_extractor.h"
#include "xml.h"
#include "utf8.h"
#include "md5.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <cassert>

#define FILE_MAGIC 0x74
#define FILE_HEAD_MIN_LEN 32

#define OFFSET_HEAD_CRC 0
#define OFFSET_HEAD_TYPE 2
#define OFFSET_HEAD_FLAGS 3
#define OFFSET_HEAD_SIZE 5
#define OFFSET_PACK_SIZE 7
#define OFFSET_UNP_SIZE 11
#define OFFSET_HOST_OS 15
#define OFFSET_FILE_CRC 16
#define OFFSET_FTIME 20
#define OFFSET_UNP_VER 24
#define OFFSET_METHOD 25
#define OFFSET_NAME_SIZE 26
#define OFFSET_ATTR 28
#define OFFSET_HIGH_PACK_SIZE 32
#define OFFSET_HIGH_UNP_SIZE 36
#define OFFSET_FILE_NAME 32
#define OFFSET_SALT 32
#define OFFSET_EXT_TIME 40

#define MANDATORY_FLAGS 0x8000
#define UNUSED_FLAGS 0x6000

#define FLAG_CONT_PREV 0x0001
#define FLAG_CONT_NEXT 0x0002
#define FLAG_ENCRYPTED 0x0004
#define FLAG_COMMENT 0x0008
#define FLAG_SOLID 0x0010
#define MASK_DICT 0x00E0
#define FLAG_BIGFILE 0x0100
#define FLAG_UNICODE_FILENAME 0x0200
#define FLAG_SALTED 0x0400
#define FLAG_OLD_VER 0x0800
#define FLAG_EXTIME 0x1000

#define OPTIONAL_BIGFILE_LEN 8

#define SUSPICIOUS_HEADER_LEN 1024

#define STRING_BUF_LEN 1024

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
                cc < sbuf.buf+sbuf.pagesize && cc < sbuf.buf + sbuf.bufsize - FILE_HEAD_MIN_LEN;
                cc++) {
            // Initial RAR file block anchor is 0x74 magic byte
            if(cc[OFFSET_HEAD_TYPE] != FILE_MAGIC) {
                continue;
            }
            // check for invalid flags
            uint16_t flags = (uint16_t) int2(cc + OFFSET_HEAD_FLAGS);
            if(!(flags & MANDATORY_FLAGS) || (flags & UNUSED_FLAGS)) {
                continue;
            }
            // ignore split files and encrypted files
            if(flags & (FLAG_CONT_PREV | FLAG_CONT_NEXT | FLAG_ENCRYPTED)) {
                continue;
            }

            // ignore impossible or improbable header lengths
            uint16_t header_len = (uint16_t) int2(cc + OFFSET_HEAD_SIZE);
            if(header_len < FILE_HEAD_MIN_LEN || header_len > SUSPICIOUS_HEADER_LEN) {
                continue;
            }
            // abort if header is longer than the remaining sbuf
            if(header_len >= (sbuf.buf + sbuf.bufsize) - cc) {
                break;
            }

            // ignore huge filename lengths
            uint16_t filename_bytes_len = (uint16_t) int2(cc + OFFSET_NAME_SIZE);
            if(filename_bytes_len > SUSPICIOUS_HEADER_LEN) {
                continue;
            }

            // ignore strange file sizes
            uint64_t packed_size = (uint64_t) int4(cc + OFFSET_PACK_SIZE);
            uint64_t unpacked_size = (uint64_t) int4(cc + OFFSET_UNP_SIZE);
            if(flags & FLAG_BIGFILE) {
                packed_size += ((uint64_t) int4(cc + OFFSET_HIGH_PACK_SIZE)) << 32;
                unpacked_size += ((uint64_t) int4(cc + OFFSET_HIGH_UNP_SIZE)) << 32;
            }
            if(packed_size == 0 || unpacked_size == 0 || packed_size * 0.95 > unpacked_size) {
                continue;
            }

            //
            // Filename extraction
            //
            string filename = "";
            uint16_t filename_len = 0;
            const char *filename_bytes = (const char *) cc + OFFSET_FILE_NAME;
            if(flags & FLAG_BIGFILE) {
                // if present, the high 32 bits of 64 bit file sizes offset the
                // location of the filename by 8
                filename_bytes += OPTIONAL_BIGFILE_LEN;
            }
            if(flags & FLAG_UNICODE_FILENAME) {
                // The unicode filename flag can indicate two filename formats,
                // predicated on the presence of a null byte:
                //   - If a null byte is present, it separates an ASCII
                //     representation and a UTF-8 representation of the filename
                //     in that order
                //   - If no null byte is present, the filename is UTF-8 encoded
                size_t null_byte_index = 0;
                for(; null_byte_index < filename_bytes_len; null_byte_index++) {
                    if(filename_bytes[null_byte_index] == 0x00) {
                        break;
                    }
                }

                if(null_byte_index == filename_bytes_len - 1u) {
                    // Zero-length UTF-8 representation is illogical
                    continue;
                }

                if(null_byte_index == filename_bytes_len) {
                    // UTF-8 only
                    filename_len = filename_bytes_len;
                    filename = string(filename_bytes, (size_t) filename_len);
                }
                else {
                    // if both ASCII and UTF-8 are present, disregard ASCII
                    filename_len = filename_bytes_len - (null_byte_index + 1);
                    filename = string(filename_bytes + null_byte_index + 1, filename_len);
                }
                // validate extracted UTF-8
                if(utf8::find_invalid(filename.begin(),filename.end()) != filename.end()) {
                    continue;
                }
            }
            else {
                filename_len = filename_bytes_len;
                filename = string(filename_bytes, filename_len);
            }
            // disallow ASCII control characters, which may also appear in valid UTF-8
            string::const_iterator first_control_character = filename.begin();
            for(; first_control_character != filename.end(); first_control_character++) {
                if((char) *first_control_character < ' ') {
                    break;
                }
            }
            if(first_control_character != filename.end()) {
                continue;
            }

            // RAR version required to extract: do we want to abort if it's too new?
            uint8_t unpack_version = cc[OFFSET_UNP_VER];
            uint8_t compression_method = cc[OFFSET_METHOD];
            // OS that created archive
            uint8_t host_os = cc[OFFSET_HOST_OS];
            // date (modification?) In DOS date format
            uint32_t dos_time = int4(cc + OFFSET_FTIME);
            uint32_t file_crc = int4(cc + OFFSET_FILE_CRC);
            uint32_t file_attr = int4(cc + OFFSET_ATTR);

            // build XML output
            filename = xml::xmlescape(filename);
            stringstream ss;
            ss << "<rarinfo>";

            char string_buf[STRING_BUF_LEN];
            snprintf(string_buf,sizeof(string_buf),
                     "<name>%s</name><name_len>%d</name_len>"
                     "<flags>0x%04X</flags><version>%d</version><compression_method>0x%X</compression_method>"
                     "<uncompr_size>%lu</uncompr_size><compr_size>%lu</compr_size><file_attr>0x%X</file_attr>"
                     "<lastmoddosdate>%d</lastmoddosdate><host_os>0x%X</host_os><crc32>%u</crc32>",
                     filename.c_str(),filename_len,flags,unpack_version,
                     compression_method,unpacked_size,packed_size,file_attr,dos_time,host_os,file_crc);
            ss << string_buf;

            ss << "</rarinfo>";

            ssize_t pos = cc-sbuf.buf; // position of the buffer
            rar_recorder->write(pos0+pos,filename,ss.str());
	}
    }
}
