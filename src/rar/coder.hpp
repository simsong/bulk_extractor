/****************************************************************************
 *  Contents: 'Carryless rangecoder' by Dmitry Subbotin                     *
 ****************************************************************************/
#ifndef _CODER_HPP_
#define _CODER_HPP_

class Unpack;

// (int) cast before "low" added only to suppress compiler warnings.
#define ARI_DEC_NORMALIZE(code,low,range,read)                           \
{                                                                        \
  while ((low^(low+range))<TOP || (range<BOT && ((range=-(int)low&(BOT-1)),1))) \
  {                                                                      \
    code=(code << 8) | read->GetChar();                                  \
    range <<= 8;                                                         \
    low <<= 8;                                                           \
  }                                                                      \
}

const uint TOP=1 << 24, BOT=1 << 15;

class RangeCoder
{
  public:
    void InitDecoder(Unpack *UnpackRead);
    int GetCurrentCount();
    uint GetCurrentShiftCount(uint SHIFT);
    void Decode();
    void PutChar(unsigned int c);
    unsigned int GetChar();

    uint low, code, range;
    struct SUBRANGE 
    {
      uint LowCount, HighCount, scale;
    } SubRange;

    Unpack *UnpackRead;
};
#endif
