#include <iostream>
#include <sstream>
#include <stdio.h>

#include "isomf.h"
#include "h264_model.h"

using namespace std;

namespace isomf
{

// class json_serializer

json_serializer::json_serializer(stringstream& s, const string& type, unsigned ind)
  :_s_type(type),_s_indent(ind),_s_stream(&s)
{
}

json_serializer::~json_serializer()
{
}

void json_serializer::indent()
{
  if (_s_stream) {
    for (unsigned i = 0; i < _s_indent; i++) {
      (*_s_stream) << "  ";
    }
  }
}

json_serializer& json_serializer::start_json()
{
  indent();
  (*_s_stream) << "{" << endl;
  return *this;
}

json_serializer& json_serializer::write_field(const string& name, int value)
{
  indent();
  (*_s_stream) << "  \"" << name << "\":" << value << "," << endl;
  return *this;
}

json_serializer& json_serializer::write_array(const string& name, const int* array, unsigned length)
{
  if (array == NULL) { return *this; }

  indent();
  (*_s_stream) << "  \"" << name << "\":" << "[ ";
  for (unsigned i=0; i<length; i++) {
    (*_s_stream) << array[i];
    if (i != length-1) {
      (*_s_stream) << ", ";
    }
  }
  (*_s_stream) << " ]," << endl;
  return *this;
}

json_serializer& json_serializer::write_array(const string& name, const bool* array, unsigned length)
{
  if (array == NULL) { return *this; }

  indent();
  (*_s_stream) << "  \"" << name << "\":" << "[ ";
  for (unsigned i=0; i<length; i++) {
    (*_s_stream) << (array[i] ? "true" : "false");
    if (i != length-1) {
      (*_s_stream) << ", ";
    }
  }
  (*_s_stream) << " ]," << endl;
  return *this;
}

json_serializer& json_serializer::write_field(const string& name, bool value)
{
  indent();
  (*_s_stream) << "  \"" << name << "\":" 
               << (value ? "true" : "false") << "," << endl;
  return *this;
}

json_serializer& json_serializer::write_field(const string& name, const string& value)
{
  indent();
  (*_s_stream) << "  \"" << name << "\":\"" << value << "\"," << endl;
  return *this;
}

json_serializer& json_serializer::write_object(const string& name, const string& obj)
{
  indent();
  (*_s_stream) << "  \"" << name << "\":" << endl << obj << "," << endl;
  return *this;
}

json_serializer& json_serializer::end_json()
{
  indent();
  (*_s_stream) << "  \"type\":\"" << _s_type << "\"" << endl;
  indent();
  (*_s_stream) << "}";
  return *this;
}

string json_serializer::str()
{
  return _s_stream->str();
}

// class parameter_set

parameter_set::parameter_set()
{
}

parameter_set::~parameter_set()
{
}

parameter_set::parameter_set(const parameter_set& s)
{
  operator=(s);
}

parameter_set& parameter_set::operator=(const parameter_set& s)
{
  return *this;
}

size_t parameter_set::parse(const char* buf, size_t sz) 
{ 
  return 0; 
}

size_t parameter_set::write(char* buf, size_t sz) const 
{ 
  return 0;
}

size_t parameter_set::size()
{
  char buf[MAX_SIZE];
  size_t nb = this->write(buf, MAX_SIZE);
  //cerr << " >> parameter set written " << nb << " bytes.\n";
  return nb;
}

// class chroma_format

const chroma_format chroma_format::MONOCHROME(0,0,0);
const chroma_format chroma_format::YUV_420(1,2,2);
const chroma_format chroma_format::YUV_422(2,2,1);
const chroma_format chroma_format::YUV_444(3,1,1);

chroma_format::chroma_format(int id, int sub_width, int sub_height)
  :_id(id),_sub_width(sub_width),_sub_height(sub_height)
{
}

const chroma_format* chroma_format::from_id(int id)
{
  if (id == MONOCHROME.get_id()) {
    return &MONOCHROME;
  } else if (id == YUV_420.get_id()) {
    return &YUV_420;
  } else if (id == YUV_422.get_id()) {
    return &YUV_422;
  } else if (id == YUV_444.get_id()) {
    return &YUV_444;
  }
  return NULL;
}

string chroma_format::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "chroma_format", indent);
  s.start_json();
  s.write_field("id", _id);
  s.write_field("sub_width", _sub_width);
  s.write_field("sub_height", _sub_height);
  s.end_json();
  return s.str();
}

// class VUI_parameters

