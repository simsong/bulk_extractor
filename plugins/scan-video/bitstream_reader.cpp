#include <iostream>
#include <string.h>

#include "bitstream_reader.h"

using namespace std;

bitstream_reader::bitstream_reader()
  :_data(NULL),_data_size(0),_bit_pos(0),_byte_pos(0),_bits_read(0)
{
}

bitstream_reader::bitstream_reader(const char* data, size_t size)
  :_data(NULL),_data_size(0),_bit_pos(0),_byte_pos(0),_bits_read(0)
{
  set_data(data, size);
}

bitstream_reader::bitstream_reader(const bitstream_reader& reader)
  :_data(NULL),_data_size(0),_bit_pos(0),_byte_pos(0),_bits_read(0)
{
  reader.copy_data(*this);
}

bitstream_reader::~bitstream_reader()
{
  if (_data) {
    delete[] _data;
  }
}

void bitstream_reader::set_data(const char* data, size_t size)
{
  if (data && size > 0) {
    _data = new char[size];
    memcpy(_data, data, size);
    _data_size = size;
    _bit_pos = 0;
    _byte_pos = 0;
    _bits_read = 0;
  }
}

void bitstream_reader::copy_data(bitstream_reader& reader) const
{
  reader.set_data(_data, _data_size);
}

bitstream_reader& bitstream_reader::operator=(const bitstream_reader& reader)
{
  reader.copy_data(*this);
  return *this;
}

int bitstream_reader::read_bit()
{
  if (_data == NULL || _data_size == 0 || (size_t)_byte_pos >= _data_size)
  {
    cerr << "*** Error reading bit at byte: " << _byte_pos 
         << ", bit: " << _bit_pos << ", total: " << _data_size << endl;
    return -1;
  }
  int b = (_data[_byte_pos] >> (7 - _bit_pos)) & 1;
  _bit_pos++;
  if (_bit_pos > 7) {
    _bit_pos = 0;
    _byte_pos++;
  }
  _bits_read++;
  return b;
}

long long bitstream_reader::read_bits(int n)
{
  if (_data == NULL || _data_size == 0 || 
      n < 0 || (size_t)n > sizeof(long long)*8)
  {
    return -1;
  }
  if (n == 0) { return 0; }

  long long v = 0;
  for (int i=0; i<n; i++) {
    int b = read_bit();
    if (b == -1) {
      return -1;
    }

    v <<= 1;
    v |= b;
  }
  return v;
}

int bitstream_reader::read_byte()
{
  if (_data == NULL || _data_size == 0 || (size_t)_byte_pos >= _data_size) {
    return -1;
  }

  // move to the next byte
  if (_bit_pos > 0) {
    _bits_read += (8-_bit_pos);
    _bit_pos = 0;
    _byte_pos++;
  }

  int byte = _data[_byte_pos];
  _byte_pos++;
  _bits_read += 8;
  return byte;
}

int bitstream_reader::read_remaining_byte()
{
  if (_data == NULL || _data_size == 0 || (size_t)_byte_pos >= _data_size) {
    return -1;
  }
  return read_bits(8-_bit_pos);
}

int bitstream_reader::read_UE()
{
  int cnt = 0;
  while (read_bit() == 0) { cnt++; }

  int res = 0;
  if (cnt > 0) {
    long val = read_bits(cnt);
    res = (int) ((1 << cnt) - 1 + val);
  }
  return res;
}

int bitstream_reader::read_SE()
{
  int val = read_UE();
  if (val == -1) { return -1; }

  int sign = ((val & 0x1) << 1) -1;
  val = ((val >> 1) + (val & 0x1)) * sign;
  return val;
}

bool bitstream_reader::read_bool()
{
  return (read_bit() == 1);
}

int bitstream_reader::read_U(int i)
{
  return read_bits(i);
}

int bitstream_reader::read_TE(int max)
{
  if (max > 1) { return read_UE(); }
  return ~read_bit() & 0x1;
}

char* bitstream_reader::read_bytes(int size)
{
  if (size <= 0) { return NULL; }

  char* result = new char[size];
  for (int i=0; i<size; i++) {
    result[i] = read_byte() & 0xff;
  }
  return result;
}

int bitstream_reader::read_zero_bit_count()
{
  int cnt = 0;
  while (read_bit() == 0) {
    cnt++;
  }
  return cnt;
}

void bitstream_reader::read_trailing_bits()
{
  read_bit();
  read_remaining_byte();
}

bool bitstream_reader::more_rbsp_data()
{
  if (_data == NULL || _byte_pos >= _data_size) {
    return false;
  }

  int cur_byte = _data[_byte_pos] & 0xff;
  int next_byte = _byte_pos == _data_size-1 ? -1 : _data[_byte_pos+1] & 0xff;

  int tail = 1 << (8-_bit_pos-1);
  int mask = ((tail << 1) -1);
  bool has_tail = (cur_byte & mask) == tail;

  return ! (next_byte == -1 && has_tail);
}


