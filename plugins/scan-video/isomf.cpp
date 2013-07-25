#include <iostream>
#include <list>
#include <string.h>

#include "type_io.h"
#include "bytestream_reader.h"
#include "isomf.h"

#define MAX_AVCC_SIZE 4096

namespace isomf {

// class box 

box::box() 
  : _type(""),_parent(NULL),_children()
{
  memset(_extended_type, 0, 16);
}

box::box(const std::string& box_type) 
  : _type(box_type),_parent(NULL),_children()
{
  memset(_extended_type, 0, 16);
}

box::box(const box& b) 
  : _type(""),_parent(NULL),_children()
{
  operator=(b);
}

void box::get_extended_type(char* type_buf) const
{
  if (! type_buf) { return; }
  memcpy(type_buf, _extended_type, 16);
}

void box::set_extended_type(char* type_buf) 
{
  if (! type_buf) { return; }
  memcpy(_extended_type, type_buf, 16);
}

size_t box::size() const
{
  size_t total = 0;
  total += 8; // size | type
  total += is_small() ? 0 : 8; // large size
  total += _type == "uuid" ? 16 : 0; // extended type
  total += content_size();

  return total;
}

void box::add(const box& c)
{
  _children.push_back(&c);
}

void box::remove(const box& c)
{
  _children.remove(&c);
}

bool box::is_small() const
{
  return true;
  /*
  size_t header_size = 8;
  if (_type == "uuid") {
    header_size += 16;
  }

  unsigned long long total_size = content_size();
  total_size += header_size;

  if ((total_size >> 32) == 0) {
    return true;
  } else {
    return false;
  }
  */
}

size_t box::content_size() const
{
  size_t sz = 0;
  std::list<const box*>::const_iterator itr = _children.begin();
  for (; itr != _children.end(); itr++) {
    sz += (*itr)->size();
  }
  return sz;
}

box& box::operator=(const box& b)
{
  _type = b.type();
  _parent = b.parent();
  _children = b.children();

  if (_type == "uuid") {
    b.get_extended_type(_extended_type);
  }
  return *this;
}

size_t box::write(char* buf, size_t sz) const
{
  char* t = buf;
  t += write_header(t, sz);
  std::cerr << "Header written: " << t-buf << " bytes.\n";

  t += write_box_content(t, sz);
  std::cerr << "Box content written: " << t-buf << " bytes.\n";

  t += write_children(t, sz);
  std::cerr << "Children boxes written: " << t-buf << " bytes.\n";

  return t-buf;
}

size_t box::write_header(char* buf, size_t sz) const
{
  if (! buf) { return 0; }

  if (sz < size()) {
    std::cerr << "Buffer too small for the box header." << std::endl;
    return 0;
  }

  std::cerr << "Writing header for type=" << _type << " and size=" << size() << "\n";

  char* t = buf;
  if (is_small()) {
    t += type_io::write_uint32((unsigned) size(), t);
    t += type_io::write_fourcc(_type, t);
  } else {
    t += type_io::write_uint32((unsigned) 1, t);
    t += type_io::write_fourcc(_type, t);
    t += type_io::write_uint64(size(), t);
  }
  
  if (_type == "uuid") {
    memcpy(buf, _extended_type, 16);
    t += 16;
  }
  return (t-buf);
}

size_t box::write_box_content(char* buf, size_t sz) const
{
  return 0;
}

size_t box::write_children(char* buf, size_t sz) const
{
  char* t = buf;
  size_t total = 0;
  size_t nb = 0;

  std::list<const box*>::const_iterator itr = _children.begin();
  for (; itr != _children.end(); itr++) {
    nb += (*itr)->write(t, sz-total);
    t += nb;
    total += nb;
    if (total >= sz) {
      return total;
    }
  }
  return t-buf;
}

size_t box::parse(char* buf, size_t sz)
{
  char* t = buf;
  t += parse_header(t, sz);
  std::cerr << "Header parsed: " << t-buf << " bytes.\n";

  t += parse_box_content(t, sz);
  std::cerr << "Box content parsed: " << t-buf << " bytes.\n";

  t += parse_children(t, sz);
  std::cerr << "Children boxes written: " << t-buf << " bytes.\n";

  return t-buf;
}

size_t box::parse_header(char* buf, size_t sz)
{
  bytestream_reader reader(buf, sz);

  long long s = reader.read_uint32();
  if (s < 0) { return 0; }

  std::cerr << " >> Parsed header size: " << s << "\n";

  std::string* fourcc = reader.read_fourcc(); 
  if (fourcc == NULL) { return 0; }
  _type = *fourcc;

  std::cerr << " >> Parsed header fourcc code: " << _type << "\n";

  if (s == 1) {
    s = reader.read_uint64();
    if (s < 0) { return 0; }
  }

  if (_type == "uuid") {
    if (reader.available() < 16) { return 0; }
    reader.read_array(_extended_type, 16);
  }
  return reader.read_count();
}

size_t box::parse_box_content(char* buf, size_t sz) 
{
  // no content for the base box class
  return 0;
}

size_t box::parse_children(char* buf, size_t sz)
{
  // TODO:
  return 0;
}

// class full_box

full_box::full_box() :_version(0),_flags(0)
{
}

size_t full_box::parse_box_content(char* buf, size_t sz) 
{
  if (buf == NULL || sz < 4) {
    std::cerr << "Buffer is NULL or too small for fullbox content.\n";
    return 0;
  }
  _version = type_io::read_uint8(buf);
  buf++;
  _flags = type_io::read_uint24(buf);
  return 4;
}

size_t full_box::write_box_content(char* buf, size_t sz) const 
{
  if (buf == NULL || sz < 4) {
    std::cerr << "Buffer is NULL or too small for fullbox content.\n";
    return 0;
  }
  type_io::write_uint8(_version, buf);
  buf++;
  type_io::write_uint24(_flags, buf);
  return 4;
}

full_box& full_box::operator=(const full_box& b)
{
  super::operator=(b);
  _version = b.version();
  _flags = b.flags();
  return *this;
}

// class sample_description_box

sample_description_box::sample_description_box() 
{
  _type = "stsd";
}

sample_description_box::~sample_description_box() 
{
}

size_t sample_description_box::write_box_content(char* buf, size_t sz) const
{
  char* t = buf;
  t += super::write_box_content(t, sz);
  t += type_io::write_uint32((unsigned)_children.size(), t);
  return t-buf;
}

size_t sample_description_box::parse_box_content(char* buf, size_t sz) 
{
  return 0;
}

// class sample_entry 

sample_entry::sample_entry() :_data_ref_index(1)
{
}

sample_entry::~sample_entry()
{
}

size_t sample_entry::write_box_content(char* buf, size_t sz) const
{
  char* t = buf;
  t += super::write_box_content(t, sz);
  t += type_io::write_unsigned(0, t, 6);
  t += type_io::write_uint16(_data_ref_index, t);
  return t-buf;
}

size_t sample_entry::parse_box_content(char* buf, size_t sz) 
{
  return 0;
}

// class visual_sample_entry

visual_sample_entry::visual_sample_entry()
  :_width(0),_height(0),_frame_count(1),_depth(0x18),
  _hres(0x00480000),_vres(0x00480000),_compressor()
{
  _type = "avc1";
}

visual_sample_entry::visual_sample_entry(const std::string& t)
  :_width(0),_height(0),_frame_count(1),_depth(0x18),
  _hres(0x00480000),_vres(0x00480000),_compressor()
{
  _type = t;
}

visual_sample_entry::~visual_sample_entry()
{
}

size_t visual_sample_entry::write_box_content(char* buf, size_t sz) const
{
  char* t = buf;
  t += super::write_box_content(t, sz);
  t += type_io::write_uint16(0, t); // 16 bit 'pre_defined'
  t += type_io::write_uint16(0, t); // 16 bit 'reserved'

  // array of three 32-bit 'pre_defined'
  t += type_io::write_uint32(0, t); 
  t += type_io::write_uint32(0, t); 
  t += type_io::write_uint32(0, t); 

  t += type_io::write_uint16(_width, t); 
  t += type_io::write_uint16(_height, t); 
  t += type_io::write_uint32(_hres, t); 
  t += type_io::write_uint32(_vres, t); 

  t += type_io::write_uint32(0, t);  // 32-bit 'reserved'

  t += type_io::write_uint16(_frame_count, t); 
  t += type_io::write_string(_compressor, 32, t); 
  t += type_io::write_uint16(_depth, t); 

  t += type_io::write_uint16(0xffff, t); // 16-bit 'pre-defined' -1

  return t-buf;
}

size_t visual_sample_entry::parse_box_content(char* buf, size_t sz) 
{
  return 0;
}

// class audio_sample_entry

audio_sample_entry::audio_sample_entry()
  :_sound_version(0),_channel_count(2),_sample_size(16),
  _compression_id(0),_packet_size(0),_sample_rate(8000),
  _samples_per_packet(0),_bytes_per_packet(0),_bytes_per_frame(0),
  _bytes_per_sample(0)
{
  _type = "mp4a";
  memset(_version2_data, 0, sizeof(_version2_data));
}

audio_sample_entry::audio_sample_entry(const std::string& codec_type)
  :_sound_version(0),_channel_count(2),_sample_size(16),
  _compression_id(0),_packet_size(0),_sample_rate(8000),
  _samples_per_packet(0),_bytes_per_packet(0),_bytes_per_frame(0),
  _bytes_per_sample(0)
{
  _type = codec_type;
  memset(_version2_data, 0, sizeof(_version2_data));
}

size_t audio_sample_entry::write_box_content(char* buf, size_t sz) const
{
  char* t = buf;
  t += super::write_box_content(t, sz);

  t += type_io::write_uint16(_sound_version, t);

  t += type_io::write_uint16(0, t);
  t += type_io::write_uint32(0, t);

  t += type_io::write_uint16(_channel_count, t);
  t += type_io::write_uint16(_sample_size, t);
  t += type_io::write_uint16(_compression_id, t);
  t += type_io::write_uint16(_packet_size, t);

  if (_type == "mlpa") {
    t += type_io::write_uint32(_sample_rate, t);
  } else {
    t += type_io::write_uint32(_sample_rate << 16, t);
  }

  if (_sound_version > 0) {
    t += type_io::write_uint32(_samples_per_packet, t);
    t += type_io::write_uint32(_bytes_per_packet, t);
    t += type_io::write_uint32(_bytes_per_frame, t);
    t += type_io::write_uint32(_bytes_per_sample, t);
  }

  if (_sound_version == 2) {
    t += type_io::write_bytes(_version2_data, sizeof(_version2_data), t);
  }

  return (t-buf);
}

size_t audio_sample_entry::parse_box_content(char* buf, size_t sz) 
{
  return 0;
}

size_t audio_sample_entry::content_size() const 
{
  size_t sz = super::content_size();
  sz += 20;
  if (_sound_version > 0) { sz += 16; }
  if (_sound_version == 2) { sz += 20; }
  return sz;
}

// class data_holder

data_holder::data_holder()
  :_data(NULL),_data_size(0)
{
}

data_holder::data_holder(const data_holder& d)
  :_data(NULL),_data_size(0)
{
  operator=(d);
}

data_holder::~data_holder()
{
  clear();
}

data_holder& data_holder::operator=(const data_holder& d)
{
  clear();

  _data_size = d.size();
  if (_data_size > 0) {
    _data = new char[_data_size];
    d.get_data(_data, _data_size);
  }
  return *this;
}

size_t data_holder::set_data(const char* data, size_t sz)
{
  clear();
  if (data && sz > 0) {
    _data_size = sz;
    _data = new char[sz];
    memcpy(_data, data, sz);
    return parse(data, sz);
  }
  return 0;
}

size_t data_holder::get_data(char* data, size_t sz) const
{
  if (_data_size == 0 || ! _data || ! data || sz < _data_size) {
    return 0;
  }

  if (_data) {
    size_t nb = _data_size > sz ? sz : _data_size;
    memcpy(data, _data, nb);
    return nb;
  } else {
    return write(data, sz);
  }
}

void data_holder::clear()
{
  if (_data) {
    delete _data;
    _data = NULL;
  }
  _data_size = 0;
}


// class avc_configuration_box

avc_configuration_box::avc_configuration_box()
  :_configuration_version(1),
  _avc_profile_indication(66),
  _profile_compatibility(192),
  _avc_level_indication(13),
  _length_size_minus_one(3),
  _sps(),
  _pps()
{
  _type = "avcC";
}

size_t avc_configuration_box::content_size() const
{
  size_t sz = 7; // fixed length

  // variable part
  std::list<sequence_parameter_set*>::const_iterator itr1 = _sps.begin();
  for (; itr1 != _sps.end(); itr1++) {
    sz += 2;
    sz += (*itr1)->size();
  }

  std::list<picture_parameter_set*>::const_iterator itr2 = _pps.begin();
  for (; itr2 != _pps.end(); itr2++) {
    sz += 2;
    sz += (*itr2)->size();
  }
  
  return sz;
}

size_t avc_configuration_box::parse_box_content(char* buf, size_t sz)
{
  if (! buf || sz == 0) { return 0; }

  char* t = buf;
  t += super::parse_box_content(t, sz);
  sz -= (t-buf);

  bytestream_reader reader(t, sz);

  if (reader.available() < 7) { 
    std::cerr << "Not enough data for avc box.\n";
    return 0; 
  }

  _configuration_version = (unsigned) reader.read_uint8();
  _avc_profile_indication = (unsigned) reader.read_uint8();
  _profile_compatibility = (unsigned) reader.read_uint8();
  _avc_level_indication = (unsigned) reader.read_uint8();
  _length_size_minus_one = (unsigned) reader.read_uint8() & 0x03;
  int num_sps = (int) reader.read_byte() & 0x1F;

  for (int i=0; i<num_sps; i++) {
    int sps_size = reader.read_uint16();
    if (sps_size < 0 || reader.available() < sps_size) { 
      std::cerr <<  "Invalid SPS size: " << sps_size << std::endl;
      return reader.read_count(); 
    }

    size_t offset = reader.read_count();
    sequence_parameter_set* sps = new sequence_parameter_set();
    size_t nb = sps->parse(t+offset, sps_size);
    if (nb != sps_size) {
      std::cerr <<  "WARN: Only " << nb << " bytes parsed in SPS, " 
                << sps_size << " bytes expected.\n";
    }
    _sps.push_back(sps);
    reader.skip(sps_size);
  }

  int num_pps = (int) reader.read_uint8();
  if (num_pps < 0) { return reader.read_count(); }

  for (int i=0; i<num_pps; i++) {
    int pps_size = reader.read_uint16();
    if (pps_size < 0 || reader.available() < pps_size) { 
      std::cerr <<  "Invalid PPS size: " << pps_size << std::endl;
      return reader.read_count(); 
    }

    size_t offset = reader.read_count();
    picture_parameter_set* pps = new picture_parameter_set();
    size_t nb = pps->parse(t + offset, pps_size);
    if (nb != pps_size) {
      std::cerr <<  "WARN: Only " << nb << " bytes parsed in PPS, " 
                << pps_size << " bytes expected.\n";
    }
    _pps.push_back(pps);
    reader.skip(pps_size);
  }

  return reader.read_count();
}

size_t avc_configuration_box::write_box_content(char* buf, size_t sz) const
{
  char* t = buf;
  t += super::write_box_content(t, sz);
  t += type_io::write_uint8(_configuration_version, t);
  t += type_io::write_uint8(_avc_profile_indication, t);
  t += type_io::write_uint8(_profile_compatibility, t);
  t += type_io::write_uint8(_avc_level_indication, t);
  t += type_io::write_uint8(_length_size_minus_one | 0xFC, t);

  t += type_io::write_uint8(_sps.size() | 0xE0, t);

  std::list<sequence_parameter_set*>::const_iterator itr1 = _sps.begin();
  for (; itr1 != _sps.end(); itr1++) {
    int sz = (*itr1)->size();
    t += type_io::write_uint16(sz, t);
    t += (*itr1)->write(t, sz);
  }

  t += type_io::write_uint8(_pps.size(), t);

  std::list<picture_parameter_set*>::const_iterator itr2 = _pps.begin();
  for (; itr2 != _pps.end(); itr2++) {
    int sz = (*itr2)->size();
    t += type_io::write_uint16(sz, t);
    t += (*itr2)->write(t, sz);
  }

  return t-buf;
}

// class descriptor_box

descriptor_box::descriptor_box()
{
}

descriptor_box::~descriptor_box()
{
}

size_t descriptor_box::content_size() const
{
  return full_box::content_size() + data_holder::size();
}

size_t descriptor_box::parse_box_content(char* buf, size_t sz)
{
  return 0;
}

size_t descriptor_box::write_box_content(char* buf, size_t sz) const
{
  char* t = buf;
  t += full_box::write_box_content(t, sz);
  t += data_holder::get_data(t, _data_size);
  return t-buf;
}

// class es_descriptor_box

es_descriptor_box::es_descriptor_box()
{
  _type = "esds";
}

// class d263_box

d263_box::d263_box()
{
  _type = "d263";
}

d263_box::~d263_box()
{
}

size_t d263_box::content_size() const
{
  return box::content_size() + data_holder::size();
}

size_t d263_box::parse_box_content(char* buf, size_t sz)
{
  return 0;
}

size_t d263_box::write_box_content(char* buf, size_t sz) const
{
  char* t = buf;
  t += box::write_box_content(t, sz);
  t += data_holder::get_data(t, _data_size);
  return t-buf;
}

}; //namespace isomf