size_t VUI_parameters::parse(bitstream_reader& reader)
{
  size_t start = reader.read_bit_count();

  //cerr << "VUI parameter starts at offset: " << start << endl;

  aspect_ratio_info_present_flag = reader.read_bool();
  if (aspect_ratio_info_present_flag) {
    aspectratio = new aspect_ratio();
    aspectratio->parse(reader);

    if ((*aspectratio) == aspect_ratio::EXTENDED_SAR) {
      sar_width = reader.read_bits(16);
      sar_height = reader.read_bits(16);
    }
  }

  //cerr << "Position after SAR: "<<reader.read_bit_count()<<endl;

  overscan_info_present_flag = reader.read_bool();
  if (overscan_info_present_flag) {
    overscan_appropriate_flag = reader.read_bool();
  }

  video_signal_type_present_flag = reader.read_bool();
  if (video_signal_type_present_flag) {
    video_format = reader.read_bits(3);
    video_full_range_flag = reader.read_bool();

    colour_description_present_flag = reader.read_bool();
    if (colour_description_present_flag) {
      colour_primaries = reader.read_bits(8);
      transfer_characteristics = reader.read_bits(8);
      matrix_coefficients = reader.read_bits(8);
    }
  }

  //cerr << "Position after video format: "<<reader.read_bit_count()<<endl;

  chroma_loc_info_present_flag = reader.read_bool();
  if (chroma_loc_info_present_flag) {
    chroma_sample_loc_type_top_field = reader.read_UE();
    chroma_sample_loc_type_bottom_field = reader.read_UE();
  }

  //cerr << "Position after chroma: "<<reader.read_bit_count()<<endl;

  timing_info_present_flag = reader.read_bool();
  if (timing_info_present_flag) {
    num_units_in_tick = reader.read_bits(32);
    time_scale = reader.read_bits(32);
    fixed_frame_rate_flag = reader.read_bool();
  }

  //cerr << "Position after timing: "<<reader.read_bit_count()<<endl;

  bool nal_hrd_parameters_present_flag = reader.read_bool();
  if (nal_hrd_parameters_present_flag) {
    nal_HRD_params = new HRD_parameters();
    nal_HRD_params->parse(reader);
  }

  //cerr << "Position after nal HRD: "<<reader.read_bit_count()<<endl;

  bool vcl_hrd_parameters_present_flag = reader.read_bool();
  if (vcl_hrd_parameters_present_flag) {
    vcl_HRD_params = new HRD_parameters();
    vcl_HRD_params->parse(reader);
  }

  //cerr << "Position after vcl HRD: "<<reader.read_bit_count()<<endl;

  if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
    low_delay_hrd_flag = reader.read_bool();
  }
  pic_struct_present_flag = reader.read_bool();

  //cerr << "Position before bitstream restriction: "<<reader.read_bit_count()<<endl;
  bool bitstream_restriction_flag = reader.read_bool();
  if (bitstream_restriction_flag) {
    bitstreamrestriction = new bitstream_restriction();
    bitstreamrestriction->parse(reader);
  }

  //cerr << "VUI ends at: "<<reader.read_bit_count()<<endl;
  return reader.read_bit_count() - start;
}

size_t VUI_parameters::write(bitstream_writer& writer) const
{
  size_t start = writer.write_bit_count();

  //cerr << "VUI parameter starts at offset: " << start <<endl;

  writer.write_bool(aspect_ratio_info_present_flag);
  if (aspect_ratio_info_present_flag) {
    aspectratio->write(writer);
    if ((*aspectratio) == aspect_ratio::EXTENDED_SAR) {
      writer.write_bits(sar_width, 16);
      writer.write_bits(sar_height, 16);
    }
  }
  //cerr << "Position after SAR: "<<writer.write_bit_count()<<endl;

  writer.write_bool(overscan_info_present_flag);
  if (overscan_info_present_flag) {
    writer.write_bool(overscan_appropriate_flag);
  }
  writer.write_bool(video_signal_type_present_flag);
  if (video_signal_type_present_flag) {
    writer.write_bits(video_format, 3);
    writer.write_bool(video_full_range_flag);
    writer.write_bool(colour_description_present_flag);
    if (colour_description_present_flag) {
      writer.write_bits(colour_primaries, 8);
      writer.write_bits(transfer_characteristics, 8);
      writer.write_bits(matrix_coefficients, 8);
    }
  }
  //cerr << "Position after video format: "<<writer.write_bit_count()<<endl;

  writer.write_bool(chroma_loc_info_present_flag);
  if (chroma_loc_info_present_flag) {
    writer.write_UE(chroma_sample_loc_type_top_field);
    writer.write_UE(chroma_sample_loc_type_bottom_field);
  }
  //cerr << "Position after chroma: "<<writer.write_bit_count()<<endl;

  writer.write_bool(timing_info_present_flag);
  if (timing_info_present_flag) {
    writer.write_bits(num_units_in_tick, 32);
    writer.write_bits(time_scale, 32);
    writer.write_bool(fixed_frame_rate_flag);
  }
  //cerr << "Position after timing: "<<writer.write_bit_count()<<endl;

  if (nal_HRD_params) {
    writer.write_bool(true);
    nal_HRD_params->write(writer);
  } else {
    writer.write_bool(false);
  }
  //cerr << "Position after nal HRD: "<<writer.write_bit_count()<<endl;

  if (vcl_HRD_params) {
    writer.write_bool(true);
    vcl_HRD_params->write(writer);
  } else {
    writer.write_bool(false);
  }
  //cerr << "Position after vcl HRD: "<<writer.write_bit_count()<<endl;

  if (nal_HRD_params || vcl_HRD_params) {
    writer.write_bool(low_delay_hrd_flag);
  }
  writer.write_bool(pic_struct_present_flag);

  //cerr << "Position before bitstream restriction: "<<writer.write_bit_count()<<endl;

  if (bitstreamrestriction) {
    writer.write_bool(true);
    bitstreamrestriction->write(writer);
  } else {
    writer.write_bool(false);
  }

  //cerr << "VUI ends at: "<<writer.write_bit_count()<<endl;
  return writer.write_bit_count() - start;
}

