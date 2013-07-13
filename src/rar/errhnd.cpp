#include "rar.hpp"

//static const bool UserBreak;

ErrorHandler::ErrorHandler() :
    ExitCode(), ErrCount(), EnableBreak(), Silent(), DoShutdown()
{
  Clean();
}


void ErrorHandler::Clean()
{
  ExitCode=SUCCESS;
  ErrCount=0;
  EnableBreak=true;
  Silent=false;
  DoShutdown=false;
}


__attribute__ ((noreturn)) void ErrorHandler::MemoryError()
{
  MemoryErrorMsg();
  Throw(MEMORY_ERROR);
  // hack to quiet GCC about noreturn.  it was unhappy both with and without
  // noreturn before
  exit(1);
}


void ErrorHandler::OpenError(const char *FileName,const wchar *FileNameW)
{
}


void ErrorHandler::CloseError(const char *FileName,const wchar *FileNameW)
{
}


void ErrorHandler::ReadError(const char *FileName,const wchar *FileNameW)
{
}


bool ErrorHandler::AskRepeatRead(const char *FileName,const wchar *FileNameW)
{
  return(false);
}


void ErrorHandler::WriteError(const char *ArcName,const wchar *ArcNameW,const char *FileName,const wchar *FileNameW)
{
}


#ifdef _WIN_ALL
void ErrorHandler::WriteErrorFAT(const char *FileName,const wchar *FileNameW)
{
}
#endif


bool ErrorHandler::AskRepeatWrite(const char *FileName,const wchar *FileNameW,bool DiskFull)
{
  return(false);
}


void ErrorHandler::SeekError(const char *FileName,const wchar *FileNameW)
{
}


void ErrorHandler::GeneralErrMsg(const char *Msg)
{
}


void ErrorHandler::MemoryErrorMsg()
{
}


void ErrorHandler::OpenErrorMsg(const char *FileName,const wchar *FileNameW)
{
  OpenErrorMsg(NULL,NULL,FileName,FileNameW);
}


void ErrorHandler::OpenErrorMsg(const char *ArcName,const wchar *ArcNameW,const char *FileName,const wchar *FileNameW)
{
}


void ErrorHandler::CreateErrorMsg(const char *FileName,const wchar *FileNameW)
{
  CreateErrorMsg(NULL,NULL,FileName,FileNameW);
}


void ErrorHandler::CreateErrorMsg(const char *ArcName,const wchar *ArcNameW,const char *FileName,const wchar *FileNameW)
{
}


// Check the path length and display the error message if it is too long.
void ErrorHandler::CheckLongPathErrMsg(const char *FileName,const wchar *FileNameW)
{
#if defined(_WIN_ALL) && !defined(_WIN_CE) && !defined (SILENT) && defined(MAX_PATH)
  if (GetLastError()==ERROR_PATH_NOT_FOUND)
  {
    wchar WideFileName[NM];
    GetWideName(FileName,FileNameW,WideFileName,ASIZE(WideFileName));
    size_t NameLength=wcslen(WideFileName);
    if (!IsFullPath(WideFileName))
    {
      wchar CurDir[NM];
      GetCurrentDirectoryW(ASIZE(CurDir),CurDir);
      NameLength+=wcslen(CurDir)+1;
    }
  }
#endif
}


void ErrorHandler::ReadErrorMsg(const char *ArcName,const wchar *ArcNameW,const char *FileName,const wchar *FileNameW)
{
#ifndef SILENT
  ErrMsg(ArcName,St(MErrRead),FileName);
  SysErrMsg();
#endif
}


void ErrorHandler::WriteErrorMsg(const char *ArcName,const wchar *ArcNameW,const char *FileName,const wchar *FileNameW)
{
#ifndef SILENT
  ErrMsg(ArcName,St(MErrWrite),FileName);
  SysErrMsg();
#endif
}


void ErrorHandler::Exit(int ExitCode_)
{
  Throw(ExitCode_);
}


#ifndef GUI
void ErrorHandler::ErrMsg(const char *ArcName,const char *fmt,...)
{
  safebuf char Msg[NM+1024];
  va_list argptr;
  va_start(argptr,fmt);
  vsprintf(Msg,fmt,argptr);
  va_end(argptr);
  if (*Msg)
  {
  }
}
#endif


void ErrorHandler::SetErrorCode(int Code)
{
  switch(Code)
  {
    case WARNING:
    case USER_BREAK:
      if (ExitCode==SUCCESS)
        ExitCode=Code;
      break;
    case FATAL_ERROR:
      if (ExitCode==SUCCESS || ExitCode==WARNING)
        ExitCode=FATAL_ERROR;
      break;
    default:
      ExitCode=Code;
      break;
  }
  ErrCount++;
}

void ErrorHandler::Throw(int Code)
{
  if (Code==USER_BREAK && !EnableBreak)
    return;
  ErrHandler.SetErrorCode(Code);
#ifdef ALLOW_EXCEPTIONS
  throw Code;
#else
  File::RemoveCreated();
  exit(Code);
#endif
}


void ErrorHandler::SysErrMsg()
{
#if !defined(SFX_MODULE) && !defined(SILENT)
#ifdef _WIN_ALL
  wchar *lpMsgBuf=NULL;
  int ErrType=GetLastError();
  if (ErrType!=0 && FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
              NULL,ErrType,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
              (LPTSTR)&lpMsgBuf,0,NULL))
  {
    wchar *CurMsg=lpMsgBuf;
    while (CurMsg!=NULL)
    {
      while (*CurMsg=='\r' || *CurMsg=='\n')
        CurMsg++;
      if (*CurMsg==0)
        break;
      wchar *EndMsg=wcschr(CurMsg,'\r');
      if (EndMsg==NULL)
        EndMsg=wcschr(CurMsg,'\n');
      if (EndMsg!=NULL)
      {
        *EndMsg=0;
        EndMsg++;
      }
      // We use ASCII for output in Windows console, so let's convert Unicode
      // message to single byte.
      size_t Length=wcslen(CurMsg)*2; // Must be enough for DBCS characters.
      char *MsgA=(char *)malloc(Length+2);
      if (MsgA!=NULL)
      {
        WideToChar(CurMsg,MsgA,Length+1);
        MsgA[Length]=0;
        free(MsgA);
      }
      CurMsg=EndMsg;
    }
  }
  LocalFree( lpMsgBuf );
#endif

#endif
}




