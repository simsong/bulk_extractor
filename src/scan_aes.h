#ifndef SCAN_AES_H
#define SCAN_AES_H
bool valid_aes128_schedule(const uint8_t * in);
bool valid_aes192_schedule(const uint8_t * in);
bool valid_aes256_schedule(const uint8_t * in);

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
