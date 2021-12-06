#ifndef SCAN_AES_H
#define SCAN_AES_H
bool valid_aes128_schedule(const uint8_t * in);
bool valid_aes192_schedule(const uint8_t * in);
bool valid_aes256_schedule(const uint8_t * in);
void create_aes128_schedule(const uint8_t * key, uint8_t computed[176]);
std::string key_to_string(const uint8_t * key, uint64_t sz);

// https://tinyurl.com/u9p944uu
// Try this one day...
#if 0
typedef uint32_t rotwidth_t;
inline rotwidth_t rotl (rotwidth_t x, unsigned int n)
{
  const decltype(n) mask = (CHAR_BIT*sizeof(x) - 1);
  // just use unsigned int for the count mask unless you're experimenting with compiler behaviour

  assert (n<=mask && "rotate by more than type width");
  n &= mask;  // avoid undef behaviour with NDEBUG.
  return (x<<n) | (x>>( (-n)&mask ));
}

inline void rotate32x8(uint8_t *in)
{
    uint32_t *in4 = reintrepret_cast<uint32_t *>(in);
    *in4 = rotl(*in4, 8);
}
#else

// Rotate a 32-bit word
// We can't make this a table; it would take 4GB!
// but it should be recoded with a rotate left instruction
inline void rotate32x8(uint8_t *in)
{
    uint8_t in0 = in[0];
    in[0] = in[1];
    in[1] = in[2];
    in[2] = in[3];
    in[3] = in0;
}
#endif
#endif
