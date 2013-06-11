#include "rar.hpp"



#ifndef RARDLL
const char *St(MSGID StringId)
{
  return(StringId);
}
#endif


#ifndef RARDLL
const wchar *StW(MSGID StringId)
{
  static __thread wchar StrTable[8][512];
  static __thread unsigned StrNum=0;
  if (++StrNum >= sizeof(StrTable)/sizeof(StrTable[0]))
    StrNum=0;
  wchar *Str=StrTable[StrNum];
  *Str=0;
  CharToWide(StringId,Str,ASIZE(StrTable[0]));
  return(Str);
}
#endif


