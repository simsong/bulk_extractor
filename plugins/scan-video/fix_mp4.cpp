#include "fix_mp4.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;
using namespace isomf;

// Scanner return status
typedef enum {
	SCANNER_STATUS_FAILURE = -1,
	SCANNER_STATUS_OK = 0,
	SCANNER_STATUS_LEAF
}SCANNER_STATUS;

// List of Keywords
const char fix_mp4::ATOM_NAMES[NO_OF_ATOMS ][LENGTH_OF_ATOM_NAME] = {
	{'m', 'd', 'a', 't'}, {'m', 'o', 'o', 'v'}, {'m', 'v', 'h', 'd'}, {'h', 'd', 'l', 'r'}, {'m', 'd', 'h', 'd'},
	{'m', 'd', 'i', 'a'}, {'t', 'k', 'h', 'd'}, {'t', 'r', 'a', 'k'}, {'s', 't', 'b', 'l'}, {'s', 't', 'c', 'o'},
	{'s', 't', 's', 'z'}, {'s', 't', 's', 'c'}, {'c', 'o', '6', '4'}, {'u', 'd', 't', 'a'}, {'e', 'd', 't', 's'},
	{'f', 'r', 'e', 'e'}, {'m', 'i', 'n', 'f'}, {'s', 't', 'd', 'p'}, {'s', 't', 's', 'd'}, {'e', 'l', 's', 't'},
	{'s', 't', 's', 'h'}, {'s', 't', 's', 's'}, {'s', 't', 't', 's'}, {'s', 't', 'z', '2'}, {'w', 'i', 'd', 'e'},
	{'p', 'n', 'o', 't'}, {'a', 'l', 'b', 'm'}, {'a', 'u', 't', 'h'}, {'b', 'p', 'c', 'c'}, {'b', 'x', 'm', 'l'},
	{'c', 'c', 'i', 'd'}, {'c', 'd', 'e', 'f'}, {'c', 'l', 's', 'f'}, {'c', 'm', 'a', 'p'}, {'c', 'o', 'l', 'r'},
	{'c', 'p', 'r', 't'}, {'c', 'r', 'h', 'd'}, {'c', 't', 't', 's'}, {'c', 'v', 'r', 'u'}, {'d', 'c', 'f', 'D'},
	{'d', 'r', 'e', 'f'}, {'d', 's', 'c', 'p'}, {'f', 'r', 'm', 'a'}, {'g', 'n', 'r', 'e'}, {'g', 'r', 'p', 'i'},
	{'h', 'm', 'h', 'd'}, {'i', 'c', 'n', 'u'}, {'i', 'h', 'd', 'r'}, {'i', 'i', 'n', 'f'}, {'i', 'l', 'o', 'c'},
	{'i', 'm', 'i', 'f'}, {'i', 'n', 'f', 'u'}, {'i', 'o', 'd', 's'}, {'i', 'p', 'h', 'd'}, {'i', 'p', 'm', 'c'},
	{'i', 'p', 'r', 'o'}, {'j', 'p', '2', 'c'}, {'j', 'p', '2', 'h'}, {'j', 'p', '2', 'i'}, {'k', 'y', 'w', 'd'},
	{'p', 'n', 'o', 't'}, {'l', 'o', 'c', 'i'}, {'l', 'r', 'c', 'u'}, {'m', '7', 'h', 'd'}, {'m', 'a', 't', 't'},
	{'m', 'd', 'r', 'i'}, {'m', 'e', 'h', 'd'}, {'m', 'e', 't', 'a'}, {'m', 'f', 'h', 'd'}, {'m', 'f', 'r', 'a'},
	{'m', 'f', 'r', 'o'}, {'m', 'j', 'h', 'd'}, {'m', 'o', 'o', 'f'}, {'m', 'v', 'e', 'x'}, {'n', 'm', 'h', 'd'},
	{'o', 'c', 'h', 'd'}, {'o', 'd', 'a', 'f'}, {'o', 'd', 'd', 'a'}, {'o', 'd', 'h', 'd'}, {'o', 'd', 'h', 'e'},
	{'o', 'd', 'r', 'b'}, {'o', 'd', 'r', 'm'}, {'o', 'd', 't', 't'}, {'o', 'h', 'd', 'r'}, {'p', 'a', 'd', 'b'},
	{'p', 'c', 'l', 'r'}, {'p', 'd', 'i', 'n'}, {'p', 'e', 'r', 'f'}, {'p', 'i', 't', 'm'}, {'r', 'e', 's', 'c'},
	{'k', 'm', 'a', 't'}, {'r', 'e', 's', 'd'}, {'r', 't', 'n', 'g'}, {'s', 'b', 'g', 'p'}, {'s', 'c', 'h', 'i'},
	{'s', 'c', 'h', 'm'}, {'s', 'd', 'e', 'p'}, {'s', 'd', 'h', 'd'}, {'s', 'd', 't', 'p'}, {'s', 'd', 'v', 'p'},
	{'s', 'g', 'p', 'd'}, {'s', 'i', 'n', 'f'}, {'s', 'k', 'i', 'p'}, {'s', 'm', 'h', 'd'}, {'s', 'u', 'b', 's'},
	{'t', 'f', 'h', 'd'}, {'t', 'f', 'r', 'a'}, {'t', 'i', 't', 'l'}, {'t', 'r', 'a', 'f'}, {'t', 'r', 'e', 'f'},
	{'t', 'r', 'e', 'x'}, {'t', 'r', 'u', 'n'}, {'d', 'i', 'n', 'f'}, {'t', 's', 'e', 'l'}, {'u', 'i', 'n', 'f'},
	{'u', 'l', 's', 't'}, {'u', 'u', 'i', 'd'}, {'v', 'm', 'h', 'd'}, {'y', 'r', 'r', 'c'}, {'c', 'l', 'i', 'p'},
	{'c', 'r', 'g', 'n'}, {'c', 't', 'a', 'b'}, {'i', 'm', 'a', 'p'}, {'l', 'o', 'a', 'd'}, {'l', 'g', 'p', 'n'},
	{'l', 'g', 't', 'n'}
};

