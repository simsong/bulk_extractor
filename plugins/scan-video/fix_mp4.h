#ifndef INCLUDED_FIX_MP4
#define INCLUDED_FIX_MP4

#include "isomf.h"
#include "isomf_factory.h"
#include "type_io.h"
#include "../src/bulk_extractor.h"

#include <iostream>
#include <sys/types.h>
#include <sstream>
#include <vector>
#include <stdint.h>


/// The below structures implement the hierarchical box structure for the ISO-MBFF (ISO_14496-12_ISOBMFF.pdf)

typedef struct {
	bool present;		//Is the box present?
	size_t offset;		// Offset from the start of file
}box_id;

typedef struct{
	box_id self;
	box_id codec;
	std::vector<box_id> children;
	std::string name;
}stsd_box;


typedef struct{
	box_id self;
	stsd_box stsd;
	box_id stts;
	box_id stss;
	box_id stsc;
	box_id stsz;
	box_id stco;
}stbl_box;

typedef struct{
	box_id self;
	box_id dref;
}dinf_box;

typedef struct{
	box_id self;
	size_t size;
	box_id vmhd;
	box_id smhd;
	dinf_box dinf;
	stbl_box stbl;
}minf_box;

typedef struct{
	box_id self;
	unsigned int version;
	uint64_t duration;
	size_t size;
}mdhd_box;

typedef struct{
	box_id self;
	mdhd_box mdhd;
	box_id hdlr;
	minf_box minf;	
}mdia_box;


typedef struct{
	box_id self;
	size_t size;
	unsigned int version;
	unsigned int height;
	unsigned int width;
}tkhd_box;

typedef struct{
	box_id self;
	tkhd_box tkhd;
	mdia_box mdia;
}trak_box;

typedef struct{
	size_t num_tracks;
	box_id mvhd;
	std::vector<trak_box> trak;
	size_t offset;
	size_t size;
}moov_box;

typedef struct{
	size_t offset;
	size_t size;
}mdat_box;

/// Supported Codec types
typedef enum {
	NO_CODEC = -1,
	CODEC_AVC = 0,
	CODEC_MP4V,		//1
	CODEC_S263,		//2
	CODEC_MJPG,		//3
	CODEC_AMR,		//4
	CODEC_MP4A		//5
}CODEC_TYPE;


/// \class fix_mp4 fix_mp4.h
/// \brief The class implements all the methods needed to identify and reconstruct
/// missing playback information for MP4, MOV, 3G2, 3GP files
///

class fix_mp4{

public:
	
	/// Default Constructor
	fix_mp4();

	/// Default Destructor
	~fix_mp4();

	/// \brief - main worker function, manages all the calls
	/// Given a memory buffer, it identifies & estimates the missing playback chunks.
	/// It writes the output file to folder specified by hte recorder.
	bool do_fix(const sbuf_t& sbuf, size_t offset, feature_recorder& recorder);
	
	/// Write the new file to disk
	unsigned char* get_output_file_ptr(){ return o;}
	size_t get_output_file_size(){ return output_file_size;}
	bool write_to_file(const std::string& o);
	/// Frees the memory allocated for the newly reconstructed file. Called after write to disk
	bool release_output_file();

	/// Tracks the output file name. This is needed extract_keyframe module
	void set_file_name(const std::string &o) { _outfile.assign(o);}
	const std::string& get_file_name() {return _outfile;}

	/// Constants that define various box parameters - 
	static const int HEADER_SIZE = 4;
	static const int LENGTH_SIZE = 4;
	static const int LENGTH_OF_ATOM_NAME = 4;
	static const int LENGTH_OF_ATOM_ENTRY = 4;
	static const int NO_OF_ATOMS = 126;
	static const char ATOM_NAMES[NO_OF_ATOMS][LENGTH_OF_ATOM_NAME];
	
private:
	/// The below methods reconstruct the various "boxes" that are part of the playback information - "moov" box.
	/// Each method reconstructs a different "box". They analyze the input media data "mdat" box and reconstruct
	/// appropriate information. The embedded codec type is identified and used for the reconstruction.

	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - length of buffer
	/// \param(ct) - embedded codec type 
	/// \param(stco_table) - index of chunk offset values. (output parameter)
	/// \return - status of function
	///
	bool reconstruct_stco(const unsigned char *s, size_t l, CODEC_TYPE ct, std::vector<size_t> &stco_table);

	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - length of buffer
	/// \param(primary, secondary) - embedded audio and video codec types 
	/// \param(stsz_table) - index of sample size values. (output parameter)
	/// \return - status of function
	///
	bool reconstruct_stsz(const unsigned char *s, size_t l, CODEC_TYPE primary, CODEC_TYPE secondary, std::vector<size_t> &stsz_table);
	
	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - length of buffer
	/// \param(ct) - embedded audio and video codec types 
	/// \return - status of function
	///
	bool reconstruct_stsd(unsigned int width, unsigned int height, CODEC_TYPE ct);
	
