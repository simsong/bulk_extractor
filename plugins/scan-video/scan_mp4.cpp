/**
*
* scan_mp4:
*/

#include <iostream>
#include <sys/types.h>
#include <sstream>
#include "../src/bulk_extractor.h"
#include "fix_mp4.h"

#include "video_processor.h"
#include "extract_keyframes.h"

// Header keywords - signatures found at the start of file
static const char MP4_MAIN_HEADER[fix_mp4::HEADER_SIZE + 1] = {'f', 't', 'y', 'p'};
static const int MP4_SUB_HEADER_NO = 5;
static const char MP4_SUB_HEADER[MP4_SUB_HEADER_NO][fix_mp4::HEADER_SIZE] = {
	{'i', 's', 'o', 'm'}, {'i', 's', 'o', '2'}, {'a', 'v', 'c', '1'}, {'m', 'p', '4', '1'}, {'m', 'p', '4', '2'},
};

// Scanner return status
typedef enum {
	SCANNER_STATUS_FAILURE = -1,
	SCANNER_STATUS_OK = 0,
	SCANNER_STATUS_LEAF
}SCANNER_STATUS;

// Parse last 2 entries of all the "stco" chunks.
bool parse_stco(const unsigned char *p, size_t offset, size_t total_bytes, std::vector<size_t> &arr){
	// Ref: http://wiki.multimedia.cx/index.php?title=QuickTime_container#stco
	size_t index = 0, size = 0;
	const unsigned char *s =  p + offset;
	const char ATOM_STCO[fix_mp4::LENGTH_OF_ATOM_NAME] = {'s', 't', 'c', 'o'};

	if( memcmp((s + fix_mp4::LENGTH_SIZE), ATOM_STCO, fix_mp4::LENGTH_OF_ATOM_NAME) != 0)
		return false;
	size = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3]);
	size_t num_entries = (s[12] << 24) | (s[13] << 16) | (s[14] << 8) | (s[15]);
	size_t loop_sz = 20;
	if(loop_sz > num_entries) loop_sz = num_entries;
	if((size < 0) || (size + offset > total_bytes) || ((loop_sz *  fix_mp4::LENGTH_SIZE) > size))
		return false;


		index = (size - (fix_mp4::LENGTH_SIZE * loop_sz));
	if(index + offset > total_bytes) return false;
	size_t stco_tmp;
	for(size_t i = 0; i < loop_sz; ++i){
		// the values of the stco entries will not be validated here. They should be checked at the time of usage.
		// This is so, because we dont have the complete stbl info here.
		stco_tmp = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
		arr.push_back(stco_tmp);
		index += fix_mp4::LENGTH_SIZE; if(index + offset > total_bytes) return false;
	}

	return true;
}

bool parse_stsz(const unsigned char *p, size_t offset, size_t total_bytes, std::vector<size_t> &arr){
	// Ref: http://wiki.multimedia.cx/index.php?title=QuickTime_container#stsz

	size_t index = 0, size = 0;
	const unsigned char *s =  p + offset;
	const char ATOM_STSZ[fix_mp4::LENGTH_OF_ATOM_NAME] = {'s', 't', 's', 'z'};

	if( memcmp((s + fix_mp4::LENGTH_SIZE), ATOM_STSZ, fix_mp4::LENGTH_OF_ATOM_NAME) != 0)
		return false;
	size = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3]);
	size_t num_entries = (s[16] << 24) | (s[17] << 16) | (s[18] << 8) | (s[19]);
	size_t loop_sz = 20;
	if(loop_sz > num_entries) loop_sz = num_entries;

	if((size < 0) || (size + offset > total_bytes) || ((loop_sz *  fix_mp4::LENGTH_SIZE) > size))
		return false;

	index = fix_mp4::LENGTH_OF_ATOM_NAME + (fix_mp4::LENGTH_SIZE * 2);


	size_t u_sz = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
	index  += fix_mp4::LENGTH_SIZE;
	size_t stsz_tmp = 0;
	if(0 == u_sz){
		// Mode - 2
		index =  (size - (fix_mp4::LENGTH_SIZE * loop_sz));
		if(index + offset > total_bytes) return false;

		for(size_t i = 0; i < loop_sz; ++i){
			stsz_tmp = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
			arr.push_back(stsz_tmp);
			index += fix_mp4::LENGTH_SIZE; if(index + offset > total_bytes) return false;
		}
	}else{
		// Mode - 1
		for(size_t i = 0; i < loop_sz; ++i){
			arr.push_back(u_sz);
		}
	}
	return true;
}