string VUI_parameters::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "VUI_parameters", indent);
  s.start_json();

  s.write_field("aspect_ratio_info_present_flag", aspect_ratio_info_present_flag);
  s.write_field("sar_width", sar_width);
  s.write_field("sar_height", sar_height);
  s.write_field("overscan_info_present_flag", overscan_info_present_flag);
  s.write_field("overscan_appropriate_flag", overscan_appropriate_flag);
  s.write_field("video_signal_type_present_flag", video_signal_type_present_flag);
  s.write_field("video_format", video_format);
  s.write_field("video_full_range_flag", video_full_range_flag);
  s.write_field("colour_description_present_flag", colour_description_present_flag);
  s.write_field("colour_primaries", colour_primaries);
  s.write_field("transfer_characteristics", transfer_characteristics);
  s.write_field("matrix_coefficients", matrix_coefficients);
  s.write_field("chroma_loc_info_present_flag", chroma_loc_info_present_flag);
  s.write_field("chroma_sample_loc_type_top_field", chroma_sample_loc_type_top_field);
  s.write_field("chroma_sample_loc_type_bottom_field", chroma_sample_loc_type_bottom_field);
  s.write_field("timing_info_present_flag", timing_info_present_flag);
  s.write_field("num_units_in_tick", num_units_in_tick);
  s.write_field("time_scale", time_scale);
  s.write_field("fixed_frame_rate_flag", fixed_frame_rate_flag);
  s.write_field("low_delay_hrd_flag", low_delay_hrd_flag);
  s.write_field("pic_struct_present_flag", pic_struct_present_flag);

  if (nal_HRD_params) {
    s.write_object("nal_HRD_params", nal_HRD_params->str(indent+1));
  }
  if (vcl_HRD_params) {
    s.write_object("vcl_HRD_params", vcl_HRD_params->str(indent+1));
  }
  if (bitstreamrestriction) {
    s.write_object("bitstreamrestriction", bitstreamrestriction->str(indent+1));
  }
  if (aspectratio) {
    s.write_object("aspectratio", aspectratio->str(indent+1));
  }

  s.end_json();
  return s.str();
}

// class HRD_parameters

size_t HRD_parameters::parse(bitstream_reader& reader)
{
  size_t start = reader.read_bit_count();

  //cerr << "HRD parameter starts at: " << start << endl;

  cpb_cnt_minus1 = reader.read_UE();
  //cerr << "Position after cpb_cnt_minus1: "<<reader.read_bit_count()<<endl;

  bit_rate_scale = reader.read_bits(4);
  //cerr << "Position after bit_rate_scale: "<<reader.read_bit_count()<<endl;
  cpb_size_scale = reader.read_bits(4);
  //cerr << "Position after cpb_size_scale: "<<reader.read_bit_count()<<endl;
  bit_rate_value_minus1 = new int[cpb_cnt_minus1 + 1];
  cpb_size_value_minus1 = new int[cpb_cnt_minus1 + 1];
  cbr_flag = new bool[cpb_cnt_minus1 + 1];

  for (int i=0; i<=cpb_cnt_minus1; i++) {
    bit_rate_value_minus1[i] = reader.read_UE();
    cpb_size_value_minus1[i] = reader.read_UE();
    cbr_flag[i] = reader.read_bool();
  }

  //cerr << "Position after arrays: "<<reader.read_bit_count()<<endl;

  initial_cpb_removal_delay_length_minus1 = reader.read_bits(5);
  cpb_removal_delay_length_minus1 = reader.read_bits(5);
  dpb_output_delay_length_minus1 = reader.read_bits(5);
  time_offset_length = reader.read_bits(5);

  size_t end = reader.read_bit_count();
  //cerr << "HRD parameter ends at: " << end << endl;

  return end - start;
}

size_t HRD_parameters::write(bitstream_writer& writer) const
{
  size_t start = writer.write_bit_count();

  //cerr << "HRD parameter starts at: " << start << endl;

  writer.write_UE(cpb_cnt_minus1);
  //cerr << "Position after cpb_cnt_minus1: "<<writer.write_bit_count()<<endl;

  writer.write_bits(bit_rate_scale, 4);
  //cerr << "Position after bit_rate_scale: "<<writer.write_bit_count()<<endl;
  writer.write_bits(cpb_size_scale, 4);
  //cerr << "Position after cpb_size_scale: "<<writer.write_bit_count()<<endl;

  for (int SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++) {
    writer.write_UE(bit_rate_value_minus1[SchedSelIdx]);
    writer.write_UE(cpb_size_value_minus1[SchedSelIdx]);
    writer.write_bool(cbr_flag[SchedSelIdx]);
  }
  //cerr << "Position after arrays: "<<writer.write_bit_count()<<endl;

  writer.write_bits(initial_cpb_removal_delay_length_minus1, 5);
  writer.write_bits(cpb_removal_delay_length_minus1, 5);
  writer.write_bits(dpb_output_delay_length_minus1, 5);
  writer.write_bits(time_offset_length, 5);

  return writer.write_bit_count() - start;
}

string HRD_parameters::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "HRD_parameters", indent);
  s.start_json();

  s.write_field("cpb_cnt_minus1", cpb_cnt_minus1);
  s.write_field("bit_rate_scale", bit_rate_scale);
  s.write_field("cpb_size_scale", cpb_size_scale);
  s.write_array("bit_rate_value_minus1", bit_rate_value_minus1, cpb_cnt_minus1+1);
  s.write_array("cpb_size_value_minus1", cpb_size_value_minus1, cpb_cnt_minus1+1);
  s.write_array("cbr_flag", cbr_flag, cpb_cnt_minus1+1);
  s.write_field("initial_cpb_removal_delay_length_minus1", initial_cpb_removal_delay_length_minus1);
  s.write_field("cpb_removal_delay_length_minus1", cpb_removal_delay_length_minus1);
  s.write_field("dpb_output_delay_length_minus1", dpb_output_delay_length_minus1);
  s.write_field("time_offset_length", time_offset_length);

  s.end_json();
  return s.str();
}

// class bitstream_restriction