	/// \brief This function identifies the location of the moov box and the mdat box
	/// and determines if the file needs to be reconstructed.
	/// \param(s) - Pointer to input buffer
	/// \param(l) - length of buffer
	/// \return - status of function
	///
	bool identify_moov_tree(const unsigned char *s, size_t l);

	/// \brief This function parses the moov box identifies which boxes are present and which are missing
	/// and notes the locations of the various boxes 
	/// \param(p) - Pointer to input buffer, mdat box
	/// \param(offset) - offset to moov box within buffer
	/// \param(total_bytes) - length of buffer
	/// \return - status of function
	///
	bool parse_moov(const unsigned char *p, size_t offset, size_t total_bytes);	

	/// \brief This function parses the mdat box identifies the embedded audio codec 
	/// type and the embedded video codec type.
	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - offset to moov box within buffer
	/// \param(codec_type) - vector of codec ids
	/// \return - status of function
	///	
	bool identify_codecs(const unsigned char *s, size_t l, std::vector<CODEC_TYPE> &codec_type);

	/// \brief Within the mdat box, it calculates the offsets to the next media sample
	/// from a given offset. Calls codec specific functions to identify signatures.
	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - offset to moov box within buffer
	/// \param(ct) - codec id
	/// \param(output_offset) - offset to signature.
	/// \return - status of function
	///	
	bool get_offset(const unsigned char *s, size_t l, size_t input_offset, CODEC_TYPE ct, size_t &output_offset);
	/// Calculate offset to the next AVC signature
	bool get_avc_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset);
	/// Calculate offset to the next MP4V signature
	bool get_mp4v_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset);	
	/// Calculate offset to the next MP4A signature
	bool get_mp4a_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset);	
	/// Calculate offset to the next S263 signature
	bool get_s263_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset);	
	/// Calculate offset to the next MJPG signature
	bool get_mjpg_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset);	
	/// Calculate offset to the next AMR signature
	bool get_amr_offset(const unsigned char *p, size_t l, size_t input_offset, size_t &output_offset);	

	/// \brief parse tkhd box for the given track. This holds the "duration" for the stream
	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - offset to moov box within buffer
	/// \param(trak_id) - Track id 
	/// \return - status of function
	///
	bool parse_tkhd(const unsigned char *s, size_t l, unsigned int trak_id);

	/// \brief parse mdhd box for the given track. This holds the "height" & "width" parameters
	/// for the stream
	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - offset to moov box within buffer
	/// \param(trak_id) - Track id 
	/// \return - status of function
	///
	bool parse_mdhd(const unsigned char *s, size_t l, unsigned int trak_id);

	/// \brief Write the newly reconstructed moov box to the output file. This function 
	/// merges all the individually reconstructed parameters into a single "moov" box
	/// \param(s) - Pointer to input buffer, mdat box
	/// \param(l) - offset to moov box within buffer
	/// \param(stco_table) - Reconstructed stco table
	/// \param(stsz_table) - Reconstructed stsz table
	/// \return - status of function
	///
	bool write_moov(const unsigned char *s, size_t total_bytes,  std::vector<size_t> &stco_table,  std::vector<size_t> &stsz_table);

	/// The reconstructed moov box has to be written to file as "big endian" 32-bit values. This function handles that operation.
	static bool write_big_endian_int32(unsigned int i, unsigned char* s, size_t offset, size_t l){
		if(offset + 4 > l) // sizeof(unsigned int) = 4
			return false;
		s[offset] = (i & 0xff000000) >> 24;
		s[offset + 1] = (i & 0x00ff0000) >> 16;
		s[offset + 2] = (i & 0x0000ff00) >> 8;
		s[offset + 3] = (i & 0x000000ff);
		return true;
	}

	/// helper function for reconstruct_stsd() & write_moov()
	size_t write_stsd(size_t index);

	/// resets all the values to for a given "trak" box.
	void reset(trak_box &tk);

	/// Instances of the "moov", "trak", "mdat" and "stsd" box
	moov_box _moov;
	trak_box _tk;
	mdat_box _mdat;
	isomf::sample_description_box* _stsd;

	/// Output file buffer, size and file name.
	unsigned char* o;
	size_t output_file_size;	 
	std::string _outfile;
};
#endif
