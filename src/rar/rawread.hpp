#ifndef _RAR_RAWREAD_
#define _RAR_RAWREAD_

/**
This class performs different types of read functions from the <code>File</code> class
The functions should be self-explanatory.
*/
class RawRead
{
  private:
    Array<byte> Data;
    File *SrcFile;
    size_t DataSize;
    size_t ReadPos;
  public:
    RawRead() : Data(), SrcFile(), DataSize(), ReadPos() { };
    RawRead(File *SrcFile);
    RawRead(const RawRead &copy);
    const RawRead& operator=(const RawRead &src);
    void Read(size_t Size);
    void Read(byte *SrcData,size_t Size);
    void Get(byte &Field);
    void Get(ushort &Field);
    void Get(uint &Field);
    void Get8(int64 &Field);
    void Get(byte *Field,size_t Size);
    void Get(wchar *Field,size_t Size);
    uint GetCRC(bool ProcessedOnly);
    size_t Size() {return DataSize;}
    size_t PaddedSize() {return Data.Size()-DataSize;}
};

#endif
