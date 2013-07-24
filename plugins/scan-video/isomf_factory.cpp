#include <iostream>
#include <string.h>

#include "isomf.h"
#include "isomf_factory.h"
#include "type_io.h"

namespace isomf {

// sample sequence parameter set from a real video. 
// unfortunately this part varies from video to video.
static char _sample_sps[27] = {
  0x67, 0x42, 0xc0, 0x0d, 0xab, 0x40, 0xa0, 0xfd, 
  0xff, 0x80, 0x07, 0x80, 0x08, 0x08, 0x00, 0x00,
  0x03, 0x00, 0xc8, 0x00, 0x00, 0x0f, 0xc4, 0x78,
  0xa1, 0x55, 0x01
};

// sample picture parameter set from a real video. more stable than sps.
static char _sample_pps[4] = {0x68, 0xce, 0x3c, 0x80};

// sample description box.
static char _sample_esds[44] = {
  0x03, 0x2a, 0x00, 0x00, 0x10, 0x04, 0x22, 0x20,
  0x11, 0x00, 0x50, 0x00, 0x00, 0x01, 0x64, 0x90,
  0x00, 0x01, 0x39, 0xe9, 0x05, 0x13, 0x00, 0x00, 
  0x01, 0x00, 0x00, 0x00, 0x01, 0x20, 0x00, 0xc8,
  0x88, 0x80, 0x07, 0xd0, 0x58, 0x41, 0x21, 0x41,
  0x03, 0x06, 0x01, 0x02
};

static char _sample_d263[7] = {
  0x46, 0x46, 0x4d, 0x50, 0x00, 0x0a, 0x00
};

/*
static char _sample_d263[7] = {
  0x73, 0x36, 0x30, 0x20, 0x00, 0x0a, 0x00
};
*/

// class parameter_set_factory

sequence_parameter_set* parameter_set_factory::get_avc_sps()
{
  sequence_parameter_set* sps = new sequence_parameter_set();

  // The following sets SPS from raw data
  //size_t nb = sps->parse(_sample_sps, sizeof(_sample_sps));
  //cerr << "SPS parsed " << nb << " bytes." << endl;

  // Instead, we can set it to be most frequently occurred values.
  sps->pic_order_cnt_type = 0;
  sps->field_pic_flag = false;
  sps->delta_pic_order_always_zero_flag = false;
  sps->entropy_coding_mode_flag = false;
  sps->mb_adaptive_frame_field_flag = false;
  sps->direct_8x8_inference_flag = true;
  sps->chroma_format_idc = &(chroma_format::YUV_420);
  sps->log2_max_frame_num_minus4 = 1;
  sps->log2_max_pic_order_cnt_lsb_minus4 = 1;
  sps->pic_height_in_map_units_minus1 = 39;
  sps->pic_width_in_mbs_minus1 = 2;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  sps->qpprime_y_zero_transform_bypass_flag = false;
  sps->profile_idc = 39;
  sps->constraint_set_0_flag = false;
  sps->constraint_set_1_flag = true;
  sps->constraint_set_2_flag = false;
  sps->constraint_set_3_flag = false;
  sps->level_idc = 224;
  sps->seq_parameter_set_id = 14;
  sps->residual_color_transform_flag = false;
  sps->offset_for_non_ref_pic = 0;
  sps->offset_for_top_to_bottom_field = 0;
  sps->num_ref_frames = 1;
  sps->gaps_in_frame_num_value_allowed_flag = false;
  sps->frame_mbs_only_flag = true;
  sps->frame_cropping_flag = true;
  sps->frame_crop_left_offset = 0;
  sps->frame_crop_right_offset = 0;
  sps->frame_crop_top_offset = 0;
  sps->frame_crop_bottom_offset = 0;
  sps->offset_for_ref_frame = NULL;
  sps->num_ref_frames_in_pic_order_cnt_cycle = 0;
  sps->reserved = 2;

  sps->vui_params = NULL;
  sps->scalingmatrix = NULL;
  
  return sps;
}

picture_parameter_set* parameter_set_factory::get_avc_pps()
{
  picture_parameter_set* pps = new picture_parameter_set();

  // The following line generates PPS from a sample
  //pps->parse(_sample_pps, sizeof(_sample_pps));
  
  // Instead we can set common values
  pps->entropy_coding_mode_flag = false;
  pps->num_ref_idx_l0_active_minus1 = 37;
  pps->num_ref_idx_l1_active_minus1 = 3;
  pps->slice_group_change_rate_minus1 = 0;
  pps->pic_parameter_set_id = 4;
  pps->seq_parameter_set_id = 12;
  pps->pic_order_present_flag = true;
  pps->num_slice_groups_minus1 = 0;
  pps->slice_group_map_type = 0;
  pps->weighted_pred_flag = false;
  pps->weighted_bipred_idc = 0;
  pps->pic_init_qp_minus26 = 0;
  pps->pic_init_qs_minus26 = 0;
  pps->chroma_qp_index_offset = 0;
  pps->deblocking_filter_control_present_flag = false;
  pps->constrained_intra_pred_flag = false;
  pps->redundant_pic_cnt_present_flag = false;
  pps->top_left = NULL;
  pps->bottom_right = NULL;
  pps->run_length_minus1 = NULL;
  pps->slice_group_change_direction_flag = false;
  pps->pic_size_map_units_minus1 = 0;
  pps->slice_group_id = NULL;
  pps->extended = NULL;

  return pps;
}

// class box_factory

es_descriptor_box* box_factory::get_esds()
{
  es_descriptor_box* esds = new es_descriptor_box();
  esds->set_data(_sample_esds, sizeof(_sample_esds));
  return esds;
}

sample_description_box* 
box_factory::get_avc_stsd(unsigned width, unsigned height)
{
  sample_description_box* box = new sample_description_box();

  visual_sample_entry* entry = new visual_sample_entry("avc1");
  entry->width(width);
  entry->height(height);

  box->add(*entry);

  avc_configuration_box* avcc = new avc_configuration_box();
  entry->add(*avcc);

  sequence_parameter_set* sps = parameter_set_factory::get_avc_sps();
  picture_parameter_set* pps = parameter_set_factory::get_avc_pps();

  avcc->add(*sps);
  avcc->add(*pps);

  //std::cerr << " >> Got avcC box of size: " << avcc->size() 
  //          << ", content size: " << avcc->content_size() << "\n";

  return box;
}

sample_description_box* 
box_factory::get_mp4a_stsd()
{
  sample_description_box* box = new sample_description_box();

  audio_sample_entry* entry = new audio_sample_entry("mp4a");
  box->add(*entry);

  es_descriptor_box* esds = get_esds();
  entry->add(*esds);

  return box;
}

sample_description_box* 
box_factory::get_mp4v_stsd(unsigned width, unsigned height)
{
  sample_description_box* box = new sample_description_box();

  visual_sample_entry* entry = new visual_sample_entry("mp4v");
  entry->width(width);
  entry->height(height);

  box->add(*entry);

  es_descriptor_box* esds = box_factory::get_esds();
  entry->add(*esds);

  return box;
}

sample_description_box* 
box_factory::get_h263_stsd(unsigned width, unsigned height)
{
  sample_description_box* box = new sample_description_box();

  visual_sample_entry* entry = new visual_sample_entry("h263");
  entry->width(width);
  entry->height(height);

  box->add(*entry);

  d263_box* d263 = new d263_box();
  d263->set_data(_sample_d263, sizeof(_sample_d263));
  entry->add(*d263);

  return box;
}

void box_factory::print_stsd(sample_description_box* box)
{
  if (! box) {
    return;
  }

  size_t sz = box->size();
  std::cout << "===== STSD sample description box: " << sz << " bytes\n";
  char* buf = new char[sz];
  memset(buf, 0xfe, sz);
  size_t nb = box->write(buf, sz);
  std::cerr << "===== STSD Written " << nb << " bytes.\n";
  type_io::print_hex(buf, static_cast<int>(nb));
  std::cerr << "===== STSD DONE\n";
}


}; // namespace isomf
