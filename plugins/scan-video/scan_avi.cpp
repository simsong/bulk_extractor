/**
*
* scan_avi:
*/

#include <iostream>
#include <sys/types.h>
#include <sstream>
#include "../src/bulk_extractor.h"

#include "video_processor.h"
#include "extract_keyframes.h"

// Header keywords - signatures found at the start of file
static const int RIFF_HEADER_SIZE = 4;
static const int RIFF_LENGTH_SIZE = 4;
static const char RIFF_MAIN_HEADER[RIFF_HEADER_SIZE + 1] = "RIFF";
static const int RIFF_SUB_HEADER_NO = 2;
static const char RIFF_SUB_HEADER[RIFF_SUB_HEADER_NO][RIFF_HEADER_SIZE] = {
	{'W','A','V','E' }, {'A','V','I',' '}
};

// keywords found through out the file.
static const int RIFF_CHUNK_NAME_SIZE = 4;
static const int RIFF_CHUNK_NAMES_NO = 8;
static const char RIFF_CHUNK_NAMES[RIFF_CHUNK_NAMES_NO][RIFF_CHUNK_NAME_SIZE] = {
	{'L','I','S','T'},{'J','U','N','K' },
	{'i','d','x','1'},{'f','m','t',' '},
	{'d','a','t','a'},{'f','a','c','t'},
	{'F','I','L','T' },{'m','o','v','i' }
};

bool is_footer(const unsigned char *p, size_t offset, size_t offset_movi, size_t total_bytes){
	// parse and verify the last chunk.
	size_t size = 0, kw1 = 0, kw2 = 0, index = 0;
	unsigned long loc = 0;
	const unsigned char *s =  p + offset;
	// parse an idx1 chunk. - RIFF_CHUNK_NAMES[2] = "idx1"
	if( memcmp(s, RIFF_CHUNK_NAMES[2], RIFF_CHUNK_NAME_SIZE) == 0){
		if(offset + 12 > total_bytes) return false;
		size = (s[7] << 24) | (s[6] << 16) | (s[5] << 8) | (s[4]);
		if(size < 0 || ((size + offset + 8 ) > total_bytes)){
			return false;
		}
		index = size - 0x8; //size + 0x8 - 0x10;
		if(offset + index + 4 > total_bytes) return false;
		kw1 = (s[index + 3] << 24) | (s[index + 2] << 16) | (s[index + 1] << 8) | (s[index]);
		index += 4;
		if(offset + index + 4 > total_bytes) return false;
		//l = (s[index + 3] << 24) | (s[index + 2] << 16) | (s[index + 1] << 8) | (s[index]);
		index += 4;
		if(offset + index + 4 > total_bytes) return false;
		loc = (s[index + 3] << 24) | (s[index + 2] << 16) | (s[index + 1] << 8) | (s[index]);
		if((loc + offset_movi + 4)> total_bytes)
			return false;
		index = loc + offset_movi + 0xC;
		if(index + 4 > total_bytes) return false;
		kw2 = (p[index + 3] << 24) | (p[index + 2] << 16) | (p[index + 1] << 8) | (p[index]);
		if(kw2 == kw1)
			return true;
	}
	return false;
}