fix_mp4::fix_mp4(){

	o = NULL;
	output_file_size = 0;

	_moov.num_tracks = 0;
	_moov.trak.clear();
	_moov.offset = 0;
	_moov.size = 0;

	_mdat.offset = _mdat.size = 0;
	reset(_tk);
	_stsd = NULL;
}

fix_mp4::~fix_mp4(){
	_moov.num_tracks = 0;
	_moov.trak.clear();
	reset(_tk);
	release_output_file();

}

bool fix_mp4::release_output_file(){
	if(NULL != o){
		delete []o;
		o =  NULL;
		output_file_size = 0;
	}
	return true;
}

void fix_mp4::reset(trak_box &tk){
	_tk.self.offset = _tk.tkhd.self.offset = _tk.mdia.self.offset = _tk.mdia.mdhd.self.offset = _tk.mdia.hdlr.offset =
		_tk.mdia.minf.self.offset = _tk.mdia.minf.vmhd.offset = _tk.mdia.minf.dinf.self.offset =
		_tk.mdia.minf.dinf.dref.offset = _tk.mdia.minf.stbl.self.offset = _tk.mdia.minf.stbl.stsd.self.offset =
		_tk.mdia.minf.stbl.stsd.codec.offset = _tk.mdia.minf.stbl.stts.offset = _tk.mdia.minf.stbl.stss.offset =
		_tk.mdia.minf.stbl.stsc.offset = _tk.mdia.minf.stbl.stsz.offset = _tk.mdia.minf.stbl.stco.offset = 0;

	_tk.self.present = _tk.tkhd.self.present = _tk.mdia.self.present = _tk.mdia.mdhd.self.present = _tk.mdia.hdlr.present =
		_tk.mdia.minf.self.present = _tk.mdia.minf.vmhd.present = _tk.mdia.minf.dinf.self.present =
		_tk.mdia.minf.dinf.dref.present = _tk.mdia.minf.stbl.self.present = _tk.mdia.minf.stbl.stsd.self.present =
		_tk.mdia.minf.stbl.stsd.codec.present = _tk.mdia.minf.stbl.stts.present = _tk.mdia.minf.stbl.stss.present =
		_tk.mdia.minf.stbl.stsc.present = _tk.mdia.minf.stbl.stsz.present = _tk.mdia.minf.stbl.stco.present = false;
	_tk.mdia.minf.stbl.stsd.children.clear();
	_tk.mdia.minf.stbl.stsd.name.clear();

	release_output_file();

}

bool fix_mp4::reconstruct_stsd(unsigned int width, unsigned int height, CODEC_TYPE ct){

	if(0 == width || 0 == height){
		std::cerr << __FILE__ << ":" << __LINE__ << "Invalid input parameters\n";
		return false;
	}
	/// The "stsd" box holds codec initialization parameters. Reconstruct according to codec type.
	switch(ct) {
	case CODEC_AVC:
		_stsd = box_factory::get_avc_stsd(width, height);
		break;
	case CODEC_MP4V:
		_stsd = box_factory::get_mp4v_stsd(width, height);
		break;
	case CODEC_S263:
		_stsd = box_factory::get_h263_stsd(width, height);
		break;
	case CODEC_MJPG:
	case CODEC_AMR:
	case CODEC_MP4A:
	case NO_CODEC:
	default: return false;
	}
	return true;
}

size_t fix_mp4::write_stsd(size_t index){
	// write stsd
	size_t sz = _stsd->size();
	size_t nb = _stsd->write((char *)(o + index), sz);
	fix_mp4::write_big_endian_int32(sz, o, index, output_file_size);
	return sz;
}