size_t bitstream_restriction::parse(bitstream_reader& reader)
{
  size_t start = reader.read_bit_count();

  motion_vectors_over_pic_boundaries_flag = reader.read_bool();
  max_bytes_per_pic_denom = reader.read_UE();
  max_bits_per_mb_denom = reader.read_UE();
  log2_max_mv_length_horizontal = reader.read_UE();
  log2_max_mv_length_vertical = reader.read_UE();
  num_reorder_frames = reader.read_UE();
  max_dec_frame_buffering = reader.read_UE();

  return reader.read_bit_count() - start;
}

size_t bitstream_restriction::write(bitstream_writer& writer) const
{
  size_t start = writer.write_bit_count();
  writer.write_bool(motion_vectors_over_pic_boundaries_flag);
  writer.write_UE(max_bytes_per_pic_denom);
  writer.write_UE(max_bits_per_mb_denom);
  writer.write_UE(log2_max_mv_length_horizontal);
  writer.write_UE(log2_max_mv_length_vertical);
  writer.write_UE(num_reorder_frames);
  writer.write_UE(max_dec_frame_buffering);
  return writer.write_bit_count() - start;
}

string bitstream_restriction::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "bitstream_restriction", indent);
  s.start_json();

  s.write_field("motion_vectors_over_pic_boundaries_flag", motion_vectors_over_pic_boundaries_flag);
  s.write_field("max_bytes_per_pic_denom", max_bytes_per_pic_denom);
  s.write_field("max_bits_per_mb_denom", max_bits_per_mb_denom);
  s.write_field("log2_max_mv_length_horizontal", log2_max_mv_length_horizontal);
  s.write_field("log2_max_mv_length_vertical", log2_max_mv_length_vertical);
  s.write_field("num_reorder_frames", num_reorder_frames);
  s.write_field("max_dec_frame_buffering", max_dec_frame_buffering);

  s.end_json();
  return s.str();
}

// class sequence_parameter_set

sequence_parameter_set::sequence_parameter_set()
  : pic_order_cnt_type(0),
    field_pic_flag(false),
    delta_pic_order_always_zero_flag(false),
    entropy_coding_mode_flag(false),
    mb_adaptive_frame_field_flag(false),
    direct_8x8_inference_flag(false),
    chroma_format_idc(NULL),
    log2_max_frame_num_minus4(0),
    log2_max_pic_order_cnt_lsb_minus4(0),
    pic_height_in_map_units_minus1(0),
    pic_width_in_mbs_minus1(0),
    bit_depth_luma_minus8(0),
    bit_depth_chroma_minus8(0),
    qpprime_y_zero_transform_bypass_flag(false),
    profile_idc(0),
    constraint_set_0_flag(false),
    constraint_set_1_flag(false),
    constraint_set_2_flag(false),
    constraint_set_3_flag(false),
    level_idc(0),
    seq_parameter_set_id(0),
    residual_color_transform_flag(false),
    offset_for_non_ref_pic(0),
    offset_for_top_to_bottom_field(0),
    num_ref_frames(0),
    gaps_in_frame_num_value_allowed_flag(false),
    frame_mbs_only_flag(false),
    frame_cropping_flag(false),
    frame_crop_left_offset(0),
    frame_crop_right_offset(0),
    frame_crop_top_offset(0),
    frame_crop_bottom_offset(0),
    offset_for_ref_frame(0),
    vui_params(NULL),
    scalingmatrix(NULL),
    num_ref_frames_in_pic_order_cnt_cycle(0),
    reserved(0)
{
}

string sequence_parameter_set::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "sequence_parameter_set", indent);
  s.start_json();

  s.write_field("pic_order_cnt_type", pic_order_cnt_type);
  s.write_field("field_pic_flag", field_pic_flag);
  s.write_field("delta_pic_order_always_zero_flag", delta_pic_order_always_zero_flag);
  s.write_field("entropy_coding_mode_flag", entropy_coding_mode_flag);
  s.write_field("mb_adaptive_frame_field_flag", mb_adaptive_frame_field_flag);
  s.write_field("direct_8x8_inference_flag", direct_8x8_inference_flag);

  if (chroma_format_idc) {
    s.write_object("chroma_format_idc", chroma_format_idc->str(indent+1));
  }

  s.write_field("log2_max_frame_num_minus4", log2_max_frame_num_minus4);
  s.write_field("log2_max_pic_order_cnt_lsb_minus4", log2_max_pic_order_cnt_lsb_minus4);
  s.write_field("pic_height_in_map_units_minus1", pic_height_in_map_units_minus1);
  s.write_field("pic_width_in_mbs_minus1", pic_width_in_mbs_minus1);
  s.write_field("bit_depth_luma_minus8", bit_depth_luma_minus8);
  s.write_field("bit_depth_chroma_minus8", bit_depth_chroma_minus8);
  s.write_field("qpprime_y_zero_transform_bypass_flag", qpprime_y_zero_transform_bypass_flag);
  s.write_field("profile_idc", profile_idc);
  s.write_field("constraint_set_0_flag", constraint_set_0_flag);
  s.write_field("constraint_set_1_flag", constraint_set_1_flag);
  s.write_field("constraint_set_2_flag", constraint_set_2_flag);
  s.write_field("constraint_set_3_flag", constraint_set_3_flag);
  s.write_field("level_idc", level_idc);
  s.write_field("seq_parameter_set_id", seq_parameter_set_id);
  s.write_field("residual_color_transform_flag", residual_color_transform_flag);
  s.write_field("offset_for_non_ref_pic", offset_for_non_ref_pic);
  s.write_field("offset_for_top_to_bottom_field", offset_for_top_to_bottom_field);
  s.write_field("num_ref_frames", num_ref_frames);
  s.write_field("gaps_in_frame_num_value_allowed_flag", gaps_in_frame_num_value_allowed_flag);
  s.write_field("frame_mbs_only_flag", frame_mbs_only_flag);
  s.write_field("frame_cropping_flag", frame_cropping_flag);
  s.write_field("frame_crop_left_offset", frame_crop_left_offset);
  s.write_field("frame_crop_right_offset", frame_crop_right_offset);
  s.write_field("frame_crop_top_offset", frame_crop_top_offset);
  s.write_field("frame_crop_bottom_offset", frame_crop_bottom_offset);
  s.write_array("offset_for_ref_frame", offset_for_ref_frame, 
      num_ref_frames_in_pic_order_cnt_cycle);

  if (vui_params) {
    s.write_object("vui_params", vui_params->str(indent+1));
  }

  if (scalingmatrix) {
    s.write_object("scalingmatrix", scalingmatrix->str(indent+1));
  }

  s.write_field("num_ref_frames_in_pic_order_cnt_cycle", num_ref_frames_in_pic_order_cnt_cycle);
  s.write_field("reserved", reserved);
  s.end_json();
  return s.str();
}