bool parse_stsc(const unsigned char *p, size_t offset, size_t total_bytes, std::vector<size_t> &arr){
	//Ref - isoiec 14496-12.pdf 8.7.4.2 Syntax  We are not parsing the entire stsc chunk,
	// here we are just going to parse the "samples_per_chunk"
	size_t index = 0, size = 0;
	const unsigned char *s =  p + offset;
	const char ATOM_STSC[fix_mp4::LENGTH_OF_ATOM_NAME] = {'s', 't', 's', 'c'};

	if( memcmp((s + fix_mp4::LENGTH_SIZE), ATOM_STSC, fix_mp4::LENGTH_OF_ATOM_NAME) != 0)
		return false;
	size = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3]);
	size_t num_entries = (s[12] << 24) | (s[13] << 16) | (s[14] << 8) | (s[15]);
	size_t loop_sz = 20;
	if(loop_sz > num_entries) loop_sz = num_entries;

	if((size < 0) || (size + offset > total_bytes)/* || ((loop_sz *  fix_mp4::LENGTH_SIZE) > size)*/)
		return false;

	index = fix_mp4::LENGTH_OF_ATOM_NAME + (fix_mp4::LENGTH_SIZE * 3);
	size_t stsc_tmp = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
	index =  (size - (fix_mp4::LENGTH_SIZE * 2));
	for(size_t i = 0; i < loop_sz; ++i){
		arr.push_back(stsc_tmp);
	}
	return true;
}

bool is_footer(const unsigned char *p, size_t offset, size_t total_bytes, int last_atom_idx,
	std::vector<size_t> &stco, std::vector<size_t> &stsz, std::vector<size_t> &stsc){
		// based on all the elements found, we will try to validate the end of file. 
		// the last atom is "mdat" we cannot validate this file.
		if(last_atom_idx == 0)
			return false;

		if((last_atom_idx == 9) || (last_atom_idx == 10) || (last_atom_idx == 11)){	// stco, stsz, stsc
			if(stco.size() != stsz.size()) return false;
			// 1. use the largest stco entry and its corresponding stsc and stsz entries and check for the presence of a kw.
			// * also mark the id for the second largest entry.
			size_t max_id = 0, prev_id = 0, max = 0;
			for(size_t i = 0; i < stco.size(); ++i){
				if(stco[i] > max) {
					max = stco[i];
					prev_id = max_id;
					max_id = i;
				}
			}
			size_t offset_2_kw = stco[max_id] + (stsc[0] * stsz[max_id]);
			if((offset_2_kw  + fix_mp4::LENGTH_SIZE + fix_mp4::LENGTH_OF_ATOM_NAME) >  total_bytes) return false;
			int index = offset_2_kw  + fix_mp4::LENGTH_SIZE;
			bool is_found = false;
			for(int i = 0; i <  fix_mp4::NO_OF_ATOMS; ++i){
				if( memcmp((p + index),  fix_mp4::ATOM_NAMES[i], fix_mp4::LENGTH_OF_ATOM_NAME) == 0){
					is_found = true;
					break;
				}
			}
			if(!is_found) return false;

			bool is_footer = false;
			// 2. if stco is the last entry then try to validate the last entry.
			if(last_atom_idx == 9){
				// if max_id is the last entry, then choose the next largest entry add the stsz
				// else subtract
				size_t last_id = stco.size() - 1;
				size_t check_offset = 0;
				if(max_id == last_id){
					check_offset = stco[prev_id] + stsz[prev_id];
					if(check_offset == stco[max_id]){
						is_footer = true;
					}
				}else if(prev_id == last_id){
					check_offset = stco[max_id] - stsz[max_id];
					if(check_offset == stco[prev_id]){
						is_footer = true;
					}
				}else{
					for(size_t i = 0; i < last_id; ++i){
						check_offset = stco[i] + stsz[i];
						if(check_offset == stco[last_id]){
							is_footer = true;
							break;
						}
					}
				}

			}
			return is_footer;
		}
		return false;
}

