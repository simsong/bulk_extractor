#include "rar.hpp"

// Buffer size for all volumes involved.
static __thread const size_t TotalBufferSize=0x4000000;

RecVolumes::RecVolumes()
{
  Buf.Alloc(TotalBufferSize);
  memset(SrcFile,0,sizeof(SrcFile));
}


RecVolumes::~RecVolumes()
{
  for (int I=0;I<sizeof(SrcFile)/sizeof(SrcFile[0]);I++)
    delete SrcFile[I];
}




bool RecVolumes::Restore(RAROptions *Cmd,const char *Name,
                         const wchar *NameW,bool Silent)
{
  return(true);
}
