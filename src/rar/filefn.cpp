#include <stdlib.h>
#include "raros.hpp"
#include "os.hpp"
#include "rartypes.hpp"
#include "timefn.hpp"
#include "file.hpp"
#include "unicode.hpp"
#include "pathfn.hpp"
#include "savepos.hpp"
#include "errhnd.hpp"
#include "rardefs.hpp"
#include "array.hpp"
#include "system.hpp"
#include "crc.hpp"

#include "filefn.hpp"

MKDIR_CODE MakeDir(const char *Name,const wchar *NameW,bool SetAttr,uint Attr)
{
    return MKDIR_SUCCESS;
}


bool CreatePath(const char *Path,bool SkipLastName)
{
    return true;
}


bool CreatePath(const wchar *Path,bool SkipLastName)
{
    return true;
}


bool CreatePath(const char *Path,const wchar *PathW,bool SkipLastName)
{
    return true;
}


void SetDirTime(const char *Name,const wchar *NameW,RarTime *ftm,RarTime *ftc,RarTime *fta)
{
}


bool IsRemovable(const char *Name)
{
    return false;
}




#ifndef SFX_MODULE
int64 GetFreeDisk(const char *Name)
{
    return 0;
}
#endif





bool FileExist(const char *Name,const wchar *NameW)
{
    return true;
}


bool FileExist(const wchar *Name)
{
    return true;
}
 

bool WildFileExist(const char *Name,const wchar *NameW)
{
    return true;
}


bool IsDir(uint Attr)
{
#if defined (_WIN_ALL) || defined(_EMX)
  return(Attr!=0xffffffff && (Attr & 0x10)!=0);
#endif
#if defined(_UNIX)
  return((Attr & 0xF000)==0x4000);
#endif
}


bool IsUnreadable(uint Attr)
{
#if defined(_UNIX) && defined(S_ISFIFO) && defined(S_ISSOCK) && defined(S_ISCHR)
  return(S_ISFIFO(Attr) || S_ISSOCK(Attr) || S_ISCHR(Attr));
#endif
  return(false);
}


bool IsLabel(uint Attr)
{
#if defined (_WIN_ALL) || defined(_EMX)
  return((Attr & 8)!=0);
#else
  return(false);
#endif
}


bool IsLink(uint Attr)
{
#ifdef _UNIX
  return((Attr & 0xF000)==0xA000);
#else
  return(false);
#endif
}






bool IsDeleteAllowed(uint FileAttr)
{
#if defined(_WIN_ALL) || defined(_EMX)
  return((FileAttr & (FA_RDONLY|FA_SYSTEM|FA_HIDDEN))==0);
#else
  return((FileAttr & (S_IRUSR|S_IWUSR))==(S_IRUSR|S_IWUSR));
#endif
}


void PrepareToDelete(const char *Name,const wchar *NameW)
{
}


uint GetFileAttr(const char *Name,const wchar *NameW)
{
    return 0;
}


bool SetFileAttr(const char *Name,const wchar *NameW,uint Attr)
{
    return true;
}




#ifndef SFX_MODULE
uint CalcFileCRC(File *SrcFile,int64 Size,CALCCRC_SHOWMODE ShowMode)
{
    return 0;
}
#endif


bool RenameFile(const char *SrcName,const wchar *SrcNameW,const char *DestName,const wchar *DestNameW)
{
    return true;
}


bool DelFile(const char *Name)
{
    return true;
}




bool DelFile(const char *Name,const wchar *NameW)
{
    return true;
}








#if defined(_WIN_ALL) && !defined(_WIN_CE) && !defined(SFX_MODULE)
bool SetFileCompression(char *Name,wchar *NameW,bool State)
{
    return true;
}
#endif