bool fix_mp4::reconstruct_stsz(const unsigned char *s, size_t l, CODEC_TYPE primary, CODEC_TYPE secondary, std::vector<size_t> &stsz_table){
	size_t index = 0;
	if((s == NULL) || (l == 0) || (_moov.num_tracks != _moov.trak.size()) || (_mdat.offset + _mdat.size > l)){
		std::cerr << __FILE__ << ":" << __LINE__ << "Invalid input parameters\n";
		return false;
	}

	stsz_table.clear();
	size_t primary_entry = 0, sec_entry = 0, next_entry = 0, table_entry = 0;
	index = _mdat.offset;
	bool do_sec = secondary != NO_CODEC;
	for(;index < _mdat.offset + _mdat.size; ){
		// get offset of the primary codec.
		if(!get_offset(s, l, index, primary, primary_entry))
			break;	
		// get the next offset of the primary codec
		if(!get_offset(s, l, (primary_entry + 1), primary, next_entry))
			break;	
		// get the offset of the secondary codec
		if(do_sec){
			if(!get_offset(s, l, index, secondary, sec_entry))
				break;	
		}

		// we can have consecutive samples from the same codec type. to calculate the sample sizes, use the value that is the closest.
		// the interleaving process need not be uniform.
		if(do_sec && (sec_entry < next_entry)){
			table_entry = sec_entry - primary_entry;	// interleaved samples
			index = sec_entry + 1;
		}else{
			table_entry = next_entry - primary_entry;	// non-interleaved samples
			index = primary_entry + 1;
		}
		stsz_table.push_back(table_entry); table_entry = 0;
	}

	// Hack for "mp4a", since the codec does not have a per sample signature, we are skipping audio samples entirely.
	// Since each sample has sufficient information for just one video frame and associated audio, no artifacts are
	// introduced to the video.
	if(1 == stsz_table.size()){
		index = _mdat.offset;
		get_offset(s, l, index, secondary, sec_entry);
		index = sec_entry + 1;
		for(;index < _mdat.offset + _mdat.size; ){
			// get offset of the primary codec.
			if(!get_offset(s, l, index, primary, primary_entry))
				break;	
			// get the next offset of the primary codec
			if(!get_offset(s, l, (primary_entry + 1), primary, next_entry))
				break;	

			// we can have consecutive samples from the same codec type. to calculate the sample sizes, use the value that is the closest.
			// the interleaving process need not be uniform.

			table_entry = next_entry - primary_entry;	// non-interleaved samples
			index = primary_entry + 1;
			stsz_table.push_back(table_entry); table_entry = 0;
		}

	}

	return true;
}

bool fix_mp4::identify_moov_tree(const unsigned char *s, size_t l){
	size_t index = 0, size = 0;
	size_t moov_idx = 0, mdat_idx = 0;
	bool moov = false, mdat = false;

	for(;((index < l - 0x8) && !(moov && mdat));){
		size = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);

		if( memcmp((s + index + LENGTH_SIZE), ATOM_NAMES[1], LENGTH_OF_ATOM_NAME) == 0){
			_moov.offset = moov_idx = index;
			moov = true;
		}else if( memcmp((s + index + LENGTH_SIZE), ATOM_NAMES[0], LENGTH_OF_ATOM_NAME) == 0){
			_mdat.offset = mdat_idx = index;
			_mdat.size =  (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
			mdat  = true;
		}
		if((size <= 0 )){
			break;
		}
		index += size;
	}

	// Reconstruction is possible only if "moov" occurs after "mdat" and start of "moov" is recovered.
	if((mdat_idx > moov_idx) || (!mdat ) || (!moov))
		return false;
	
	size = (s[moov_idx] << 24) | (s[moov_idx + 1] << 16) | (s[moov_idx + 2] << 8) | (s[moov_idx + 3]);
	if((moov_idx + size) > l){
		// Proceed even if the expected size is greater than actual size, Since we know the file is incomplete/partial
		_moov.offset = moov_idx;
		_moov.size = size;
		// parse the "moov" tree and identify which boxes are missing. 
		parse_moov((s + moov_idx), 0, size);
	}
	return true;
}

