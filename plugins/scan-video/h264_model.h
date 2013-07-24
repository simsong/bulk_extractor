#ifndef H264_MODEL
#define H264_MODEL

#include "bitstream_reader.h"
#include "bitstream_writer.h"
#include "isomf_base.h"

using namespace std;

namespace isomf
{

class json_serializer 
{
  public:
    json_serializer(stringstream& s, const string& type, unsigned ind);
    ~json_serializer();

    json_serializer& start_json();
    json_serializer& write_field(const string& f, int value);
    json_serializer& write_field(const string& f, bool value);
    json_serializer& write_field(const string& f, const string& value);
    json_serializer& write_object(const string& f, const string& obj);
    json_serializer& write_array(const string& f, const int* array, unsigned length); 
    json_serializer& write_array(const string& f, const bool* array, unsigned length); 
    json_serializer& end_json();

    string str();

  protected:
    void indent();

    const string _s_type;
    unsigned _s_indent;
    stringstream* _s_stream;
};

/// Base class for parameter sets
class parameter_set
{
  public:
    static const size_t MAX_SIZE = 1024;

    parameter_set();
    parameter_set(const parameter_set& s);
    virtual ~parameter_set();

    parameter_set& operator=(const parameter_set& s);

    virtual size_t size();

    virtual size_t parse(const char* buf, size_t size);
    virtual size_t write(char* buf, size_t size) const;

  protected:

};

/// Chroma format
class chroma_format
{
  public:
    static const chroma_format MONOCHROME;
    static const chroma_format YUV_420;
    static const chroma_format YUV_422;
    static const chroma_format YUV_444;

    chroma_format(int id, int sub_width, int sub_height);
    ~chroma_format() {};

    static const chroma_format* from_id(int id);

    bool operator==(const chroma_format& cf) const
    { return cf.get_id() == _id; };

    int get_id() const { return _id; }
    int get_sub_width() const { return _sub_width; }
    int get_sub_height() const { return _sub_height; }

    string str(unsigned indent = 0) const;

  private:
    int _id;
    int _sub_width;
    int _sub_height;
};

class bitstream_restriction
{
  public:
    bool motion_vectors_over_pic_boundaries_flag;
    int max_bytes_per_pic_denom;
    int max_bits_per_mb_denom;
    int log2_max_mv_length_horizontal;
    int log2_max_mv_length_vertical;
    int num_reorder_frames;
    int max_dec_frame_buffering;

    size_t parse(bitstream_reader& reader);
    size_t write(bitstream_writer& writer) const;
    string str(unsigned indent=0) const;
};

class HRD_parameters
{
  public:
    int cpb_cnt_minus1;
    int bit_rate_scale;
    int cpb_size_scale;
    int* bit_rate_value_minus1;
    int* cpb_size_value_minus1;
    bool* cbr_flag;
    int initial_cpb_removal_delay_length_minus1;
    int cpb_removal_delay_length_minus1;
    int dpb_output_delay_length_minus1;
    int time_offset_length;

    size_t parse(bitstream_reader& reader);
    size_t write(bitstream_writer& writer) const;
    string str(unsigned indent = 0) const;
};

class aspect_ratio
{
  public:
    static aspect_ratio EXTENDED_SAR;

    aspect_ratio():_value(0) {};

    aspect_ratio(int value):_value(value) {};
    int get_value() const { return _value; }

    bool operator==(const aspect_ratio& ar)
    { return _value == ar.get_value(); }

    aspect_ratio& operator=(const aspect_ratio& ar)
    { _value = ar.get_value(); return *this; }

    size_t parse(bitstream_reader& reader);
    size_t write(bitstream_writer& writer) const;
    string str(unsigned indent = 0) const;
  private:
    int _value;
};

class scaling_list
{
  public:
    int* scalinglist;
    int list_length;
    bool use_default_scaling_matrix_flag;

    static scaling_list* read(bitstream_reader& reader, int list_size);
    size_t write(bitstream_writer& writer) const;

    string str(unsigned indent = 0) const;
};

class scaling_matrix
{
  public:
    scaling_list** scaling_list_4x4;
    scaling_list** scaling_list_8x8;

