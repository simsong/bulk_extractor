#include <iostream>
#include <string.h>

#include "bytestream_reader.h"

using namespace std;

bytestream_reader::bytestream_reader()
  :_data(NULL),_data_size(0),_pos(0)
{
}

bytestream_reader::bytestream_reader(const char* data, size_t size)
  :_data(NULL),_data_size(0),_pos(0)
{
  set_data(data, size);
}

bytestream_reader::bytestream_reader(const bytestream_reader& reader)
  :_data(NULL),_data_size(0),_pos(0)
{
  reader.copy_data(*this);
}

bytestream_reader::~bytestream_reader()
{
  if (_data) {
    delete[] _data;
  }
}

void bytestream_reader::set_data(const char* data, size_t size)
{
  if (data && size > 0) {
    _data = new char[size];
    memcpy(_data, data, size);
    _data_size = size;
    _pos = 0;
  }
}

void bytestream_reader::copy_data(bytestream_reader& reader) const
{
  reader.set_data(_data, _data_size);
}

bytestream_reader& bytestream_reader::operator=(const bytestream_reader& reader)
{
  reader.copy_data(*this);
  return *this;
}

int bytestream_reader::read_byte()
{
  if (_data == NULL || _data_size == 0 || _pos >= _data_size) {
    cerr << "*** Error reading byte at offset: " << _pos 
         << ", total: " << _data_size << endl;
    return -1;
  }
  int value = _data[_pos];
  _pos++;
  return value;
}

long long bytestream_reader::read_bytes(int n)
{
  if (_data == NULL || _data_size == 0 || n < 0 || 
      (size_t)n > sizeof(long long) || n + _pos > _data_size) 
  {
    cerr << "*** Error reading bytes at offset: " << _pos 
         << ", total: " << _data_size << endl;
    return -1;
  }
  if (n == 0) { return 0; }

  char* buf = _data + _pos;
  long long v = 0;
  for (int i=0; i<n; i++) {
    v <<= 8;
    v |= (buf[i] & 0xff);
  }
  _pos += n;
  return v;
}

int bytestream_reader::read_array(char* dest, int n)
{
  if (_data == NULL || _data_size == 0 || n < 0 || dest == NULL ||
      n + _pos > _data_size) 
  {
    cerr << "*** Error reading array at offset: " << _pos 
         << ", total: " << _data_size << endl;
    return -1;
  }
  if (n == 0) { return 0; }

  memcpy(dest, _data+_pos, n);
  _pos += n;
  return n;
}

std::string* bytestream_reader::read_fourcc() 
{
  if (_data == NULL || _data_size == 0 || _pos + 4 > _data_size) 
  {
    cerr << "*** Error reading fourcc code at offset: " << _pos 
         << ", total: " << _data_size << endl;
    return NULL;
  }
  char fourcc[5] = {0};
  memcpy(fourcc, _data+_pos, 4);
  _pos += 4;
  return new std::string(fourcc);
}