size_t sequence_parameter_set::parse(const char* buf, size_t sz)
{
  bitstream_reader r(buf, sz);

  profile_idc = r.read_bits(8);
  constraint_set_0_flag = r.read_bool();
  constraint_set_1_flag = r.read_bool();
  constraint_set_2_flag = r.read_bool();
  constraint_set_3_flag = r.read_bool();
  reserved = r.read_bits(4);

  level_idc = r.read_bits(8);
  seq_parameter_set_id = r.read_UE();

  if (profile_idc == 100 || profile_idc == 110 || 
      profile_idc == 122 || profile_idc == 144) 
  {
    chroma_format_idc = chroma_format::from_id(r.read_UE());
    if (chroma_format_idc == &chroma_format::YUV_444) {
      residual_color_transform_flag = r.read_bool();
    }

    bit_depth_luma_minus8 = r.read_UE();
    bit_depth_chroma_minus8 = r.read_UE();
    qpprime_y_zero_transform_bypass_flag = r.read_bool();

    bool seq_scaling_matrix_present = r.read_bool();
    if (seq_scaling_matrix_present) {
      scalingmatrix = new scaling_matrix();
      scalingmatrix->parse(r);
    }
  } else {
    chroma_format_idc = &chroma_format::YUV_420;
  }

  log2_max_frame_num_minus4 = r.read_UE();
  pic_order_cnt_type = r.read_UE();

  if (pic_order_cnt_type == 0) {
    log2_max_pic_order_cnt_lsb_minus4 = r.read_UE();
  } else if (pic_order_cnt_type == 1) {
    delta_pic_order_always_zero_flag = r.read_bool();
    offset_for_non_ref_pic = r.read_SE();
    offset_for_top_to_bottom_field = r.read_SE();
    num_ref_frames_in_pic_order_cnt_cycle = r.read_UE();
    offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
    for (int i=0; i<num_ref_frames_in_pic_order_cnt_cycle; i++) {
      offset_for_ref_frame[i] = r.read_SE();
    }
  }

  num_ref_frames = r.read_UE();
  gaps_in_frame_num_value_allowed_flag = r.read_bool();
  pic_width_in_mbs_minus1 = r.read_UE();
  pic_height_in_map_units_minus1 = r.read_UE();
  frame_mbs_only_flag = r.read_bool();
  if (! frame_mbs_only_flag) {
    mb_adaptive_frame_field_flag = r.read_bool();
  }

  direct_8x8_inference_flag = r.read_bool();
  frame_cropping_flag = r.read_bool();
  if (frame_cropping_flag) {
    frame_crop_left_offset = r.read_UE();
    frame_crop_right_offset = r.read_UE();
    frame_crop_top_offset = r.read_UE();
    frame_crop_bottom_offset = r.read_UE();
  }

  bool vui_parameters_present_flag = r.read_bool();
  if (vui_parameters_present_flag) {
    vui_params = new VUI_parameters();
    vui_params->parse(r);
  }
  r.read_trailing_bits();

  return r.read_count();
}

