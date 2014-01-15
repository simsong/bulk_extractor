/**
 * The code in this file was originally written by Sam Trenholme
 * and was released to the public domain. The original files 
 * are available at:
 * http://www.samiam.org/rijndael.html
 * http://www.samiam.org/key-schedule.html
 * http://www.samiam.org/galois.html
 * and http://www.samiam.org/s-box.html
 *
 * Modifications made by Jesse Kornblum and are also released
 * to the public domain. Summary of modifications:
 * * Modified key schedule code to verify a key schedule,
 *   not just compute them.
 *
 * Additional modifications by Simson Garfinkel for incorporation into
 * bulk_extractor, which is public domain.
 *
 * Here's how it work:
 *
 * You can't recognize AES keys.  You can only recognize scheduled AES
 * keys. The schedule provides the redundancy that the scanner looks
 * for. The scanner basically re-schedules the AES key and then it
 * sees if the memory matches a scheduled key.
 * 
 * We use entropy detection to look for a key schedule. The theory
 * here is that the key scheduling process creates high-entropy
 * data. So there is no reason to examine low-entropy data for a
 * scheduled key, becuase you won't find it.
 */
 

#include "config.h"
#include "be13_api/bulk_extractor_i.h"



/* old aes.h file */


#define AES128_KEY_SIZE                  16 //  Size of a 128-bit AES key, in bytes
#define AES192_KEY_SIZE                  24 // Size of a 192-bit AES key, in bytes
#define AES256_KEY_SIZE                  32 // Size of a 256-bit AES key, in bytes

#define AES128_KEY_SCHEDULE_SIZE        176 // Size of a 128-bit AES key schedule, in bytes
#define AES192_KEY_SCHEDULE_SIZE        208 // Size of a 128-bit AES key schedule, in bytes
#define AES256_KEY_SCHEDULE_SIZE        240 // Size of a 128-bit AES key schedule, in bytes

#define REQUIRED_DISTINCT_COUNTS	10 // number of unique  bytes to require in AES key

#define FALSE 0
#define TRUE  1

// Determines whether or not data represents valid
// AES key schedules. In reality, this is very efficient code for 
// finding blocks of data that are NOT AES key schedules.
//
// Because we are going to encounter blocks of data that are not
// valid key schedules far more often than not, these functions
// have been optimized to find values that are not key schedules.
//
// Returns TRUE if 'in' is a valid 128-bit AES key schedule, otherwise FALSE
int valid_aes128_schedule(const unsigned char * in);
int valid_aes192_schedule(const unsigned char * in);
int valid_aes256_schedule(const unsigned char * in);

/* old aes.c file */

#include <string.h>
#include <inttypes.h>

/* 8 bit x 8 bit group multiplication.
 */
inline unsigned char gmul(unsigned char a, unsigned char b) 
{
    unsigned char p = 0;

    for(u_char counter = 0; counter < 8; counter++) {
	if((b & 1) == 1) p ^= a;
	u_char hi_bit_set = (a & 0x80);
	a <<= 1;
	if(hi_bit_set == 0x80) a ^= 0x1b;		
	b >>= 1;
    }
    return p;
}

/* The rcon function.
 * This function is now solely used to create the rcon table
 */
inline unsigned char rcon_function(unsigned char in) 
{
    unsigned char c=1;

    if(in == 0) return 0; 
    while(in != 1) {
	c = gmul(c,2);
	in--;
    }
    return c;
}

u_char rcon[256];
void rcon_setup()
{
    for(int i=0;i<256;i++){
	rcon[i] = rcon_function(i);
    }
}


