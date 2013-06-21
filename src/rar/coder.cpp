#include "rar.hpp"

unsigned int RangeCoder::GetChar()
{
  return(UnpackRead->GetChar());
}


void RangeCoder::InitDecoder(Unpack *UnpackRead_)
{
  RangeCoder::UnpackRead=UnpackRead_;

  low=code=0;
  range=uint(-1);
  for (int i=0;i < 4;i++)
    code=(code << 8) | GetChar();
}



int RangeCoder::GetCurrentCount() 
{
  return (code-low)/(range /= SubRange.scale);
}


uint RangeCoder::GetCurrentShiftCount(uint SHIFT) 
{
  return (code-low)/(range >>= SHIFT);
}


void RangeCoder::Decode()
{
  low += range*SubRange.LowCount;
  range *= SubRange.HighCount-SubRange.LowCount;
}
