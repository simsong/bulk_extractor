#ifndef BYTESTREAM_READER_H
#define BYTESTREAM_READER_H

class bytestream_reader
{
  public:
    bytestream_reader();
    bytestream_reader(const bytestream_reader& reader);
    bytestream_reader(const char* data, size_t size);
    ~bytestream_reader();

    void set_data(const char* data, size_t size);
    void copy_data(bytestream_reader& reader) const;

    bytestream_reader& operator=(const bytestream_reader& reader);

    int read_byte();
    long long read_bytes(int n);

    inline int read_uint8() { return (int) read_bytes(1); }
    inline int read_uint16() { return (int) read_bytes(2); }
    inline long read_uint32() { return (long) read_bytes(4); }
    inline long long read_uint64() { return read_bytes(8); }

    std::string* read_fourcc();

    int read_array(char* dest, int n);

    inline size_t read_count() const { return _pos; }
    inline size_t available() const { return _data_size - _pos; }
    inline void skip(int n) { if (n > 0) { _pos += n; } }

  protected:
    char* _data;
    size_t _data_size;
    size_t _pos;
};

#endif