// Log table using 0xe5 (229) as the generator 
static unsigned char ltable[256] = {
  0x00, 0xff, 0xc8, 0x08, 0x91, 0x10, 0xd0, 0x36, 
  0x5a, 0x3e, 0xd8, 0x43, 0x99, 0x77, 0xfe, 0x18, 
  0x23, 0x20, 0x07, 0x70, 0xa1, 0x6c, 0x0c, 0x7f, 
  0x62, 0x8b, 0x40, 0x46, 0xc7, 0x4b, 0xe0, 0x0e, 
  0xeb, 0x16, 0xe8, 0xad, 0xcf, 0xcd, 0x39, 0x53, 
  0x6a, 0x27, 0x35, 0x93, 0xd4, 0x4e, 0x48, 0xc3, 
  0x2b, 0x79, 0x54, 0x28, 0x09, 0x78, 0x0f, 0x21, 
  0x90, 0x87, 0x14, 0x2a, 0xa9, 0x9c, 0xd6, 0x74, 
  0xb4, 0x7c, 0xde, 0xed, 0xb1, 0x86, 0x76, 0xa4, 
  0x98, 0xe2, 0x96, 0x8f, 0x02, 0x32, 0x1c, 0xc1, 
  0x33, 0xee, 0xef, 0x81, 0xfd, 0x30, 0x5c, 0x13, 
  0x9d, 0x29, 0x17, 0xc4, 0x11, 0x44, 0x8c, 0x80, 
  0xf3, 0x73, 0x42, 0x1e, 0x1d, 0xb5, 0xf0, 0x12, 
  0xd1, 0x5b, 0x41, 0xa2, 0xd7, 0x2c, 0xe9, 0xd5, 
  0x59, 0xcb, 0x50, 0xa8, 0xdc, 0xfc, 0xf2, 0x56, 
  0x72, 0xa6, 0x65, 0x2f, 0x9f, 0x9b, 0x3d, 0xba, 
  0x7d, 0xc2, 0x45, 0x82, 0xa7, 0x57, 0xb6, 0xa3, 
  0x7a, 0x75, 0x4f, 0xae, 0x3f, 0x37, 0x6d, 0x47, 
  0x61, 0xbe, 0xab, 0xd3, 0x5f, 0xb0, 0x58, 0xaf, 
  0xca, 0x5e, 0xfa, 0x85, 0xe4, 0x4d, 0x8a, 0x05, 
  0xfb, 0x60, 0xb7, 0x7b, 0xb8, 0x26, 0x4a, 0x67, 
  0xc6, 0x1a, 0xf8, 0x69, 0x25, 0xb3, 0xdb, 0xbd, 
  0x66, 0xdd, 0xf1, 0xd2, 0xdf, 0x03, 0x8d, 0x34, 
  0xd9, 0x92, 0x0d, 0x63, 0x55, 0xaa, 0x49, 0xec, 
  0xbc, 0x95, 0x3c, 0x84, 0x0b, 0xf5, 0xe6, 0xe7, 
  0xe5, 0xac, 0x7e, 0x6e, 0xb9, 0xf9, 0xda, 0x8e, 
  0x9a, 0xc9, 0x24, 0xe1, 0x0a, 0x15, 0x6b, 0x3a, 
  0xa0, 0x51, 0xf4, 0xea, 0xb2, 0x97, 0x9e, 0x5d, 
  0x22, 0x88, 0x94, 0xce, 0x19, 0x01, 0x71, 0x4c, 
  0xa5, 0xe3, 0xc5, 0x31, 0xbb, 0xcc, 0x1f, 0x2d, 
  0x3b, 0x52, 0x6f, 0xf6, 0x2e, 0x89, 0xf7, 0xc0, 
  0x68, 0x1b, 0x64, 0x04, 0x06, 0xbf, 0x83, 0x38 };

