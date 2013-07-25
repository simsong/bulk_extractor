#ifndef BITSTREAM_READER_H
#define BITSTREAM_READER_H

class bitstream_reader
{
  public:
    bitstream_reader();
    bitstream_reader(const bitstream_reader& reader);
    bitstream_reader(const char* data, size_t size);
    ~bitstream_reader();

    void set_data(const char* data, size_t size);
    void copy_data(bitstream_reader& reader) const;

    bitstream_reader& operator=(const bitstream_reader& reader);

    int read_bit();
    long long read_bits(int n);

    // discard unread partial bits and read the next byte
    int read_byte();

    // read partial bytes in the current byte position
    int read_remaining_byte();

    int read_UE();
    int read_SE();
    bool read_bool();
    int read_U(int i);
    int read_TE(int max);

    char* read_bytes(int size);

    int read_zero_bit_count();

    void read_trailing_bits();

    size_t read_count() const 
    { return _bit_pos > 0 ? _byte_pos + 1 : _byte_pos; };

    size_t read_bit_count() const
    { return _bits_read; }

    bool more_rbsp_data();

  protected:
    char* _data;
    size_t _data_size;

    int _bit_pos;
    size_t _byte_pos;

    size_t _bits_read;
};

#endif