bool fix_mp4::parse_moov(const unsigned char *p, size_t offset, size_t total_bytes){
	size_t index = 0, size = 0;
	bool is_found = false;
	const unsigned char *s =  p + offset;

	if(total_bytes == 0)
		return true;
	// leaf atom check
	if(index < total_bytes - 0x8){
		is_found = false;
		for(int i = 0; i < NO_OF_ATOMS; ++i){
			if( memcmp((s + index + LENGTH_SIZE), ATOM_NAMES[i], LENGTH_OF_ATOM_NAME) == 0){
				is_found = true;
				break;
			}
		}
		if(is_found == false){
			return true;
		}
	}else {
		return true;
	}

	int curr_trak = _moov.num_tracks - 1;
	for(;index < total_bytes - 0x8;){

		if( memcmp((s + index + LENGTH_SIZE), "moov", LENGTH_OF_ATOM_NAME) == 0){
			//_moov.self.offset = index + offset; _moov.self.present = true;
		}else if(memcmp((s + index + LENGTH_SIZE), "mvhd", LENGTH_OF_ATOM_NAME) == 0){
			_moov.mvhd.offset = index + offset + offset; _moov.mvhd.present = true;
		}else if(memcmp((s + index + LENGTH_SIZE), "trak", LENGTH_OF_ATOM_NAME) == 0){
			reset(_tk);
			_tk.self.offset = index + offset; _tk.self.present = true;
			_moov.trak.push_back(_tk);
			curr_trak = _moov.num_tracks;
			++_moov.num_tracks;
		}else {
			if(curr_trak < _moov.num_tracks){
				if(memcmp((s + index + LENGTH_SIZE), "tkhd", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].tkhd.self.offset = index + offset;
					_moov.trak[curr_trak].tkhd.self.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "mdia", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.self.offset = index + offset;
					_moov.trak[curr_trak].mdia.self.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "mdhd", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.mdhd.self.offset = index + offset;
					_moov.trak[curr_trak].mdia.mdhd.self.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "hdlr", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.hdlr.offset = index + offset;
					_moov.trak[curr_trak].mdia.hdlr.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "minf", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.self.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.self.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "vmhd", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.vmhd.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.vmhd.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "smhd", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.smhd.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.smhd.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "dinf", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.dinf.self.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.dinf.self.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "dref", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.dinf.dref.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.dinf.dref.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "stbl", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.self.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.self.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "stsd", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.self.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.self.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "stts", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stts.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stts.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "stss", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stss.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stss.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "stsc", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stsc.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stsc.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "stsz", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stsz.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stsz.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "stco", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stco.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stco.present = true;
				}else if(memcmp((s + index + LENGTH_SIZE), "mp4v", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.codec.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.codec.present = true;
					_tk.mdia.minf.stbl.stsd.name.assign("mp4v");
					// child esds
				}else if(memcmp((s + index + LENGTH_SIZE), "mp4a", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.codec.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.codec.present = true;
					_tk.mdia.minf.stbl.stsd.name.assign("mp4a");
					//child esds
				}else if(memcmp((s + index + LENGTH_SIZE), "s263", LENGTH_OF_ATOM_NAME) == 0){
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.codec.offset = index + offset;
					_moov.trak[curr_trak].mdia.minf.stbl.stsd.codec.present = true;
					_tk.mdia.minf.stbl.stsd.name.assign("s263");
					//child d263
				}else{
				}
			}else{
			}
		}


		size = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
		if(size < 0 ) {
			return false;
		}else if (size == 1 ){
			//std::cerr << "64 bit atom size. Add support\n";
		}else if (size == 0 ){
			return false;
		}
		// recursively parse the tree upto the child atoms. we validate the entire structure.
		if(parse_moov(p, (offset + index + LENGTH_OF_ATOM_NAME + LENGTH_SIZE), (size - LENGTH_OF_ATOM_NAME - LENGTH_SIZE))){
			index += size;
		}else{
			return false;
		}
	}
	return true;
}

bool fix_mp4::reconstruct_stco(const unsigned char *s, size_t l, CODEC_TYPE ct, std::vector<size_t> &stco_table){
	size_t index = 0;
	
	// Check if input parameters are valid
	if((s == NULL) || (l == 0) || (_moov.num_tracks != _moov.trak.size()) || (_mdat.offset + _mdat.size > l)){
		std::cerr << __FILE__ << ":" << __LINE__ << "Invalid input parameters\n";
		return false;
	}

	//Since we won't know the number of entries, calculate the entries and add to vector. Translate vector entries to atom later
	stco_table.clear();
	size_t table_entry = 0;
	index = _mdat.offset;
	for(;index < _mdat.offset + _mdat.size; ){
		if(!get_offset(s, l, index, ct, table_entry))
			break;	
		index = table_entry + 1;	// +1 moves the pointer forward, s.t. the same signature is not picked up agn
		stco_table.push_back(table_entry); table_entry = 0;
	}

	return true;
}

bool fix_mp4::identify_codecs(const unsigned char *s, size_t l, std::vector<CODEC_TYPE> &codec_type){
	// This will look through the mdat box and identify the embedded codecs.
	// The output container will contain the codec signatures.

	// Go to the mdat box
	if(_mdat.offset + _mdat.size > l){
		std::cerr << __FILE__ << ":" << __LINE__ << "Invalid input parameters\n";
		return false;
	}

	size_t index = 0;
	bool video_found = false, audio_found = false;
	// search file signatures.
	for(index = 0; ((index < _mdat.size - 0xF) && !(video_found && audio_found)); ++index){

		if(!video_found && ((s[index] == 0) && (s[index + 1] == 0) && (s[index + 2] == 0) && (s[index + 4] == 0x6))){
			if((s[index + 3] == 0x0B) || (s[index + 3] == 0x0C)){
				// Add AVC to output
				codec_type.push_back(CODEC_AVC);
				video_found  = true;
			}
		}else if (!video_found && ((s[index] == 0xff) && (s[index + 1] == 0xd9) && (s[index + 2] == 0xFF) && (s[index + 3] == 0xDB))){
			// Add MJPG to output
			codec_type.push_back(CODEC_MJPG); video_found  = true;

		}else if (!video_found && ((s[index] == 0x0) && (s[index + 1] == 0x0)  && (s[index + 2] == 0x01)  && ((s[index + 3] == 0xB3) ||(s[index + 3] == 0xB6)))){
			// Add MP4V to output
			codec_type.push_back(CODEC_MP4V); video_found  = true;

		}else if (!video_found && ((s[index] == 0x0) && (s[index + 1] == 0x0)  && (s[index + 2] >= 0x80)  && (s[index + 2] < 0x84))) {
			// Add H263 to output
			codec_type.push_back(CODEC_S263); video_found  = true; // 3byte signature, high likelihood of false positives
			///if  0x00000000 are found immediately after, then the signature is not counted. use this for offset calc

		}else if (!audio_found && (((s[index] == 0xde) && (s[index + 1] == 0x04) && (s[index + 2] == 0x0) && (s[index + 3] == 0x0)
			&& (s[index + 4] == 0x6c) && (s[index + 5] == 0x69) && (s[index + 6] == 0x62) && (s[index + 7] == 0x66)
			&& (s[index + 8] == 0x61) && (s[index + 9] == 0x61) && (s[index + 10] == 0x63)) ||
			((s[index] == 0x21) && (s[index + 1] == 0x47) && (s[index + 2] == 0xFE) && (s[index + 3] == 0xFF)))){
				// Add MP4A to output DE04 00 00 "libfaac"
				codec_type.push_back(CODEC_MP4A); audio_found  = true;

		}else if (!audio_found && ((s[index] == 0x3c) && ((s[index + 1] == 0x91) || (s[index + 1] == 0x48) ||
			(s[index + 1] == 0x55)) && (s[index + 10] == 0x01) && (s[index + 11] == 0xE7))){
				// Add AMR to output
				codec_type.push_back(CODEC_AMR); audio_found  = true;

		}
	}

	if(codec_type.empty()) return false;
	return true;
}

bool fix_mp4::get_avc_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset){
	// from the given offset, it calculates the offset to the next signature for AVC
	if(( input_offset > l ) || (input_offset > (_mdat.size + _mdat.offset)))
		return false;
	const unsigned char *s = p + input_offset;
	size_t index;
	output_offset = 0;
	for(index = 0; ((input_offset + index < _mdat.offset + _mdat.size - 0x4)); ++index){
		if((s[index] == 0) && (s[index + 1] == 0) && (s[index + 2] == 0) && (s[index + 4] == 0x6)){
			if((s[index + 3] == 0x0B) || (s[index + 3] == 0x0C) || (s[index + 3] == 0x15)){
				output_offset = input_offset + index;
				break;
			}
		}
	}
	if(0 == output_offset) return false;

	return true;

}

bool fix_mp4::get_mp4v_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset){
	// from the given offset, it calculates the offset to the next signature for MP4V
	if(( input_offset > l ) || (input_offset > (_mdat.size + _mdat.offset)))
		return false;
	const unsigned char *s = p + input_offset;
	size_t index;
	output_offset = 0;
	for(index = 0; ((input_offset + index < _mdat.offset + _mdat.size - 0x4)); ++index){
		if((s[index] == 0x0) && (s[index + 1] == 0x0)  && (s[index + 2] == 0x01)  && ((s[index + 3] == 0xB3) ||(s[index + 3] == 0xB6))){
			output_offset = input_offset + index;
			break;
		}
	}
	if(0 == output_offset) return false;

	return true;

}