// Anti-log table:
static unsigned char atable[256] = {
  0x01, 0xe5, 0x4c, 0xb5, 0xfb, 0x9f, 0xfc, 0x12, 
  0x03, 0x34, 0xd4, 0xc4, 0x16, 0xba, 0x1f, 0x36, 
  0x05, 0x5c, 0x67, 0x57, 0x3a, 0xd5, 0x21, 0x5a, 
  0x0f, 0xe4, 0xa9, 0xf9, 0x4e, 0x64, 0x63, 0xee, 
  0x11, 0x37, 0xe0, 0x10, 0xd2, 0xac, 0xa5, 0x29, 
  0x33, 0x59, 0x3b, 0x30, 0x6d, 0xef, 0xf4, 0x7b, 
  0x55, 0xeb, 0x4d, 0x50, 0xb7, 0x2a, 0x07, 0x8d, 
  0xff, 0x26, 0xd7, 0xf0, 0xc2, 0x7e, 0x09, 0x8c, 
  0x1a, 0x6a, 0x62, 0x0b, 0x5d, 0x82, 0x1b, 0x8f, 
  0x2e, 0xbe, 0xa6, 0x1d, 0xe7, 0x9d, 0x2d, 0x8a, 
  0x72, 0xd9, 0xf1, 0x27, 0x32, 0xbc, 0x77, 0x85, 
  0x96, 0x70, 0x08, 0x69, 0x56, 0xdf, 0x99, 0x94, 
  0xa1, 0x90, 0x18, 0xbb, 0xfa, 0x7a, 0xb0, 0xa7, 
  0xf8, 0xab, 0x28, 0xd6, 0x15, 0x8e, 0xcb, 0xf2, 
  0x13, 0xe6, 0x78, 0x61, 0x3f, 0x89, 0x46, 0x0d, 
  0x35, 0x31, 0x88, 0xa3, 0x41, 0x80, 0xca, 0x17, 
  0x5f, 0x53, 0x83, 0xfe, 0xc3, 0x9b, 0x45, 0x39, 
  0xe1, 0xf5, 0x9e, 0x19, 0x5e, 0xb6, 0xcf, 0x4b, 
  0x38, 0x04, 0xb9, 0x2b, 0xe2, 0xc1, 0x4a, 0xdd, 
  0x48, 0x0c, 0xd0, 0x7d, 0x3d, 0x58, 0xde, 0x7c, 
  0xd8, 0x14, 0x6b, 0x87, 0x47, 0xe8, 0x79, 0x84, 
  0x73, 0x3c, 0xbd, 0x92, 0xc9, 0x23, 0x8b, 0x97, 
  0x95, 0x44, 0xdc, 0xad, 0x40, 0x65, 0x86, 0xa2, 
  0xa4, 0xcc, 0x7f, 0xec, 0xc0, 0xaf, 0x91, 0xfd, 
  0xf7, 0x4f, 0x81, 0x2f, 0x5b, 0xea, 0xa8, 0x1c, 
  0x02, 0xd1, 0x98, 0x71, 0xed, 0x25, 0xe3, 0x24, 
  0x06, 0x68, 0xb3, 0x93, 0x2c, 0x6f, 0x3e, 0x6c, 
  0x0a, 0xb8, 0xce, 0xae, 0x74, 0xb1, 0x42, 0xb4, 
  0x1e, 0xd3, 0x49, 0xe9, 0x9c, 0xc8, 0xc6, 0xc7, 
  0x22, 0x6e, 0xdb, 0x20, 0xbf, 0x43, 0x51, 0x52, 
  0x66, 0xb2, 0x76, 0x60, 0xda, 0xc5, 0xf3, 0xf6, 
  0xaa, 0xcd, 0x9a, 0xa0, 0x75, 0x54, 0x0e, 0x01 };


inline unsigned char gmul_inverse(unsigned char in) 
{
    if(in == 0)     return 0;    // 0 is self inverting
    return atable[(255 - ltable[in])];
}


// sbox function is now used only to create the sbox table 

unsigned char sbox[256];
inline unsigned char sbox_function(unsigned char in) 
{
  unsigned char c, s, x;
  s = x = gmul_inverse(in);
  for(c = 0; c < 4; c++)   {
    // One bit circular rotate to the left 
    s = (s << 1) | (s >> 7);
    // xor with x 
    x ^= s;
  }
  x ^= 99; // 0x63 
  return x;
}

void sbox_setup()
{
    for(int i=0;i<256;i++){
	sbox[i] = sbox_function(i);
    }
}


// Rotate a 32-bit word
// We can't make this a table; it would take 4GB!
// but it should be recoded with a rotate left instruction
inline void rotate(unsigned char *in) 
{
    unsigned char in0 = in[0];
    in[0] = in[1];
    in[1] = in[2];
    in[2] = in[3];
    in[3] = in0;
}

