#include "rar.hpp"

bool ReadTextFile(
  const char *Name,
  const wchar *NameW,
  StringList *List,
  bool Config,
  bool AbortOnError,
  RAR_CHARSET SrcCharset,
  bool Unquote,
  bool SkipComments,
  bool ExpandEnvStr)
{
    return false;
}


bool IsUnicode(byte *Data,int Size)
{
  if (Size<4 || Data[0]!=0xff || Data[1]!=0xfe)
    return(false);
  for (int I=2;I<Size;I++)
    if (Data[I]<32 && Data[I]!='\r' && Data[I]!='\n')
      return(true);
  return(false);
}
