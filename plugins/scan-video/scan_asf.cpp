/**
*
* scan_asf, wmv:
*/

#include <iostream>
#include <sys/types.h>
#include <sstream>
#include "../src/bulk_extractor.h"

#include "video_processor.h"
#include "extract_keyframes.h"

// Header keywords - signatures found at the start of file
static const unsigned int ASF_HEADER_SIZE = 16;
static const unsigned int ASF_LENGTH_SIZE = 8;
static const unsigned char ASF_MAIN_HEADER[ASF_HEADER_SIZE + 1] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
static const unsigned int ASF_KW_NO = 5;

// keywords found through out the file.
static const unsigned char ASF_KW[ASF_KW_NO][ASF_HEADER_SIZE] = {
	{0x36, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C},	//ASF_Data_Object
	{0x90, 0x08, 0x00, 0x33, 0xB1, 0xE5, 0xCF, 0x11, 0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB},	//ASF_Simple_Index_Object
	{0xD3, 0x29, 0xE2, 0xD6, 0xDA, 0x35, 0xD1, 0x11, 0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE},	//ASF_Index_Object
	{0xF8, 0x03, 0xB1, 0xFE, 0xAD, 0x12, 0x64, 0x4C, 0x84, 0x0F, 0x2A, 0x1D, 0x2F, 0x7A, 0xD4, 0x8C},	//ASF_Media_Object_Index_Object
};

bool carve_from_header(const unsigned char *s, size_t total_bytes, size_t &length){
	size_t index = 0, prev_index = 0;
	unsigned long long size = 0, size_lsb = 0, size_usb = 0;
	bool is_found = false;
	index = ASF_HEADER_SIZE;
	for(;index < (total_bytes - 24);){
		size_usb = (s[index + 7] << 24) | (s[index + 6] << 16) | (s[index + 5] << 8) | (s[index + 4]);
		size_lsb = (s[index + 3] << 24) | (s[index + 2] << 16) | (s[index + 1] << 8) | (s[index + 0]);
		size = static_cast<unsigned long long>((size_usb << 32) | size_lsb);
		if(((index + size - ASF_HEADER_SIZE) > total_bytes)  || (size <= 0 )) {
			length = index + ASF_HEADER_SIZE;
			return false;
		}
		prev_index = index;
		index += size - ASF_HEADER_SIZE;

		is_found = false;
		for(size_t i = 0; i < ASF_KW_NO; ++i){
			if( memcmp((s + index), ASF_KW[i], ASF_HEADER_SIZE) == 0){
				is_found = true;
				break;
			}
		}

		if(!is_found){
			length = prev_index;
			return false;
		}
		index += ASF_HEADER_SIZE;
	}
	length = prev_index;
	return false;		// we cannot validate an asf/wmv file currently. hence the "false" status return.
}

extern "C"
	void  scan_asf(const class scanner_params &sp,const recursion_control_block &rcb)
{
	
	if(sp.version != 1){
		cerr << "scan_asf requires sp version 1; got version " << sp.version << "\n";
		exit(1);
	}

	if(sp.phase == 0){
		sp.info->name  = "asf";
		sp.info->feature_names.insert("asf");
		return;
	}

	if(sp.phase == 2) return;

	const sbuf_t &sbuf = sp.sbuf;
	
	feature_recorder_set &fs = sp.fs;
	feature_recorder *recorder = fs.get_name("asf");
	recorder->set_quoting_enabled(false);

	const unsigned char* s = sbuf.buf;
	size_t l = sbuf.pagesize;
	char b2[1024];
	bool print = false;

	size_t offset = 0;
	size_t length = 0;
	bool status;
	extract_keyframes ek;
	std::string file_name;

	for(const unsigned char *cc = s; ((cc < (s + l)) && (cc < (s + l - 24))); ++cc){		//24 = ASF_HEADER_SIZE + ASF_LENGTH_SIZE

		if( memcmp(cc, ASF_MAIN_HEADER, ASF_HEADER_SIZE) == 0){
			offset = cc - s;
			status = carve_from_header(cc, (l - offset), length);
			// Status true implies that an intact file was recovered.
			if(status) {
				snprintf(b2,sizeof(b2), "\t%ld\t%ld\tC", offset, length);
				print = true;
				// Extract key frames based complete file in memory. This function also saves file to disk
				ek.extract(sbuf, offset, *recorder);	
			}else{
				// partial header found, try reconstruction instead.
				//use the video processor to convert/store the video found
				video_processor vp;
				if(vp.process_header(sbuf, offset, *recorder)){
					file_name = vp.get_file_name();
					// Extract key frames based on output file
					if(!file_name.empty()){
						ek.extract(file_name);	
						file_name.clear();
					}
				}
			}

		}
		if(print){
			recorder->write(sp.sbuf.pos0+offset, " ", b2);
			//recorder->carve(sbuf,offset, length);
			print = false;
		}
		cc += length;
		length = 0;
	}
	return;
}