SCANNER_STATUS carve_from_keyword(const unsigned char *p, size_t offset, size_t total_bytes, size_t &length, int &last_atom_idx, std::vector<size_t> &stco, std::vector<size_t> &stsz, std::vector<size_t> &stsc, bool footer){
	size_t index = 0, size = 0, prev_index = 0;
	bool is_found = false;
	const unsigned char *s =  p + offset;
	
	if(total_bytes == 0)
		return SCANNER_STATUS_OK;
	// leaf atom check - if we have reached a leaf atom, we can terminate the recursive search for children atoms.
	if((offset + index) < total_bytes - 0x8){
		is_found = false;
		for(int i = 0; i <  fix_mp4::NO_OF_ATOMS; ++i){
			if( memcmp((s + index + fix_mp4::LENGTH_SIZE),  fix_mp4::ATOM_NAMES[i], fix_mp4::LENGTH_OF_ATOM_NAME) == 0){
				is_found = true;
				last_atom_idx = i;
				break;
			}
		}
		if(is_found == false){
			return SCANNER_STATUS_LEAF;
		}
	}else {
		return SCANNER_STATUS_LEAF;
	}
	size_t child_length = 0;
	for(;(offset + index) < total_bytes - 0x8;){
		is_found = false;
		for(int i = 0; i <  fix_mp4::NO_OF_ATOMS; ++i){
			if( memcmp((s + index + fix_mp4::LENGTH_SIZE),  fix_mp4::ATOM_NAMES[i], fix_mp4::LENGTH_OF_ATOM_NAME) == 0){
				is_found = true;
				last_atom_idx = i;
				break;
			}
		}
		// No key word found - implies that either we have reached end of file or the file is truncated.
		if(is_found == false){
			if(footer){
				if(is_footer(p, offset, total_bytes, last_atom_idx, stco, stsz, stsc)){
					length += index;
					return SCANNER_STATUS_OK;
				}
				else{
					length += prev_index + fix_mp4::LENGTH_OF_ATOM_NAME + fix_mp4::LENGTH_SIZE;
					return SCANNER_STATUS_FAILURE;
				}
			}else{
				if(index == total_bytes){
					length += index;
				}else{
					length += prev_index + fix_mp4::LENGTH_OF_ATOM_NAME;
				}
				return SCANNER_STATUS_OK;
			}
		}

		size = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
		if(size < 0 ) {
			length += index + fix_mp4::LENGTH_OF_ATOM_NAME + fix_mp4::LENGTH_SIZE;
			return SCANNER_STATUS_FAILURE;
		}else if(size + index + offset > total_bytes){
			return SCANNER_STATUS_FAILURE;
		}

		if(last_atom_idx == 9){
			// stco
			if(!parse_stco((s + index), 0, size, stco)){
				length += index + fix_mp4::LENGTH_OF_ATOM_NAME + fix_mp4::LENGTH_SIZE;
				return SCANNER_STATUS_FAILURE;
			}
		}else if(last_atom_idx == 10){
			//stsz
			if(!parse_stsz((s + index), 0, size, stsz)){
				length += index + fix_mp4::LENGTH_OF_ATOM_NAME + fix_mp4::LENGTH_SIZE;
				return SCANNER_STATUS_FAILURE;
			}
		}else if(last_atom_idx == 11){
			//stsc
			if(!parse_stsc((s + index), 0, size, stsc)){
				length += index + fix_mp4::LENGTH_OF_ATOM_NAME + fix_mp4::LENGTH_SIZE;
				return SCANNER_STATUS_FAILURE;
			}
		}

		prev_index = index;
		// recursively parse the tree upto the child atoms. we validate the entire structure.
		if(SCANNER_STATUS_FAILURE != carve_from_keyword((s + index), (fix_mp4::LENGTH_OF_ATOM_NAME + fix_mp4::LENGTH_SIZE), (size - fix_mp4::LENGTH_OF_ATOM_NAME - fix_mp4::LENGTH_SIZE),
			child_length, last_atom_idx,  stco, stsz, stsc, false)){
				index += size;
		}else{
			length += index + fix_mp4::LENGTH_OF_ATOM_NAME + child_length + fix_mp4::LENGTH_SIZE;
			return SCANNER_STATUS_FAILURE;
		}
	}

	// Footer check is performed only at the first call of carve_from_keyword. Subsequently, we parse for the children atoms. 
	if(footer){
		if(is_footer(p, offset, total_bytes, last_atom_idx, stco, stsz, stsc)){
			length += index;
			return SCANNER_STATUS_OK;
		}
		else{
			length += prev_index + fix_mp4::LENGTH_OF_ATOM_NAME + fix_mp4::LENGTH_SIZE;
			return SCANNER_STATUS_FAILURE;
		}
	}
	return SCANNER_STATUS_OK;
}