// This is the core key expansion, which, given a 4-byte value,
// does some scrambling
inline void schedule_core(unsigned char *in, unsigned char i) 
{
    // Rotate the input 8 bits to the left 
    rotate(in);
    // Apply Rijndael's s-box on all 4 bytes 

    for(uint8_t a = 0; a < 4; a++) {
	in[a] = sbox[in[a]];
    }

    // On just the first byte, add 2^i to the byte 
    in[0] ^= rcon[i];
}


// Returns TRUE if the buffer in contains a valid AES-128 key
// schedule, otherwise, FALSE.
int valid_aes128_schedule(const unsigned char * in)
{
  unsigned char computed[AES128_KEY_SCHEDULE_SIZE];
  unsigned char t[4];

  // c is 16 because the first sub-key is the user-supplied key
  unsigned char pos = AES128_KEY_SIZE;
  unsigned char i = 1;

  memcpy(computed, in, AES128_KEY_SIZE);

  // We need 11 sets of sixteen bytes each for 128-bit mode
  while (pos < AES128_KEY_SCHEDULE_SIZE) {
    // Copy the temporary variable over from the last 4-byte block
    memcpy (t, in + pos - 4, 4);

    // Every four blocks (of four bytes), do a complex calculation 
    if (pos % AES128_KEY_SIZE == 0) {
      schedule_core(t,i);
      i++;
    }

    for (u_char a = 0; a < 4 && pos<AES128_KEY_SCHEDULE_SIZE; a++) {
      computed[pos] = computed[pos - AES128_KEY_SIZE] ^ t[a];

      // If the computed schedule doesn't match our goal,
      // punt immediately!
      if (computed[pos] != in[pos]){
	return FALSE;
      }
      pos++;
    }
  }
  return TRUE;
}


// Returns TRUE if the buffer in contains a valid AES-192 key
// schedule, otherwise, FALSE.
int valid_aes192_schedule(const unsigned char * in)
{
  unsigned char computed[AES192_KEY_SCHEDULE_SIZE];
  unsigned char t[4];

  // c is 24 because the first sub-key is the user-supplied key
  unsigned char pos = AES192_KEY_SIZE;
  unsigned char i = 0;
  unsigned char a;

  memcpy(computed, in, AES192_KEY_SIZE);

  // We need 11 sets of sixteen bytes each for 128-bit mode
  while (pos < AES192_KEY_SCHEDULE_SIZE) {
    // Copy the temporary variable over from the last 4-byte block
    memcpy (t, in + pos - 4, 4);

    // Every six blocks (of four bytes), do a complex calculation 
    if (pos % AES192_KEY_SIZE == 0) {
      schedule_core(t,i);
      i++;
    }

    for (a = 0; a < 4 && pos<AES192_KEY_SCHEDULE_SIZE; a++)     {
      computed[pos] = computed[pos - AES192_KEY_SIZE] ^ t[a];

      // If the computed schedule doesn't match our goal,
      // punt immediately!
      if (computed[pos] != in[pos])
	return FALSE;

      pos++;
    }
  }

  return TRUE;
}



// Returns TRUE if the buffer in contains a valid AES-256 key
// schedule, otherwise, FALSE.
int valid_aes256_schedule(const unsigned char * in)
{
  unsigned char computed[AES256_KEY_SCHEDULE_SIZE];
  unsigned char t[4];

  // c is 32 because the first sub-key is the user-supplied key
  unsigned char pos = AES256_KEY_SIZE;
  unsigned char i = 1;

  memcpy(computed, in, AES256_KEY_SIZE);

  // We need 11 sets of sixteen bytes each for 256-bit mode
  while (pos < AES256_KEY_SCHEDULE_SIZE)   {
    // Copy the temporary variable over from the last 4-byte block
    memcpy (t, in + pos - 4, 4);

    // Every eight blocks (of four bytes), do a complex calculation 
    if (pos % AES256_KEY_SIZE == 0)     {
      schedule_core(t,i);
      i++;
    }

    // For 256-bit keys, we add an extra sbox to the calculation
    if (16 == pos % AES256_KEY_SIZE)    {
      for (u_char a = 0 ; a < 4 ; ++a)
	t[a] = sbox_function(t[a]);
    }

    for (u_char a = 0; a < 4 && pos<AES256_KEY_SCHEDULE_SIZE; a++)     {
      computed[pos] = computed[pos - AES256_KEY_SIZE] ^ t[a];

      // If the computed schedule doesn't match our goal,
      // punt immediately!
      if (computed[pos] != in[pos])
	return FALSE;

      pos++;
    }
  }

  return TRUE;
}
    