bool fix_mp4::get_s263_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset){
	// from the given offset, it calculates the offset to the next signature S263
	if(( input_offset > l ) || (input_offset > (_mdat.size + _mdat.offset)))
		return false;
	const unsigned char *s = p + input_offset;
	size_t index;
	output_offset = 0;
	for(index = 0; ((input_offset + index < _mdat.offset + _mdat.size - 0x8)); ++index){
		if((s[index] == 0x0) && (s[index + 1] == 0x0)  && (s[index + 2] >= 0x80)  && (s[index + 2] < 0x84)){
			if((s[index + 4] == 0x0) && (s[index + 5] == 0x0)  && (s[index + 6] == 0x0)  && (s[index + 7] == 0x0)){
				continue;
			}
			output_offset = input_offset + index;
			break;
		}
	}
	if(0 == output_offset)
		return false;

	return true;

}

bool fix_mp4::get_mjpg_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset){
	// from the given offset, it calculates the offset to the next signature MJPG
	if(( input_offset > l ) || (input_offset > (_mdat.size + _mdat.offset)))
		return false;
	const unsigned char *s = p + input_offset;
	size_t index;
	output_offset = 0;
	for(index = 0; ((input_offset + index < _mdat.offset + _mdat.size - 0x4)); ++index){
		if((s[index] == 0xff) && (s[index + 1] == 0xd8)  && (s[index + 2] == 0xff)  && (s[index + 3] == 0xdb)){
			output_offset = input_offset + index;
			break;
		}
	}
	if(0 == output_offset) return false;

	return true;
}

bool fix_mp4::get_amr_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset){
	// from the given offset, it calculates the offset to the next signature AMR
	if(( input_offset > l ) || (input_offset > (_mdat.size + _mdat.offset)))
		return false;
	const unsigned char *s = p + input_offset;
	size_t index;
	output_offset = 0;
	for(index = 0; ((input_offset + index < _mdat.offset + _mdat.size - 0xC)); ++index){
		if((s[index] == 0x3c) && ((s[index + 1] == 0x91) || (s[index + 1] == 0x48) ||
			(s[index + 1] == 0x55)) && (s[index + 10] == 0x01) && (s[index + 11] == 0xE7)){
				output_offset = input_offset + index;
				break;
		}
	}
	if(0 == output_offset) return false;

	return true;
}

bool fix_mp4::get_mp4a_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset){
	// from the given offset, it calculates the offset to the next signature MP4A
	if(( input_offset > l ) || (input_offset > (_mdat.size + _mdat.offset)))
		return false;
	const unsigned char *s = p + input_offset;
	size_t index;
	output_offset = 0;
	for(index = 0;((input_offset + index < _mdat.offset + _mdat.size -  0xB)); ++index){
		if(((s[index] == 0xde) && (s[index + 1] == 0x04) && (s[index + 2] == 0x0) && (s[index + 3] == 0x0)
			&& (s[index + 4] == 0x6c) && (s[index + 5] == 0x69) && (s[index + 6] == 0x62) && (s[index + 7] == 0x66)
			&& (s[index + 8] == 0x61) && (s[index + 9] == 0x61) && (s[index + 10] == 0x63)) ||
			((s[index] == 0x21) && (s[index + 1] == 0x47) && (s[index + 2] == 0xFE) && (s[index + 3] == 0xFF))){
				output_offset = input_offset + index;
				break;
		}
	}
	if(0 == output_offset)
		return false;
	return true;
}

bool fix_mp4::get_offset(const unsigned char *s, size_t l, size_t input_offset, CODEC_TYPE ct, size_t &output_offset){

	// from the given offset, it calculates the offset to the next signature.
	if(( input_offset > l ) || (input_offset > (_mdat.size + _mdat.offset)))
		return false;

	switch(ct) {
	case CODEC_AVC:
		return get_avc_offset(s, l, input_offset, output_offset);
	case CODEC_MP4V:
		return get_mp4v_offset(s, l, input_offset, output_offset);
	case CODEC_S263:
		return get_s263_offset(s, l, input_offset, output_offset);
	case CODEC_MJPG:
		return get_mjpg_offset(s, l, input_offset, output_offset);
	case CODEC_AMR:
		return get_amr_offset(s, l, input_offset, output_offset);
	case CODEC_MP4A:
		return get_mp4a_offset(s, l, input_offset, output_offset);
	case NO_CODEC:
	default: break;
	}
	return true;
}