    size_t parse(bitstream_reader& reader);
    size_t write(bitstream_writer& writer) const;
    string str(unsigned indent = 0) const;
};

/// VUI parameters
class VUI_parameters 
{
  public:
    bool aspect_ratio_info_present_flag;
    int sar_width;
    int sar_height;
    bool overscan_info_present_flag;
    bool overscan_appropriate_flag;
    bool video_signal_type_present_flag;
    int video_format;
    bool video_full_range_flag;
    bool colour_description_present_flag;
    int colour_primaries;
    int transfer_characteristics;
    int matrix_coefficients;
    bool chroma_loc_info_present_flag;
    int chroma_sample_loc_type_top_field;
    int chroma_sample_loc_type_bottom_field;
    bool timing_info_present_flag;
    int num_units_in_tick;
    int time_scale;
    bool fixed_frame_rate_flag;
    bool low_delay_hrd_flag;
    bool pic_struct_present_flag;
    HRD_parameters* nal_HRD_params;
    HRD_parameters* vcl_HRD_params;
    bitstream_restriction* bitstreamrestriction;
    aspect_ratio* aspectratio;

    size_t parse(bitstream_reader& reader);
    size_t write(bitstream_writer& writer) const;
    string str(unsigned indent = 0) const;
};

/// Sequence Parameter Set
class sequence_parameter_set : public parameter_set
{
  public:
    sequence_parameter_set();
    virtual ~sequence_parameter_set() {};

    virtual size_t parse(const char* buf, size_t sz);
    virtual size_t write(char* buf, size_t sz) const; 

    int pic_order_cnt_type;
    bool field_pic_flag;
    bool delta_pic_order_always_zero_flag;
    bool entropy_coding_mode_flag;
    bool mb_adaptive_frame_field_flag;
    bool direct_8x8_inference_flag;
    const chroma_format* chroma_format_idc;
    int log2_max_frame_num_minus4;
    int log2_max_pic_order_cnt_lsb_minus4;
    int pic_height_in_map_units_minus1;
    int pic_width_in_mbs_minus1;
    int bit_depth_luma_minus8;
    int bit_depth_chroma_minus8;
    bool qpprime_y_zero_transform_bypass_flag;
    int profile_idc;
    bool constraint_set_0_flag;
    bool constraint_set_1_flag;
    bool constraint_set_2_flag;
    bool constraint_set_3_flag;
    int level_idc;
    int seq_parameter_set_id;
    bool residual_color_transform_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int num_ref_frames;
    bool gaps_in_frame_num_value_allowed_flag;
    bool frame_mbs_only_flag;
    bool frame_cropping_flag;
    int frame_crop_left_offset;
    int frame_crop_right_offset;
    int frame_crop_top_offset;
    int frame_crop_bottom_offset;
    int* offset_for_ref_frame;
  
    VUI_parameters* vui_params;
    scaling_matrix* scalingmatrix;
    int num_ref_frames_in_pic_order_cnt_cycle;

    int reserved;

    string str(unsigned indent = 0) const;

  protected:

  private:
    typedef parameter_set super;
};

class pps_ext
{
  public:
    bool transform_8x8_mode_flag;
    scaling_matrix* scalingmatrix;
    int second_chroma_qp_index_offset;
    bool* pic_scaling_list_present_flag;

    std::string str(unsigned indent = 0) const;
};

/// Picture Parameter Set
class picture_parameter_set : public parameter_set
{
  public:
    bool entropy_coding_mode_flag;
    int num_ref_idx_l0_active_minus1;
    int num_ref_idx_l1_active_minus1;
    int slice_group_change_rate_minus1;
    int pic_parameter_set_id;
    int seq_parameter_set_id;
    bool pic_order_present_flag;
    int num_slice_groups_minus1;
    int slice_group_map_type;
    bool weighted_pred_flag;
    int weighted_bipred_idc;
    int pic_init_qp_minus26;
    int pic_init_qs_minus26;
    int chroma_qp_index_offset;
    bool deblocking_filter_control_present_flag;
    bool constrained_intra_pred_flag;
    bool redundant_pic_cnt_present_flag;
    int* top_left;
    int* bottom_right;
    int* run_length_minus1;
    bool slice_group_change_direction_flag;
    int pic_size_map_units_minus1;
    int* slice_group_id;
    pps_ext* extended;

    picture_parameter_set();
    virtual ~picture_parameter_set() {};

    virtual size_t parse(const char* buf, size_t sz); 
    virtual size_t write(char* buf, size_t sz) const;

    std::string str(unsigned indent = 0) const;
  protected:

  private:
    typedef parameter_set super;
};


}; // namespace isomf

#endif