size_t sequence_parameter_set::write(char* buf, size_t sz) const
{
  bitstream_writer* writer = new bitstream_writer(buf, sz);

  writer->write_bits(profile_idc, 8);
  writer->write_bool(constraint_set_0_flag);
  writer->write_bool(constraint_set_1_flag);
  writer->write_bool(constraint_set_2_flag);
  writer->write_bool(constraint_set_3_flag);
  writer->write_bits(reserved, 4);

  writer->write_bits(level_idc, 8);
  writer->write_UE(seq_parameter_set_id);

  if (profile_idc == 100 || profile_idc == 110 || 
      profile_idc == 122 || profile_idc == 144) 
  {
    writer->write_UE(chroma_format_idc->get_id());
    if ((*chroma_format_idc) == chroma_format::YUV_444) {
      writer->write_bool(residual_color_transform_flag);
    }
    writer->write_UE(bit_depth_luma_minus8);
    writer->write_UE(bit_depth_chroma_minus8);
    writer->write_bool(qpprime_y_zero_transform_bypass_flag);

    if (scalingmatrix) {
      writer->write_bool(true);
      for (int i = 0; i < 8; i++) {
        if (i < 6) {
          if (scalingmatrix->scaling_list_4x4[i]) {
            writer->write_bool(true);
            scalingmatrix->scaling_list_4x4[i]->write(*writer);
          } else {
            writer->write_bool(false);
          }
        } else {
          if (scalingmatrix->scaling_list_8x8[i - 6]) {
            writer->write_bool(true);
            scalingmatrix->scaling_list_8x8[i - 6]->write(*writer);
          } else {
            writer->write_bool(false);
          }
        }
      }
    } else {
      writer->write_bool(false);
    }
  }
  
  writer->write_UE(log2_max_frame_num_minus4);
  writer->write_UE(pic_order_cnt_type);
  if (pic_order_cnt_type == 0) {
    writer->write_UE(log2_max_pic_order_cnt_lsb_minus4);
  } else if (pic_order_cnt_type == 1) {
    writer->write_bool(delta_pic_order_always_zero_flag);
    writer->write_SE(offset_for_non_ref_pic);
    writer->write_SE(offset_for_top_to_bottom_field);
    // size of offset_for_ref_frame array
    writer->write_UE(num_ref_frames_in_pic_order_cnt_cycle);
    for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
      writer->write_SE(offset_for_ref_frame[i]);
    }
  }
  writer->write_UE(num_ref_frames);
  writer->write_bool(gaps_in_frame_num_value_allowed_flag);
  writer->write_UE(pic_width_in_mbs_minus1);
  writer->write_UE(pic_height_in_map_units_minus1);
  writer->write_bool(frame_mbs_only_flag);
  if (! frame_mbs_only_flag) {
    writer->write_bool(mb_adaptive_frame_field_flag);
  }
  writer->write_bool(direct_8x8_inference_flag);
  writer->write_bool(frame_cropping_flag);
  if (frame_cropping_flag) {
    writer->write_UE(frame_crop_left_offset);
    writer->write_UE(frame_crop_right_offset);
    writer->write_UE(frame_crop_top_offset);
    writer->write_UE(frame_crop_bottom_offset);
  }

  if (vui_params) {
    writer->write_bool(true);
    vui_params->write(*writer);
  } else {
    writer->write_bool(false);
  }
  writer->write_trailing_bits();

  return writer->write_count();
}

// class aspect_ratio
aspect_ratio aspect_ratio::EXTENDED_SAR(255);

size_t aspect_ratio::parse(bitstream_reader& reader)
{
  _value = reader.read_bits(8);
  return 8;
}

size_t aspect_ratio::write(bitstream_writer& writer) const
{
  return writer.write_bits(_value, 8);
}

string aspect_ratio::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "aspect_ratio", indent);
  s.start_json();

  s.write_field("value", _value);

  s.end_json();
  return s.str();
}

// class scaling list

scaling_list* scaling_list::read(bitstream_reader& reader, int list_size)
{
  if (list_size <= 0) {
    return NULL;
  }

  scaling_list* sl = new scaling_list();
  sl->scalinglist = new int[list_size];
  sl->list_length = list_size;
  int last_scale = 8;
  int next_scale = 8;

  for (int j=0; j<list_size; j++) {
    if (next_scale != 0) {
      int delta_scale = reader.read_SE();
      next_scale = (last_scale + delta_scale + 256) % 256;
      sl->use_default_scaling_matrix_flag = (j==0 && next_scale==0);
    }
    sl->scalinglist[j] = next_scale == 0 ? last_scale : next_scale;
    last_scale = sl->scalinglist[j];
  }
  return sl;
}

size_t scaling_list::write(bitstream_writer& writer) const
{
  if (use_default_scaling_matrix_flag) {
    writer.write_SE(0);
    return writer.write_count();
  }

  int lastScale = 8;
  int nextScale = 8;
  for (int j = 0; j < list_length; j++) {
    if (nextScale != 0) {
      int deltaScale = scalinglist[j] - lastScale - 256;
      writer.write_SE(deltaScale);
    }
    lastScale = scalinglist[j];
  }
  return writer.write_count();
}

string scaling_list::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "scaling_list", indent);
  s.start_json();

  s.write_field("use_default_scaling_matrix_flag", use_default_scaling_matrix_flag);
  if (! use_default_scaling_matrix_flag) {
    s.write_array("scalinglist", scalinglist, list_length);
  }

  s.end_json();
  return s.str();
}

// class scaling_matrix

size_t scaling_matrix::parse(bitstream_reader& reader)
{
  size_t start = reader.read_bit_count();

  scaling_list_4x4 = new scaling_list*[6];
  scaling_list_8x8 = new scaling_list*[2];

  for (int i=0; i<6; i++) {
    if (reader.read_bool()) {
      scaling_list_4x4[i] = scaling_list::read(reader, 16);
    } else {
      scaling_list_4x4[i] = NULL;
    }
  }

  for (int i=0; i<2; i++) {
    if (reader.read_bool()) {
      scaling_list_8x8[i] = scaling_list::read(reader, 64);
    } else {
      scaling_list_8x8[i] = NULL;
    }
  }

  return reader.read_bit_count() - start;
}

size_t scaling_matrix::write(bitstream_writer& writer) const
{
  if (! scaling_list_4x4 || ! scaling_list_8x8) { return 0; }

  size_t start = writer.write_bit_count();

  for (int i=0; i<6; i++) {
    if (scaling_list_4x4[i]) {
      writer.write_bool(true);
      scaling_list_4x4[i]->write(writer);
    } else {
      writer.write_bool(false);
    }
  }

  for (int i=0; i<2; i++) {
    if (scaling_list_8x8[i]) {
      writer.write_bool(true);
      scaling_list_8x8[i]->write(writer);
    } else {
      writer.write_bool(false);
    }
  }

  return writer.write_bit_count() - start;
}

