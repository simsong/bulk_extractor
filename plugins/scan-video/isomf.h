#ifndef _ISOMF_H_
#define _ISOMF_H_

#include <iostream>
#include <list>

#include "isomf_base.h"
#include "h264_model.h"

namespace isomf {

/// Sample Description Box (ISO/IEC 14496-12 8.5.2)
class sample_description_box : public full_box 
{
  public:
    sample_description_box();
    ~sample_description_box();

    virtual size_t content_size() const 
    { 
      // 4 byte entry count
      return super::content_size() + 4; 
    }

  protected:
    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;

  private:  
    typedef full_box super;
};

/// Sample Entry Box (ISO/IEC 14496-12 8.5.2.2)
class sample_entry : public box
{
  public:
    sample_entry();
    virtual ~sample_entry();

    unsigned data_reference_index() const { return _data_ref_index; }
    void data_reference_index(unsigned index) { _data_ref_index = index; }

    virtual size_t content_size() const 
    { 
      // 2 byte data reference index + 6 byte reserved data
      return super::content_size() + 8; 
    }

  protected:
    unsigned _data_ref_index;

    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;

  private:
    typedef box super;
};

/// Visual Sample Entry Box (ISO/IEC 14496-12 8.5.2.2)
class visual_sample_entry : public sample_entry
{
  public:
    visual_sample_entry();
    visual_sample_entry(const std::string& t);
    virtual ~visual_sample_entry();

    unsigned width() const { return _width; }
    void width(unsigned w) { _width = w; }

    unsigned height() const { return _height; }
    void height(unsigned h) { _height = h; }

    unsigned hres() const { return _hres; }
    void hres(unsigned hr) { _hres = hr; }

    unsigned vres() const { return _vres; }
    void vres(unsigned vr) { _vres = vr; }

    unsigned frame_count() const { return _frame_count; }
    void frame_count(unsigned fc) { _frame_count = fc; }

    unsigned depth() const { return _depth; }
    void depth(unsigned d) { _depth = d; }

    std::string compressor() const { return _compressor; }
    void compressor(std::string& name) { _compressor = name; }

    virtual size_t content_size() const 
    { return super::content_size() + 70; }

  protected:
    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;

    unsigned _width;
    unsigned _height;
    unsigned _frame_count;
    unsigned _depth;
    // 16.16 fixed point horizontal resolution
    unsigned _hres;
    // 16.16 fixed point vertical resolution
    unsigned _vres;
    std::string _compressor;

  private:  
    typedef sample_entry super;
};

/// Audio Sample Entry Box (ISO/IEC 14496-12 8.5.2.2)
class audio_sample_entry : public sample_entry
{
  public:
    audio_sample_entry();
    audio_sample_entry(const std::string& codec_type);
    ~audio_sample_entry() {};

    unsigned channel_count() const { return _channel_count; }
    void channel_count(unsigned ch_cnt) { _channel_count = ch_cnt; }

    unsigned sample_size() const { return _sample_size; }
    void sample_size(unsigned sz) { _sample_size = sz; }

    unsigned compression_id() const { return _compression_id; }
    void compression_id(unsigned id) { _compression_id = id; }

    unsigned packet_size() const { return _packet_size; }
    void packet_size(unsigned sz) { _packet_size = sz; }

    unsigned sample_rate() const { return _sample_rate; }
    void sample_rate(unsigned rate) { _sample_rate = rate; }

    unsigned samples_per_packet() const { return _samples_per_packet; }
    void samples_per_packet(unsigned n) { _samples_per_packet = n; }

    unsigned bytes_per_packet() const { return _bytes_per_packet; }
    void bytes_per_packet(unsigned n) { _bytes_per_packet = n; }

    unsigned bytes_per_frame() const { return _bytes_per_frame; }
    void bytes_per_frame(unsigned n) { _bytes_per_frame = n; }

    unsigned bytes_per_sample() const { return _bytes_per_sample; }
    void bytes_per_sample(unsigned n) { _bytes_per_sample = n; }

    virtual size_t content_size() const;

  protected:
    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;

    unsigned _sound_version;
    unsigned _channel_count; 
    unsigned _sample_size; 
    unsigned _compression_id;
    unsigned _packet_size;
    unsigned _sample_rate;

    unsigned _samples_per_packet;
    unsigned _bytes_per_packet;
    unsigned _bytes_per_frame;
    unsigned _bytes_per_sample;

    char _version2_data[20];

  private:  
    typedef sample_entry super;
};


/// AVC Configuration Box (ISO 14496-15 5.2.4.1)
class avc_configuration_box : public box
{
  public:
    avc_configuration_box();
    virtual ~avc_configuration_box() {};

    virtual size_t content_size() const; 

    unsigned configuration_version() const { return _configuration_version; }
    void configuration_version(unsigned v) { _configuration_version = v; }

    unsigned avc_profile_indication() const {return _avc_profile_indication;}
    void avc_profile_indication(unsigned v) {_avc_profile_indication = v;}

    unsigned profile_compatibility() const {return _profile_compatibility;}
    void profile_compatibility(unsigned v) {_profile_compatibility = v;}

    unsigned avc_level_indication() const {return _avc_level_indication;}
    void avc_level_indication(unsigned v) {_avc_level_indication = v;}

    unsigned length_size_minus_one() const {return _length_size_minus_one;}
    void length_size_minus_one(unsigned v) {_length_size_minus_one = v;}

    void add(sequence_parameter_set& set) {_sps.push_back(&set);}
    void add(picture_parameter_set& set) {_pps.push_back(&set);}

    void remove(sequence_parameter_set& set) {_sps.remove(&set);}
    void remove(picture_parameter_set& set) {_pps.remove(&set);}

    std::list<sequence_parameter_set*>& get_sps_list() { return _sps; }
    std::list<picture_parameter_set*>& get_pps_list() { return _pps; }

  protected:
    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;

    unsigned _configuration_version;
    unsigned _avc_profile_indication;
    unsigned _profile_compatibility;
    unsigned _avc_level_indication;
    unsigned _length_size_minus_one;
    
    std::list<sequence_parameter_set*> _sps;
    std::list<picture_parameter_set*> _pps;

  private:  
    typedef box super;
};

// base class for descriptor classes

class descriptor_box : public full_box, public data_holder
{
  public:
    descriptor_box();
    virtual ~descriptor_box();

    virtual size_t content_size() const; 

  protected:
    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;
};

class es_descriptor_box : public descriptor_box
{
  public:
    es_descriptor_box();
    virtual ~es_descriptor_box() {};
};

class d263_box : public box, public data_holder
{
  public:
    d263_box();
    virtual ~d263_box();

    virtual size_t content_size() const;

  protected:
    virtual size_t parse_box_content(char* buf, size_t sz);
    virtual size_t write_box_content(char* buf, size_t sz) const;
};

}; //namespace isomf

#endif