bool fix_mp4::parse_tkhd(const unsigned char *s, size_t l, unsigned int trak_id){

	if(trak_id >= _moov.num_tracks)
		return false;
	tkhd_box *curr = &(_moov.trak[trak_id].tkhd);

	if((!curr->self.present) || (curr->self.offset + _moov.offset > l))
		return false;

	size_t index = curr->self.offset + _moov.offset;
	size_t length = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
	if(index + length > l)
		return false;
	curr->size = length;
	index += (LENGTH_OF_ATOM_ENTRY + LENGTH_OF_ATOM_ENTRY);
	curr->version = s[index];

	// Reference - ISO_14496-12_ISOBMFF.pdf : 8.3.2 Track Header Box
	// version 0, has 32 bit representation for time
	index += 74;

	if(curr->version){
		// version 1, has 64 bit representation for time
		index += 12;
	}

	curr->width = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
	index += LENGTH_OF_ATOM_ENTRY;
	curr->height = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);

	return true;
}


bool fix_mp4::parse_mdhd(const unsigned char *s, size_t l, unsigned int trak_id){

	if(trak_id >= _moov.num_tracks)
		return false;
	mdhd_box *curr = &(_moov.trak[trak_id].mdia.mdhd);

	if((!curr->self.present) || (curr->self.offset + _moov.offset > l))
		return false;

	size_t index = curr->self.offset + _moov.offset;
	size_t length = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
	curr->size = length;
	if(index + length > l)
		return false;

	curr->size = length;
	index += (LENGTH_OF_ATOM_ENTRY + LENGTH_OF_ATOM_ENTRY);
	curr->version = s[index];
	uint64_t lsb = 0, msb = 0;
	if(curr->version){
		// version 1, has 64 bit representation for time
		index += 24; //1+3+8+8+4
		msb = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
		index += 4;
		lsb = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
		curr->duration = ((msb & 0x00000000ffffffffL) << 32) | lsb;

	}else{
		// version 0, has 32 bit representation for time
		index += 16; //1+3+4+4+4
		curr->duration = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
	}
	return true;
}