string scaling_matrix::str(unsigned indent) const
{
  stringstream stream;
  json_serializer s(stream, "scaling_matrix", indent);
  s.start_json();

  char buf[64];
  if (scaling_list_4x4) {
    for (int i=0; i<6; i++) {
      if (scaling_list_4x4[i]) {
        sprintf(buf, "scaling_list_4x4[%d]", i);
        s.write_object(buf, scaling_list_4x4[i]->str());
      }
    }
  }

  if (scaling_list_8x8) {
    for (int i=0; i<2; i++) {
      if (scaling_list_8x8[i]) {
        sprintf(buf, "scaling_list_8x8[%d]", i);
        s.write_object(buf, scaling_list_8x8[i]->str());
      }
    }
  }

  s.end_json();
  return s.str();
}

// class picture_parameter_set

picture_parameter_set::picture_parameter_set()
  : entropy_coding_mode_flag(false),
    num_ref_idx_l0_active_minus1(0),
    num_ref_idx_l1_active_minus1(0),
    slice_group_change_rate_minus1(0),
    pic_parameter_set_id(0),
    seq_parameter_set_id(0),
    pic_order_present_flag(false),
    num_slice_groups_minus1(0),
    slice_group_map_type(0),
    weighted_pred_flag(false),
    weighted_bipred_idc(0),
    pic_init_qp_minus26(0),
    pic_init_qs_minus26(0),
    chroma_qp_index_offset(0),
    deblocking_filter_control_present_flag(false),
    constrained_intra_pred_flag(false),
    redundant_pic_cnt_present_flag(false),
    top_left(NULL),
    bottom_right(NULL),
    run_length_minus1(NULL),
    slice_group_change_direction_flag(false),
    pic_size_map_units_minus1(0),
    slice_group_id(NULL),
    extended(NULL)
{
}

size_t picture_parameter_set::parse(const char* buf, size_t sz)
{
  bitstream_reader r(buf, sz);

  pic_parameter_set_id = r.read_UE();
  seq_parameter_set_id = r.read_UE();
  entropy_coding_mode_flag = r.read_bool();
  pic_order_present_flag = r.read_bool();
  num_slice_groups_minus1 = r.read_UE();

  if (num_slice_groups_minus1 > 0) {
    slice_group_map_type = r.read_UE();
    top_left = new int[num_slice_groups_minus1 + 1];
    bottom_right  = new int[num_slice_groups_minus1 + 1];
    run_length_minus1  = new int[num_slice_groups_minus1 + 1];

    if (slice_group_map_type == 0) {
      for (int i_group = 0; i_group <= num_slice_groups_minus1; i_group++) {
        run_length_minus1[i_group] = r.read_UE();
      }
    } else if (slice_group_map_type == 2) {
      for (int i_group = 0; i_group < num_slice_groups_minus1; i_group++) {
        top_left[i_group] = r.read_UE();
        bottom_right[i_group] = r.read_UE();
      }
    } else if (slice_group_map_type == 3 ||
               slice_group_map_type == 4 ||
               slice_group_map_type == 5) {
      slice_group_change_direction_flag = r.read_bool();
      slice_group_change_rate_minus1 = r.read_UE();
    } else if (slice_group_map_type == 6) {
      int number_bits_per_slice_group_id = 0;
      if (num_slice_groups_minus1 + 1 > 4) {
        number_bits_per_slice_group_id = 3;
      } else if (num_slice_groups_minus1 + 1 > 2) {
        number_bits_per_slice_group_id = 2;
      } else {
        number_bits_per_slice_group_id = 1;
      }

      pic_size_map_units_minus1 = r.read_UE();
      slice_group_id = new int[pic_size_map_units_minus1 + 1];
      for (int i=0; i<pic_size_map_units_minus1; i++) {
        slice_group_id[i] = r.read_U(number_bits_per_slice_group_id);
      }
    }
  }

  num_ref_idx_l0_active_minus1 = r.read_UE();
  num_ref_idx_l1_active_minus1 = r.read_UE();
  weighted_pred_flag = r.read_bool();
  weighted_bipred_idc = r.read_bits(2);
  pic_init_qp_minus26 = r.read_SE();
  pic_init_qs_minus26 = r.read_SE();
  chroma_qp_index_offset = r.read_SE();

  deblocking_filter_control_present_flag = r.read_bool();
  constrained_intra_pred_flag = r.read_bool();
  redundant_pic_cnt_present_flag = r.read_bool();

  if (r.more_rbsp_data()) {
    extended = new pps_ext();
    extended->transform_8x8_mode_flag = r.read_bool();
    bool pic_scaling_matrix_present_flag = r.read_bool();
    if (pic_scaling_matrix_present_flag) {
      for (int i=0; i<6+2*(extended->transform_8x8_mode_flag ? 1 : 0); i++) {
        bool pic_scalinglist_present_flag = r.read_bool();
        if (pic_scalinglist_present_flag) {
          extended->scalingmatrix->scaling_list_4x4 = new scaling_list*[8];
          extended->scalingmatrix->scaling_list_8x8 = new scaling_list*[8];
          if (i<6) {
            extended->scalingmatrix->scaling_list_4x4[i] = 
              scaling_list::read(r, 16);
          } else {
            extended->scalingmatrix->scaling_list_8x8[i-6] = 
              scaling_list::read(r, 64);
          }
        }
      }
    }
    extended->second_chroma_qp_index_offset = r.read_SE();
  }

  //cerr << "+++ pps read bit count: " << r.read_bit_count() << endl;

  r.read_trailing_bits();

  return r.read_count();
}