// FindAES version 1.0 by Jesse Kornblum
// http://jessekornblum.com/tools/findaes/
// This code is public domain.
// Substantially modified by Simson Garfinkel



static std::string key_to_string(const unsigned char * key, uint64_t sz)
{
    std::string ret;
    for(size_t pos=0;pos<sz;pos++){
	char buf[4];
	snprintf(buf,sizeof(buf),"%02x ", key[pos]);
	ret += buf;
    }
    return ret;
}

#define BUFFER_SIZE  10485760
#define WINDOW_SIZE  AES256_KEY_SCHEDULE_SIZE

extern "C"
void scan_aes(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "aes";
	sp.info->author		= "Sam Trenholme, Jesse Kornblum and Simson Garfinkel";
	sp.info->description    = "Search for AES key schedules";
	sp.info->feature_names.insert("aes_keys");

	rcon_setup();
	sbox_setup();

	return;
    }

    /* We don't need to check for phase 2 of if sbuf isn't big enough to hold a KEY_SCHEDULE
     */

    if(sp.phase==scanner_params::PHASE_SCAN && sp.sbuf.bufsize >= WINDOW_SIZE){
	feature_recorder *aes_recorder = sp.fs.get_name("aes_keys");

	/* Simple mod: Keep a rolling window of the entropy and don't
	 * we see fewer than 10 distinct characters in window. This will
	 * eliminate checks on many kinds of bulk data that simply can't have a key
	 * in the block.
	 */
	uint32_t counts[256];
	memset(counts,0,sizeof(counts));
	int32_t distinct_counts = 0;	// how many distinct counts do we have?

	/* Initialize the sliding window */
	for (size_t pos = 0; pos < WINDOW_SIZE ; pos++)	{
	    const u_char val = sp.sbuf[pos];
	    counts[val]++;
	    if (counts[val] == 1) {
		distinct_counts++;
	    }
	}
	for (size_t pos = 0 ;
	     pos < sp.sbuf.bufsize-WINDOW_SIZE && pos < sp.sbuf.pagesize;
	     pos++){


	    /* add value at end of 128 bits to sliding window */
	    {
		const u_char val = sp.sbuf[pos+AES256_KEY_SCHEDULE_SIZE];
		counts[val]++;
		if(counts[val]==1) {		// we have one more distinct count
		    distinct_counts++;
		}
	    }

	    if(distinct_counts>10){
		const u_char *p2 = sp.sbuf.buf + pos;
		if (valid_aes128_schedule(p2)) {
                    std::string key = key_to_string(p2, AES128_KEY_SIZE);
		    aes_recorder->write(sp.sbuf.pos0+pos,key,std::string("AES128"));
		}
		if (valid_aes192_schedule(p2)) {
                    std::string key = key_to_string(p2, AES192_KEY_SIZE);
		    aes_recorder->write(sp.sbuf.pos0+pos,key,std::string("AES192"));
		}
		if (valid_aes256_schedule(p2)) {
                    std::string key = key_to_string(p2, AES256_KEY_SIZE);
		    aes_recorder->write(sp.sbuf.pos0+pos,key,std::string("AES256"));
		}
	    }
	    /* remove current byte being analyzed */
	    if(pos>WINDOW_SIZE){
		const u_char val = sp.sbuf[pos];
		counts[val]--;
		if(counts[val]==0){		
		    distinct_counts--;	// we have one fewer
		}
		assert(distinct_counts>0);	// we must have at least one distinct count...
	    }
	}
    }
}

