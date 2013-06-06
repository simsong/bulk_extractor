#include "rar.hpp"

/**
Constructor function for the <code>RawRead</code> class
@param SrcFile - the <code>File</code> object that this class will read from
*/
RawRead::RawRead(File *SrcFile_) :
    Data(), SrcFile(), DataSize(), ReadPos()
{
  RawRead::SrcFile=SrcFile_;
  ReadPos=0;
  DataSize=0;
}

RawRead::RawRead(const RawRead &copy) :
    Data(), SrcFile(), DataSize(), ReadPos()
{
    *this = copy;
}

const RawRead& RawRead::operator=(const RawRead &src)
{
    Data = src.Data;
    SrcFile = src.SrcFile;
    DataSize = src.DataSize;
    ReadPos = src.ReadPos;

    return *this;
}

/*void RawRead::RawRead (File *SrcFile)
{
  RawRead::SrcFile=SrcFile;
  ReadPos=0;
  DataSize=0;
#ifndef SHELL_EXT
  Crypt=NULL;
#endif
}*/


void RawRead::Read(size_t Size_)
{
    if (Size_!=0)
    {
      Data.Add(Size_);
      DataSize+=SrcFile->Read(&Data[DataSize],Size_);
    }
}


void RawRead::Read(byte *SrcData,size_t Size_)
{
  if (Size_!=0)
  {
    Data.Add(Size_);
    memcpy(&Data[DataSize],SrcData,Size_);
    DataSize+=Size_;
  }
}


void RawRead::Get(byte &Field)
{
  if (ReadPos<DataSize)
  {
    Field=Data[ReadPos];
    ReadPos++;
  }
  else
    Field=0;
}


void RawRead::Get(ushort &Field)
{
  if (ReadPos+1<DataSize)
  {
    Field=Data[ReadPos]+(Data[ReadPos+1]<<8);
    ReadPos+=2;
  }
  else
    Field=0;
}


void RawRead::Get(uint &Field)
{
  if (ReadPos+3<DataSize)
  {
    Field=Data[ReadPos]+(Data[ReadPos+1]<<8)+(Data[ReadPos+2]<<16)+
          (Data[ReadPos+3]<<24);
    ReadPos+=4;
  }
  else
    Field=0;
}


void RawRead::Get8(int64 &Field)
{
  uint Low,High;
  Get(Low);
  Get(High);
  Field=INT32TO64(High,Low);
}


void RawRead::Get(byte *Field,size_t Size_)
{
  if (ReadPos+Size_-1<DataSize)
  {
    memcpy(Field,&Data[ReadPos],Size_);
    ReadPos+=Size_;
  }
  else
    memset(Field,0,Size_);
}


void RawRead::Get(wchar *Field,size_t Size_)
{
  if (ReadPos+2*Size_-1<DataSize)
  {
    RawToWide(&Data[ReadPos],Field,Size_);
    ReadPos+=sizeof(wchar)*Size_;
  }
  else
    memset(Field,0,sizeof(wchar)*Size_);
}


uint RawRead::GetCRC(bool ProcessedOnly)
{
  return(DataSize>2 ? CRC(0xffffffff,&Data[2],(ProcessedOnly ? ReadPos:DataSize)-2):0xffffffff);
}