size_t picture_parameter_set::write(char* buf, size_t sz) const
{
  bool flag;
  bitstream_writer writer(buf, sz);
  writer.write_UE(pic_parameter_set_id);
  writer.write_UE(seq_parameter_set_id);
  writer.write_bool(entropy_coding_mode_flag);
  writer.write_bool(pic_order_present_flag);
  writer.write_UE(num_slice_groups_minus1);

  if (num_slice_groups_minus1 > 0) {
    writer.write_UE(slice_group_map_type);
    if (slice_group_map_type == 0) {
      for (int iGroup = 0; iGroup <= num_slice_groups_minus1; iGroup++) {
        writer.write_UE(run_length_minus1[iGroup]);
      }
    } else if (slice_group_map_type == 2) {
      for (int iGroup = 0; iGroup < num_slice_groups_minus1; iGroup++) {
        writer.write_UE(top_left[iGroup]);
        writer.write_UE(bottom_right[iGroup]);
      }
    } else if (slice_group_map_type == 3 || slice_group_map_type == 4
        || slice_group_map_type == 5) {
      writer.write_bool(slice_group_change_direction_flag);
      writer.write_UE(slice_group_change_rate_minus1);
    } else if (slice_group_map_type == 6) {
      int NumberBitsPerSliceGroupId;
      if (num_slice_groups_minus1 + 1 > 4)
        NumberBitsPerSliceGroupId = 3;
      else if (num_slice_groups_minus1 + 1 > 2)
        NumberBitsPerSliceGroupId = 2;
      else
        NumberBitsPerSliceGroupId = 1;

      int slice_group_id_length = pic_size_map_units_minus1 + 1;
      writer.write_UE(slice_group_id_length);
      for (int i = 0; i <= slice_group_id_length; i++) {
        writer.write_U(slice_group_id[i], NumberBitsPerSliceGroupId);
      }
    }
  }
  writer.write_UE(num_ref_idx_l0_active_minus1);
  writer.write_UE(num_ref_idx_l1_active_minus1);
  writer.write_bool(weighted_pred_flag);
  writer.write_bits(weighted_bipred_idc, 2);
  writer.write_SE(pic_init_qp_minus26);
  writer.write_SE(pic_init_qs_minus26);
  writer.write_SE(chroma_qp_index_offset);
  writer.write_bool(deblocking_filter_control_present_flag);
  writer.write_bool(constrained_intra_pred_flag);
  writer.write_bool(redundant_pic_cnt_present_flag);

  if (extended) {
    writer.write_bool(extended->transform_8x8_mode_flag);

    flag = extended->scalingmatrix != NULL;
    writer.write_bool(flag);
    if (flag) {
      for (int i=0; i< 6+2*(extended->transform_8x8_mode_flag ? 1 : 0); i++) {
        if (i < 6) {
          flag = extended->scalingmatrix->scaling_list_4x4[i] != NULL;
          writer.write_bool(flag);
          if (flag) {
            extended->scalingmatrix->scaling_list_4x4[i]->write(writer);
          }

        } else {
          flag = extended->scalingmatrix->scaling_list_8x8[i - 6] != NULL;
          writer.write_bool(flag);
          if (flag) {
            extended->scalingmatrix->scaling_list_8x8[i - 6]->write(writer);
          }
        }
      }
    }
    writer.write_SE(extended->second_chroma_qp_index_offset);
  }

  //cerr << "+++ pps write bit count: " << writer.write_bit_count() << endl;

  writer.write_trailing_bits();
  return writer.write_count();  
}

std::string picture_parameter_set::str(unsigned indent) const
{
  std::stringstream stream;
  json_serializer s(stream, "picture_parameter_set", indent);

  s.start_json();

  s.write_field("entropy_coding_mode_flag", entropy_coding_mode_flag);
  s.write_field("num_ref_idx_l0_active_minus1", num_ref_idx_l0_active_minus1);
  s.write_field("num_ref_idx_l1_active_minus1", num_ref_idx_l1_active_minus1);
  s.write_field("slice_group_change_rate_minus1", slice_group_change_rate_minus1);
  s.write_field("pic_parameter_set_id", pic_parameter_set_id);
  s.write_field("seq_parameter_set_id", seq_parameter_set_id);
  s.write_field("pic_order_present_flag", pic_order_present_flag);
  s.write_field("num_slice_groups_minus1", num_slice_groups_minus1);
  s.write_field("slice_group_map_type", slice_group_map_type);
  s.write_field("weighted_pred_flag", weighted_pred_flag);
  s.write_field("weighted_bipred_idc", weighted_bipred_idc);
  s.write_field("pic_init_qp_minus26", pic_init_qp_minus26);
  s.write_field("pic_init_qs_minus26", pic_init_qs_minus26);
  s.write_field("chroma_qp_index_offset", chroma_qp_index_offset);
  s.write_field("deblocking_filter_control_present_flag", deblocking_filter_control_present_flag);
  s.write_field("constrained_intra_pred_flag", constrained_intra_pred_flag);
  s.write_field("redundant_pic_cnt_present_flag", redundant_pic_cnt_present_flag);
  s.write_array("top_left", top_left, num_slice_groups_minus1 + 1);
  s.write_array("bottom_right", bottom_right, num_slice_groups_minus1 + 1);
  s.write_array("run_length_minus1", run_length_minus1, num_slice_groups_minus1 + 1);
  s.write_field("slice_group_change_direction_flag", slice_group_change_direction_flag);
  s.write_field("pic_size_map_units_minus1", pic_size_map_units_minus1);
  s.write_array("slice_group_id", slice_group_id, pic_size_map_units_minus1);

  if (extended) {
    s.write_object("extended", extended->str(indent+1));
  }

  s.end_json();
  return s.str();
}

// class pps_ext

string pps_ext::str(unsigned indent) const
{
  std::stringstream stream;
  json_serializer s(stream, "pps_ext", indent);

  s.start_json();

  // TODO: serialize PPS Ext data structure.

  s.end_json();

  return s.str();
}

}; // namespace isomf

