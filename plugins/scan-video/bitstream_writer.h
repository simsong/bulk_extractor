#ifndef BITSTREAM_WRITER_H
#define BITSTREAM_WRITER_H

class bitstream_writer
{
  public:
    bitstream_writer();
    bitstream_writer(const bitstream_writer& writer);
    bitstream_writer(char* data, size_t size);
    ~bitstream_writer();

    void set_data(char* data, size_t size);
    void copy_data(bitstream_writer& writer) const;

    bitstream_writer& operator=(const bitstream_writer& writer);

    int write_bit(int bit);
    int write_bits(long long value, int n);
    int write_byte(int byte);

    int write_remaining_zero();
    int write_U(int value, int n);
    int write_UE(int value);
    int write_SE(int value);
    int write_bool(bool value);
    int write_trailing_bits();

    size_t write_count() const 
    { return _bit_pos > 0 ? _byte_pos + 1 : _byte_pos; };

    size_t write_bit_count() const
    { return _bits_written; }

  protected:
    char* _data;
    size_t _data_size;

    int _bit_pos;
    size_t _byte_pos;

    size_t _bits_written;
};

#endif
