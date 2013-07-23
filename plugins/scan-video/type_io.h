#ifndef _TYPE_IO_H_
#define _TYPE_IO_H_

#include <stdio.h>
#include <string.h>

class type_io 
{
  public:
    inline static size_t write_string(const std::string& s, size_t n, char* buf)
    {
      if (! buf || n <= 1) { return 0; };
      write_uint8(static_cast<unsigned>(s.length()), buf);
      size_t nb = 1 + s.copy(buf+1, n, 0);
      if (nb < n) {
        memset(buf+nb, 0, (n-nb));
      }
      return n;
    }

    inline static size_t write_fourcc(const std::string& s, char* buf) 
    {
      if (! buf) { return 0; }
      size_t nb = s.copy(buf, 4, 0);
      if (nb < 4) {
        memset(buf + nb, 0, (4-nb));
      }
      return 4;
    }

    inline static std::string* read_fourcc(char* buf) 
    {
      if (! buf) { return NULL; }
      char fourcc[5] = {0};
      memcpy(fourcc, buf, 4);
      return new std::string(fourcc);
    }

    inline static size_t write_bytes(const char* data, size_t size, char* buf)
    {
      if (! data || size == 0 || ! buf) {
        return 0;
      }
      memcpy(buf, data, size);
      return size;
    }

    inline static unsigned long read_unsigned(char* buf, int size = 1)
    {
      if (! buf) { return 0; }

      unsigned long result = 0;
      for (int i=0; i<size; i++) {
        result <<= 8;
        result |= (buf[i] & 0xff);
      }
      return result;
    }

    inline static size_t write_unsigned(long v, char* buf, int size = 1)
    {
      if (! buf) { return 0; }

      for (int i=0; i<size; i++) {
        int shift = (size-i-1) << 3;
        buf[i] = static_cast<char>((v >> shift) & 0xff);
      }
      return size;
    }

    inline static unsigned read_uint8(char* buf)
    {
      return (unsigned) read_unsigned(buf, 1);
    };

    inline static unsigned read_uint16(char* buf)
    {
      return (unsigned) read_unsigned(buf, 2);
    }

    inline static unsigned read_uint24(char* buf)
    {
      return (unsigned) read_unsigned(buf, 3);
    }

    inline static unsigned read_uint32(char* buf)
    {
      return (unsigned) read_unsigned(buf, 4);
    }

    inline static unsigned long read_uint64(char* buf)
    {
      return read_unsigned(buf, 8);
    }

    inline static size_t write_uint8(unsigned v, char* buf)
    {
      return write_unsigned(v, buf, 1);
    }

    inline static size_t write_uint16(unsigned v, char* buf)
    {
      return write_unsigned(v, buf, 2);
    }

    inline static size_t write_uint24(unsigned v, char* buf)
    {
      return write_unsigned(v, buf, 3);
    }

    inline static size_t write_uint32(unsigned v, char* buf)
    {
      return write_unsigned(v, buf, 4);
    }

    inline static size_t write_uint64(unsigned long v, char* buf)
    {
      return write_unsigned(v, buf, 8);
    }

    static void print_hex(char* buf, int len) {
      int cnt = 0;
      while (cnt < len) {
        int rem = len - cnt;
        int n = rem > 16 ? 16 : rem;

        for (int i=0; i<n; i++) {
          printf("%.2X ", buf[cnt] & 0xff);
          cnt++;
        }
        printf("\n");
      }
    }
};

#endif
