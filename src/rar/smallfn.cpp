#include "rar.hpp"

int ToPercent(int64 N1_,int64 N2_)
{
  if (N2_<N1_)
    return(100);
  return(ToPercentUnlim(N1_,N2_));
}


// Allows the percent larger than 100.
int ToPercentUnlim(int64 N1_,int64 N2_)
{
  if (N2_==0)
    return(0);
  return((int)(N1_*100/N2_));
}


void RARInitData()
{
  InitCRC();
  ErrHandler.Clean();
}


#ifdef _DJGPP
// Disable wildcard expansion in DJGPP DOS SFX module.
extern "C" char **__crt0_glob_function (char *arg) 
{ 
  return 0; 
}
#endif


#ifdef _DJGPP
// Disable environments variable loading in DJGPP DOS SFX module
// to reduce the module size.
extern "C" void __crt0_load_environment_file (char *progname) 
{ 
}
#endif