bool fix_mp4::write_moov(const unsigned char *s, size_t total_bytes,  std::vector<size_t> &stco_table,  std::vector<size_t> &stsz_table){

	size_t length_to_mdhd =  _moov.offset + _moov.trak[0].mdia.mdhd.self.offset + _moov.trak[0].mdia.mdhd.size;
	if(NULL == s || 0 == total_bytes || NULL == _stsd){
		std::cerr << __FILE__ << ":" << __LINE__ << " Input parameters  are invalid \n";
		return false;
	}

	// Calculate output file length
	output_file_size = 0;
	if(1 > _moov.num_tracks)
		return false;

	// This includes only one of the reconstructed traks
	// Since the current trak reconstruction is possible if and only if the current trak's tkhd and mdia.mdhd boxes are present.
	// start point is set at the offset and size of the mdhd box for the first trak;

	// default atom sizes.
	const size_t HDLR_BOX_SZ = 45, VMHD_BOX_SZ = 20, DINF_BOX_SZ = 36, STTS_BOX_SZ = 24, STSC_BOX_SZ = 28;	;
	size_t stco_length = LENGTH_SIZE + LENGTH_OF_ATOM_NAME + LENGTH_OF_ATOM_ENTRY + LENGTH_OF_ATOM_ENTRY + (LENGTH_OF_ATOM_ENTRY * stco_table.size());
	size_t stsz_length = LENGTH_SIZE + LENGTH_OF_ATOM_NAME + LENGTH_OF_ATOM_ENTRY + LENGTH_OF_ATOM_ENTRY + LENGTH_OF_ATOM_ENTRY + (LENGTH_OF_ATOM_ENTRY * stsz_table.size());
	// Choose stsd
	size_t stsd_box_sz = _stsd->size();
	size_t index = _moov.offset;
	// If original file contains stsd, copy it over. Dont reconstruct
	if(_moov.trak[0].mdia.minf.stbl.stsd.self.present){
		index += _moov.trak[0].mdia.minf.stbl.stsd.self.offset;
		stsd_box_sz = (s[index] << 24) | (s[index + 1] << 16) | (s[index + 2] << 8) | (s[index + 3]);
	}

	size_t stbl_sz =  stsd_box_sz + stco_length +  stsz_length + STTS_BOX_SZ + STSC_BOX_SZ + LENGTH_OF_ATOM_NAME + LENGTH_SIZE;

	_moov.trak[0].mdia.minf.size = LENGTH_OF_ATOM_NAME + LENGTH_SIZE + stbl_sz + DINF_BOX_SZ + VMHD_BOX_SZ;

	output_file_size =  _moov.offset + _moov.trak[0].mdia.mdhd.self.offset + _moov.trak[0].mdia.mdhd.size + HDLR_BOX_SZ + _moov.trak[0].mdia.minf.size;
	// Allocate memory
	o = new (std::nothrow) unsigned char[output_file_size];
	if(NULL == o || length_to_mdhd >= output_file_size) return false;

	// Copy data until the mdhd box (ftyp, mdat, moov.tkhd, moov.trak.mdia.mdhd)
	if(length_to_mdhd > output_file_size) return false;
	memcpy(o, s, length_to_mdhd);

	// we need to correct the moov size, trak size and mdia size as per the new file size
	size_t mdia_sz = LENGTH_OF_ATOM_NAME + LENGTH_SIZE + _moov.trak[0].mdia.mdhd.size + _moov.trak[0].mdia.minf.size + HDLR_BOX_SZ;
	size_t trak_sz = LENGTH_OF_ATOM_NAME + LENGTH_SIZE + _moov.trak[0].tkhd.size + mdia_sz;
	size_t moov_sz = output_file_size - _moov.offset;

	index = _moov.offset;
	if(!fix_mp4::write_big_endian_int32(moov_sz, o, index, output_file_size)) return false;

	index = _moov.offset + _moov.trak[0].self.offset;
	if(!fix_mp4::write_big_endian_int32(trak_sz, o, index, output_file_size)) return false;

	index = _moov.offset + _moov.trak[0].mdia.self.offset;
	if(!fix_mp4::write_big_endian_int32(mdia_sz, o, index, output_file_size)) return false;

	index = length_to_mdhd;

	// write hdlr
	const unsigned char hdlr_arr[HDLR_BOX_SZ] = { 0x00, 0x00, 0x00, 0x2D, 0x68, 0x64, 0x6C, 0x72, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x56, 0x69, 0x64, 0x65, 0x6F, 0x48, 0x61, 0x6E, 0x64, 0x6C, 0x65, 0x72
	};
	if(index + HDLR_BOX_SZ > output_file_size) return false;
	memcpy((o + index), hdlr_arr, HDLR_BOX_SZ);
	index += HDLR_BOX_SZ;

	unsigned int kw = 0;
	//write minf
	if(!fix_mp4::write_big_endian_int32(_moov.trak[0].mdia.minf.size, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;
	kw = 0x6D696E66;	//minf
	if(!fix_mp4::write_big_endian_int32(kw, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;
	// vmhd and dinf boxes
	const size_t ARR_SZ = VMHD_BOX_SZ + DINF_BOX_SZ;
	const unsigned char minf_arr[ARR_SZ] = { 0x00, 0x00, 0x00, 0x14, 0x76, 0x6D, 0x68, 0x64, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x64, 0x69, 0x6E, 0x66, 0x00,
		0x00, 0x00, 0x1C, 0x64, 0x72, 0x65, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x0C, 0x75, 0x72, 0x6C, 0x20, 0x00, 0x00, 0x00, 0x01
	};
	if(index + ARR_SZ > output_file_size) return false;
	memcpy((o + index), minf_arr, ARR_SZ);
	index += ARR_SZ;

	//write stbl
	if(!fix_mp4::write_big_endian_int32(_moov.trak[0].mdia.minf.size, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;
	kw = 0x7374626C;	//stbl
	if(!fix_mp4::write_big_endian_int32(kw, o, index, output_file_size))
		return false;
	index += LENGTH_OF_ATOM_ENTRY;

	// write stsd
	// If original file contains stsd, copy it over. Dont reconstruct
	if(_moov.trak[0].mdia.minf.stbl.stsd.self.present){
		size_t input_index = _moov.offset + _moov.trak[0].mdia.minf.stbl.stsd.self.offset;
		if((index + stsd_box_sz) > output_file_size) return false;
		if((input_index + stsd_box_sz) > total_bytes) return false;
		memcpy((o + index), (s + input_index), stsd_box_sz);
		index += stsd_box_sz;
	}else{
		size_t check = write_stsd(index);
		if(0 == check) return false;
		index += check;
	}
	// write stts: Implementation - The mdhd must be parsed and we need to have a "valid" value for the duration & stsz must be written out to know the number of samples.
	// default implementation where all the samples have the same delta value.

	//if(trak_id >= _moov.num_tracks)
	//	return false;
	size_t trak_id = 0;

	if(!fix_mp4::write_big_endian_int32(STTS_BOX_SZ, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	kw = 0x73747473;	//stts
	if(!fix_mp4::write_big_endian_int32(kw, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	if(!fix_mp4::write_big_endian_int32(0, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	unsigned int num_entries = 1;
	if(!fix_mp4::write_big_endian_int32(num_entries, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	unsigned int sample_count = stsz_table.size();
	if(!fix_mp4::write_big_endian_int32(sample_count, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	unsigned int delta = static_cast<unsigned int> (_moov.trak[trak_id].mdia.mdhd.duration * 1.0 / sample_count);
	if(0 == delta) delta = 1;
	if(!fix_mp4::write_big_endian_int32(delta, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	//write stsc
	// 0000001C737473630000000000000001000000010000000100000001
	if(!fix_mp4::write_big_endian_int32(STSC_BOX_SZ, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	kw = 0x73747363;	//stsc
	if(!fix_mp4::write_big_endian_int32(kw, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	if(!fix_mp4::write_big_endian_int32(0, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	num_entries = 1;
	if(!fix_mp4::write_big_endian_int32(num_entries, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	unsigned int first_chunk = 1;
	if(!fix_mp4::write_big_endian_int32(first_chunk, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	unsigned int samples_per_chunk = 1;
	if(!fix_mp4::write_big_endian_int32(samples_per_chunk, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	unsigned int sample_description_index = 1;
	if(!fix_mp4::write_big_endian_int32(sample_description_index, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	//write stsz
	if(!fix_mp4::write_big_endian_int32(stsz_length, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	kw = 0x7374737A;	//stsz
	if(!fix_mp4::write_big_endian_int32(kw, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	if(!fix_mp4::write_big_endian_int32(0, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	if(!fix_mp4::write_big_endian_int32(0, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	num_entries = stsz_table.size();
	if(!fix_mp4::write_big_endian_int32(num_entries, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	for(size_t i = 0; i < num_entries; ++i){
		if(!fix_mp4::write_big_endian_int32(stsz_table[i], o, index, output_file_size)) return false;
		index += LENGTH_OF_ATOM_ENTRY;
	}

	// write stco
	num_entries = stco_table.size();
	if(!fix_mp4::write_big_endian_int32(stco_length, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	kw = 0x7374636F;	//stco
	if(!fix_mp4::write_big_endian_int32(kw, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	if(!fix_mp4::write_big_endian_int32(0, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	if(!fix_mp4::write_big_endian_int32(num_entries, o, index, output_file_size)) return false;
	index += LENGTH_OF_ATOM_ENTRY;

	for(size_t i = 0; i < num_entries; ++i){
		if(!fix_mp4::write_big_endian_int32(stco_table[i], o, index, output_file_size)) return false;
		index += LENGTH_OF_ATOM_ENTRY;
	}

	return true;
}

bool fix_mp4::write_to_file(const std::string& out_file_name){
	if(out_file_name.empty())
		return false;

	std::ofstream fp(out_file_name.c_str(), std::ios_base::out | std::ios_base::binary);
	if(NULL == fp)
		return false;

	fp.write((char*) o, output_file_size);

	bool ok = ! fp.bad();
	if (! ok) {
		std::cerr << __FILE__ << ":" << __LINE__ << " Unable to create output file\n";
		return false;
	}

	fp.close();
	release_output_file();
	return true;
}

bool fix_mp4::do_fix(const sbuf_t& sbuf, size_t offset, feature_recorder& recorder){
	
	unsigned char* s = (unsigned char*) (sbuf.buf + offset);
	size_t len = sbuf.pagesize;
	size_t l = len - offset;
	const pos0_t &pos0 = sbuf.pos0;
	std::string fpath = pos0.path;


	if(!identify_moov_tree(s,l)){
		// parses the moov structure. If the mdat box occurs after the moov box,
		// then there is nothing to reconstruct.
		std::cerr << __FILE__ << ":" << __LINE__ << " Reconstruction for this file is not supported as mdat box occurs at the end of file\n";
		return false;
	}
	std::vector<CODEC_TYPE> ct;

	// identify the embedded codec types. This is needed to reconstruct the tables
	if(!identify_codecs(s,l, ct)){
		std::cerr << __FILE__ << ":" << __LINE__ << " Unsupported codec types embedded in video, Reconstruction failed\n";
		return false;
	}

	size_t sz = ct.size();
	if(sz == 0){
		std::cerr << __FILE__ << ":" << __LINE__ << " Unsupported codec types embedded in video, Reconstruction failed\n";
		return false;
	}else if(sz == 1)
		ct.push_back(NO_CODEC);	// no audio codec detected


	// parse tkhd to identify the height and width - needed for "stsd" reconstruction
	if(!parse_tkhd(s, l, 0)){
		std::cerr << __FILE__ << ":" << __LINE__ << " Invalid tkhd box - unable to calculate duration of the stream, Reconstruction failed\n";
		return false;
	}

	// parse mdhd to identify the duration - needed for "stts" reconstruction
	if(!parse_mdhd(s, l, 0)){
		std::cerr << __FILE__ << ":" << __LINE__ << " Invalid mdhd box - unable to calculate duration, height and width of the video stream, Reconstruction failed\n";
		return false;
	}

	// Based on the identified codecs, reconstruct the tables.
	if(!reconstruct_stsd(_moov.trak[0].tkhd.width, _moov.trak[0].tkhd.height, ct[0])){
		std::cerr << __FILE__ << ":" << __LINE__ << " Reconstruction of the stsd box failed, Invalid codec initialization parameters" << std::endl;
		return false;
	}

	std::vector<size_t> stco, stsz;
	if(!reconstruct_stco(s, l, ct[0], stco)){
		std::cerr << __FILE__ << ":" << __LINE__ << " Reconstruction of the stco box failed, Invalid index table" << std::endl;
		return false;
	}

	if(!reconstruct_stsz(s, l, ct[0], ct[1], stsz)){
		std::cerr << __FILE__ << ":" << __LINE__ << " Reconstruction of the stsz box failed, Invalid index table" << std::endl;	
		return false;
	}
	
	if(!write_moov(s, l, stco, stsz)){
		std::cerr << __FILE__ << ":" << __LINE__ << "Failed to reconstruct playback information" << std::endl;
		return false;
	}

	std::stringstream ss_a, ss_b;
	ss_a << recorder.name << "-p" << fpath << "-" << pos0.offset
		<< "-" << offset << "-" << rand() << "." << recorder.name;


	std::string outfile_a = recorder.outdir + "/" + ss_a.str();
	set_file_name(outfile_a);
	std::stringstream i;
	i << "file_size: " << output_file_size << " height : " <<  _moov.trak[0].tkhd.height <<
		" width : " <<  _moov.trak[0].tkhd.width << " frame_count: " << stsz.size() <<" file_name: "  ;

	if(!write_to_file(outfile_a)){
		std::cerr << __FILE__ << ":" << __LINE__ << "Failed to write reconstructed file to disk" << std::endl;
		return false;
	}

	// Record changes to output file.
	recorder.write(pos0+offset, i.str(), ss_a.str());
	
	return true;
}