bool carve_from_keyword(const unsigned char *p, size_t offset, size_t total_bytes, size_t &length){
	size_t index = 0, size = 0, prev_index = 0;
	bool is_found = false;
	const unsigned char *s =  p + offset;
	size_t offset_movi = 0;

	for(;(offset + index) < total_bytes - 0x10;){
		is_found = false;

		for(int i = 0; i < RIFF_CHUNK_NAMES_NO; ++i){
			if( memcmp((s + index), RIFF_CHUNK_NAMES[i], RIFF_CHUNK_NAME_SIZE) == 0){
				is_found = true;
				break;
			}
		}
		// parse "LIST" to id "movi". calculate offset_movi
		if( memcmp((s + index), RIFF_CHUNK_NAMES[0], RIFF_CHUNK_NAME_SIZE) == 0){
			// we have a list chunk
			if( memcmp((s + index + 8), RIFF_CHUNK_NAMES[7], RIFF_CHUNK_NAME_SIZE) == 0){
				offset_movi = index + 8;
			}
		}
		// No key word found - implies that either we have reached end of file or the file is truncated.
		if(is_found == false){
			if(is_footer(p, (offset + prev_index), offset_movi, total_bytes)){
				length += index;
				return true;
			}
			else{
				length += prev_index + RIFF_CHUNK_NAME_SIZE;
				return false;
			}
		}

		size = (s[index + 7] << 24) | (s[index + 6] << 16) | (s[index + 5] << 8) | (s[index + 4]);
		if(size < 0 ) {
			length += index + RIFF_CHUNK_NAME_SIZE;
			return false;
		}
		prev_index = index;
		index += 8 + size;
	}

	if((index + 4) < total_bytes){
		if(is_footer(p, (offset + prev_index), offset_movi, total_bytes)){
			length += index;
			return true;
		}
		else{
			length += prev_index + RIFF_CHUNK_NAME_SIZE;
			return false;
		}
	}else{
		length += prev_index + RIFF_CHUNK_NAME_SIZE;
		return false;
	}
}


bool carve_from_header(const unsigned char *s, size_t total_bytes, size_t &length){
	size_t index = 0;
	bool is_found = false;
	index = RIFF_HEADER_SIZE + RIFF_LENGTH_SIZE;

	is_found = false;
	for(int i = 0; i < RIFF_SUB_HEADER_NO; ++i){
		if( memcmp((s + index), RIFF_SUB_HEADER[i], RIFF_HEADER_SIZE) == 0){
			is_found = true;
			break;
		}
	}

	if(is_found == false){
		return false;

	}
	index += RIFF_HEADER_SIZE;
	length += index;
	return carve_from_keyword(s, index, total_bytes, length);

}

extern "C"
	void  scan_avi(const class scanner_params &sp,const recursion_control_block &rcb)
{
	
	if(sp.version != 1){
		cerr << "scan_avi requires sp version 1; got version " << sp.version << "\n";
		exit(1);
	}

	if(sp.phase == 0){
		sp.info->name  = "avi";
		sp.info->feature_names.insert("avi");
		return;
	}

	if(sp.phase == 2) return;

	const sbuf_t &sbuf = sp.sbuf;
	
	feature_recorder_set &fs = sp.fs;
	feature_recorder *avi_recorder = fs.get_name("avi");
	avi_recorder->set_quoting_enabled(false);


	size_t offset = 0;
	size_t length = 0;
	int status = 0;
	const unsigned char* s = sbuf.buf;
	size_t l = sbuf.pagesize;
	char b2[1024];
	bool print = false;
	extract_keyframes ek;
	std::string file_name;

	for(const unsigned char *cc = s; ((cc < (s + l)) && (cc < (s + l - 0x10))); ++cc){

		if( memcmp(cc, RIFF_MAIN_HEADER, RIFF_HEADER_SIZE) == 0){
			offset = cc - s;
			status = carve_from_header(cc, (l - offset), length);
			// Status true implies that an intact file was recovered.
			if(status) {
				snprintf(b2,sizeof(b2), "\t%ld\t%ld\tC", offset, length);
				print = true;
				// Extract key frames based complete file in memory. This function also saves file to disk
				ek.extract(sbuf, offset, *avi_recorder);	
			}else{
				// partial header found, try reconstruction instead.
				//use the video processor to convert/store the video found
				video_processor vp;
				if(vp.process_header(sbuf, offset, *avi_recorder)){
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
			avi_recorder->write(sp.sbuf.pos0+offset, " ", b2);
			//avi_recorder->carve(sbuf,offset, length);
			print = false;
		}
		cc += length;
		length = 0;
		//avi_recorder->flush();
	}
	
	return;
}

