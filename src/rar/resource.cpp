#include "rar.hpp"



#ifndef RARDLL
const char *St(MSGID StringId)
{
  return(StringId);
}
#endif


#if 0
#ifndef RARDLL
const wchar *StW(MSGID StringId)
{
  wchar StrTable[8][512];
  unsigned StrNum=0;
  if (++StrNum >= sizeof(StrTable)/sizeof(StrTable[0]))
    StrNum=0;
  wchar *Str=StrTable[StrNum];
  *Str=0;
  CharToWide(StringId,Str,ASIZE(StrTable[0]));
  return(Str);
}
#endif
#endif


