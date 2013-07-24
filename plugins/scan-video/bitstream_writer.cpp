#include <iostream>
#include <string.h>

#include "bitstream_writer.h"

bitstream_writer::bitstream_writer()
  :_data(NULL),_data_size(0),_bit_pos(0),_byte_pos(0),_bits_written(0)
{
}

bitstream_writer::bitstream_writer(char* data, size_t size)
  :_data(NULL),_data_size(0),_bit_pos(0),_byte_pos(0),_bits_written(0)
{
  set_data(data, size);
}

bitstream_writer::bitstream_writer(const bitstream_writer& writer)
  :_data(NULL),_data_size(0),_bit_pos(0),_byte_pos(0),_bits_written(0)
{
  writer.copy_data(*this);
}

bitstream_writer::~bitstream_writer()
{
}

void bitstream_writer::set_data(char* data, size_t size)
{
  if (data && size > 0) {
    _data = data;
    _data_size = size;
    _bit_pos = 0;
    _byte_pos = 0;
    _bits_written = 0;
  }
}

void bitstream_writer::copy_data(bitstream_writer& writer) const
{
  writer.set_data(_data, _data_size);
}

bitstream_writer& bitstream_writer::operator=(const bitstream_writer& writer)
{
  writer.copy_data(*this);
  return *this;
}

int bitstream_writer::write_bit(int bit)
{
  if (_data == NULL || _data_size == 0 || (size_t)_byte_pos >= _data_size)
  {
    return -1;
  }

  int mask = 1 << (7-_bit_pos);
  if (bit == 1) {
    _data[_byte_pos] |= (mask & 0xff);
  } else {
    mask = 0xff - mask;
    _data[_byte_pos] &= mask;
  }

  _bit_pos++;
  if (_bit_pos > 7) {
    _bit_pos = 0;
    _byte_pos++;
  }
  _bits_written++;
  return 1;
}

int bitstream_writer::write_bits(long long value, int n)
{
  if (_data == NULL || _data_size == 0 || 
      n < 0 || (size_t)n > sizeof(long long)*8)
  {
    return -1;
  }
  if (n == 0) { return 0; }

  for (int i=n-1; i>=0; i--) {
    int result;
    if ((value >> i) & 1) {
      result = write_bit(1);
    } else {
      result = write_bit(0);
    }
    if (result != 1) {
      return i;
    }
  }
  return n;
}

int bitstream_writer::write_byte(int byte)
{
  if (_data == NULL || _data_size == 0 || (size_t)_byte_pos >= _data_size) {
    return -1;
  }

  // move to the next byte
  int nb = 0;
  if (_bit_pos > 0) {
    _bits_written += (8-_bit_pos);
    nb += (8-_bit_pos);
    _bit_pos = 0;
    _byte_pos++;
  }

  _data[_byte_pos] = byte;
  _byte_pos++;
  _bits_written += 8;
  nb += 8;
  return nb;
}

int bitstream_writer::write_remaining_zero()
{
  if (_data == NULL || _data_size == 0 || (size_t)_byte_pos >= _data_size) {
    return -1;
  }
  return write_bits(0, 8-_bit_pos);
}

int bitstream_writer::write_U(int value, int n)
{
  return write_bits(value, n);
}


int bitstream_writer::write_UE(int value)
{
  int bits = 0;
  int cumul = 0;

  for (int i = 0; i < 15; i++) {
    if (value < cumul + (1 << i)) {
      bits = i;
      break;
    }
    cumul += (1 << i);
  }
  int nb = 0;
  nb += write_bits(0, bits);
  nb += write_bit(1);
  nb += write_bits(value - cumul, bits);
  return nb;
}

int bitstream_writer::write_SE(int value)
{
  return write_UE((value << 1) * (value < 0 ? -1 : 1) + (value > 0 ? 1 : 0));
}

int bitstream_writer::write_bool(bool value)
{
  return write_bit(value ? 1 : 0);
}

int bitstream_writer::write_trailing_bits()
{
  int nb = write_bit(1);
  if (nb <= 0) { return nb;}
  return 1 + write_remaining_zero();
}