bool carve_from_header(const unsigned char *s, size_t total_bytes, size_t &length){
	size_t index = 0, size = 0;
	index =  fix_mp4::HEADER_SIZE + fix_mp4::LENGTH_SIZE;
	
	size = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3]);
	if(size < 0 ) {
		length += index + fix_mp4::LENGTH_OF_ATOM_NAME;
		return false;
	}
	index = size;
	length += index;
	int last_atom_idx = -1;
	std::vector<size_t> stco, stsz, stsc;
	if(carve_from_keyword(s, index, total_bytes, length, last_atom_idx, stco, stsz, stsc, true) == SCANNER_STATUS_OK){
		return true;
	}else
		return false;
}


extern "C"
	void  scan_mp4(const class scanner_params &sp,const recursion_control_block &rcb)
{
	if(sp.version != 1){
		cerr << "scan_mp4 requires sp version 1; got version " << sp.version << "\n";
		exit(1);
	}

	if(sp.phase == 0){
		sp.info->name  = "mp4";
		sp.info->feature_names.insert("mp4");
		return;
	}

	if(sp.phase == 2) return;

	const sbuf_t &sbuf = sp.sbuf;

	feature_recorder_set &fs = sp.fs;
	feature_recorder *recorder = fs.get_name("mp4");
	recorder->set_quoting_enabled(false);

	const unsigned char* s = sbuf.buf;
	size_t l = sbuf.pagesize;
	char b2[1024];
	bool print = false;

	size_t offset = 0;
	size_t length = 0;
	bool is_found = false, status;
	fix_mp4 fixer;
	extract_keyframes ek;
	std::string file_name;

	for(const unsigned char *cc = s; ((cc < (s + l)) && (cc < (s + l - 0x10))); ++cc){

		if( memcmp((cc + fix_mp4::LENGTH_SIZE), MP4_MAIN_HEADER,  fix_mp4::HEADER_SIZE) == 0){
			is_found = false;
			for(int i = 0; i < MP4_SUB_HEADER_NO; ++i){
				if( memcmp((cc +   fix_mp4::HEADER_SIZE + fix_mp4::LENGTH_SIZE), MP4_SUB_HEADER[i],  fix_mp4::HEADER_SIZE) == 0){
					is_found = true;
					break;
				}
			}
			if(!is_found)
				continue;		// Not an MP4 file
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
				status = true;
				if(!vp.process_header(sbuf, offset, *recorder)){
					// Video processor failed, Try to fix the video using fixmp4
					if(!fixer.do_fix(sbuf, offset, *recorder)){
						status = false;
					}else{
						file_name = fixer.get_file_name();
					}
				}else{
					file_name = vp.get_file_name();
				}
				// Extract key frames based on output file
				if(!file_name.empty() && status){
					ek.extract(file_name);	
					file_name.clear();
				}

			}

		}

		if(print){
			recorder->write(sp.sbuf.pos0+offset, " ", b2);
			print = false;
		}
		cc += length;
		length = 0;
	}
	
	return;
}

