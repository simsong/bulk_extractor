#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <stdarg.h>
#include "string.h"
#include "rartypes.hpp"
#include "rardefs.hpp"
#include "system.hpp"
#include "timefn.hpp"
#include "errhnd.hpp"
#include "strfn.hpp"
#include "os.hpp"
#include "array.hpp"
#include "global.hpp"

#include "file.hpp"

static const File *CreatedFiles[256];
//char *_i64toa( int64 value, char *string, int radix );
/**
File constructor - After this is called, one <b>must</b> call the <code>InitFile(...)</code> function
*/
File::File() :
    ptrlocation(), initptrlocation(), ptrlength(), hFile(), LastWrite(),
    HandleType(), SkipClose(), IgnoreReadErrors(), NewFile(), AllowDelete(),
    AllowExceptions(), OpenShared(), ErrorType(), CloseCount()
{ //Constructor
  hFile=BAD_HANDLE;
  *FileName=0;
  *FileNameW=0;
  NewFile=false;
  LastWrite=false;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  IgnoreReadErrors=false;
  ErrorType=FILE_SUCCESS;
  OpenShared=false;
  AllowDelete=true;
  CloseCount=0;
  AllowExceptions=true;
  initptrlocation = 0; //set the pointer location to null. The Init function will set this.
  ptrlocation = 0; //set the pointer location to null. The Init function will set this.
#ifdef _WIN_ALL
  NoSequentialRead=false;
#endif
}

File::File(const File &copy) :
    ptrlocation(), initptrlocation(), ptrlength(), hFile(), LastWrite(),
    HandleType(), SkipClose(), IgnoreReadErrors(), NewFile(), AllowDelete(),
    AllowExceptions(), OpenShared(), ErrorType(), CloseCount()
{
    *this = copy;
}


File::~File()
{//Destructor class
    if (hFile!=BAD_HANDLE && !SkipClose)
    {
        if (NewFile)
        {
            Delete();
        }
        else
        {
            Close();
        }
    }
}

/**
This function initializes the file class to read information from memory.

@param ptr - a pointer to the location in memory where the RAR file lives
@param length - The length of the location in memory where the RAR file lives
*/
void File::InitFile(void* ptr, int64 length)
{//initialize the data to point to the RAR pointer in memory
	//set the pointer to the right place so we can read from it later on.
	//also, store the initial location in case we have to go back there later on.
	initptrlocation = (byte*)ptr; //initialize the initial pointer location
	ptrlocation = (byte*)ptr; //initialize the current pointer location
	ptrlength = length; //set the length of the file
	return; 
}

/**
This function is not called in bulk_extractor
*/
const File& File::operator=(const File &SrcFile)
{//This is not called in bulk_extractor
  hFile=SrcFile.hFile;
  strcpy(FileName,SrcFile.FileName);
  NewFile=SrcFile.NewFile;
  LastWrite=SrcFile.LastWrite;
  HandleType=SrcFile.HandleType;
  //SrcFile.SkipClose=true;
  initptrlocation = SrcFile.initptrlocation; //save the initial ptr location
  ptrlocation = SrcFile.ptrlocation; //save the ptr location

  return *this;
}


bool File::Open(const char *Name,const wchar *NameW,bool OpenShared_,bool Update)
{ //This function does nothing. It simply complies with the File class. 
  //This class should utilize the Init(void* ptr) function to open a file.
	//hFile = hNewFile;
	return true;
/*  ErrorType=FILE_SUCCESS;
  FileHandle hNewFile;
  if (BenFile::OpenShared_)
    OpenShared_=true;
#ifdef _WIN_ALL
  uint Access=GENERIC_READ;
  if (Update)
    Access|=GENERIC_WRITE;
  uint ShareMode=FILE_SHARE_READ;
  if (OpenShared_)
    ShareMode|=FILE_SHARE_WRITE;
  uint Flags=NoSequentialRead ? 0:FILE_FLAG_SEQUENTIAL_SCAN;
  if (WinNT() && NameW!=NULL && *NameW!=0)
    hNewFile=CreateFileW(NameW,Access,ShareMode,NULL,OPEN_EXISTING,Flags,NULL);
  else
    hNewFile=CreateFileA(Name,Access,ShareMode,NULL,OPEN_EXISTING,Flags,NULL);

  if (hNewFile==BAD_HANDLE && GetLastError()==ERROR_FILE_NOT_FOUND)
    ErrorType=FILE_NOTFOUND;
#else
  int flags=Update ? O_RDWR:O_RDONLY;
#ifdef O_BINARY
  flags|=O_BINARY;
#if defined(_AIX) && defined(_LARGE_FILE_API)
  flags|=O_LARGEFILE;
#endif
#endif
#if defined(_EMX) && !defined(_DJGPP)
  int sflags=OpenShared_ ? SH_DENYNO:SH_DENYWR;
  int handle=sopen(Name,flags,sflags);
#else
  int handle=open(Name,flags);
#ifdef LOCK_EX

#ifdef _OSF_SOURCE
  extern "C" int flock(int, int);
#endif

  if (!OpenShared_ && Update && handle>=0 && flock(handle,LOCK_EX|LOCK_NB)==-1)
  {
    close(handle);
    return(false);
  }
#endif
#endif
  hNewFile=handle==-1 ? BAD_HANDLE:fdopen(handle,Update ? UPDATEBINARY:READBINARY);
  if (hNewFile==BAD_HANDLE && errno==ENOENT)
    ErrorType=FILE_NOTFOUND;
#endif
  NewFile=false;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  bool Success=hNewFile!=BAD_HANDLE;
  if (Success)
  {
    hFile=hNewFile;

    // We use memove instead of strcpy and wcscpy to avoid problems
    // with overlapped buffers. While we do not call this function with
    // really overlapped buffers yet, we do call it with Name equal to
    // FileName like Arc.Open(Arc.FileName,Arc.FileNameW).
    if (NameW!=NULL)
      memmove(FileNameW,NameW,(wcslen(NameW)+1)*sizeof(*NameW));
    else
      *FileNameW=0;
    if (Name!=NULL)
      memmove(FileName,Name,strlen(Name)+1);
    else
      WideToChar(NameW,FileName);
    AddFileToList(hFile);
  }
  return(Success);*/
}


#if !defined(SHELL_EXT) && !defined(SFX_MODULE)
/**
This function is not called in bulk_extractor
*/
void File::TOpen(const char *Name,const wchar *NameW)
{ //Nothing to do for this function
  if (!WOpen(Name,NameW))
    ErrHandler.Exit(OPEN_ERROR);
}
#endif

/**
This function does nothing. It simply complies with the <code>File</code> class.
*/
bool File::WOpen(const char *Name,const wchar *NameW)
{ //This class should utilize the Init(void* ptr) function to open a file.
	bool result = true;
	if(ptrlocation == NULL || ptrlength == 0)
		result = false;
//	else
	//	hFile = FileHandle.;
	return result;
 /* if (Open(Name,NameW))
    return(true);
  ErrHandler.OpenErrorMsg(Name,NameW);
  return(false);*/
}

/**
This function does nothing. It simply complies with the File class.
*/
bool File::Create(const char *Name,const wchar *NameW,bool ShareRead)
{
	return true;
	/*
#ifdef _WIN_ALL
  DWORD ShareMode=(ShareRead || BenFile::OpenShared) ? FILE_SHARE_READ:0;
  if (WinNT() && NameW!=NULL && *NameW!=0)
    hFile=CreateFileW(NameW,GENERIC_READ|GENERIC_WRITE,ShareMode,NULL,
                      CREATE_ALWAYS,0,NULL);
  else
    hFile=CreateFileA(Name,GENERIC_READ|GENERIC_WRITE,ShareMode,NULL,
                     CREATE_ALWAYS,0,NULL);
#else
  hFile=fopen(Name,CREATEBINARY);
#endif
  NewFile=true;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  if (NameW!=NULL)
    wcscpy(FileNameW,NameW);
  else
    *FileNameW=0;
  if (Name!=NULL)
    strcpy(FileName,Name);
  else
    WideToChar(NameW,FileName);
  AddFileToList(hFile);
  return(hFile!=BAD_HANDLE);*/
}

/**
This is not called in bulk_extractor
*/
void File::AddFileToList(FileHandle hFile_)
{
  if (hFile_!=BAD_HANDLE)
    for (unsigned I=0;I<sizeof(CreatedFiles)/sizeof(CreatedFiles[0]);I++)
      if (CreatedFiles[I]==NULL)
      {
        CreatedFiles[I]=this;
        break;
      }
}


#if !defined(SHELL_EXT) && !defined(SFX_MODULE)
/**
This is not called in bulk_extractor
*/
void File::TCreate(const char *Name,const wchar *NameW,bool ShareRead)
{
  if (!WCreate(Name,NameW,ShareRead))
    ErrHandler.Exit(FATAL_ERROR);
}
#endif

/**
This is not called in bulk_extractor
*/
bool File::WCreate(const char *Name,const wchar *NameW,bool ShareRead)
{
  if (Create(Name,NameW,ShareRead))
    return(true);
  ErrHandler.SetErrorCode(CREATE_ERROR);
  ErrHandler.CreateErrorMsg(Name,NameW);
  return(false);
}

/**
This function does nothing. It simply complies with the File class.
*/
bool File::Close()
{ 
	ptrlocation = initptrlocation; //reset Pointer location to the initial spot
	return true;
  /*bool Success=true;
  if (HandleType!=FILE_HANDLENORMAL)
    HandleType=FILE_HANDLENORMAL;
  else
    if (hFile!=BAD_HANDLE)
    {
      if (!SkipClose)
      {
#ifdef _WIN_ALL
        Success=CloseHandle(hFile)==TRUE;
#else
        Success=fclose(hFile)!=EOF;
#endif
        if (Success || !RemoveCreatedActive)
          for (int I=0;I<sizeof(CreatedFiles)/sizeof(CreatedFiles[0]);I++)
            if (CreatedFiles[I]==this)
            {
              CreatedFiles[I]=NULL;
              break;
            }
      }
      hFile=BAD_HANDLE;
      if (!Success && AllowExceptions)
        ErrHandler.CloseError(FileName,FileNameW);
    }
  CloseCount++;
  return(Success);*/
}

/**
This is not called in bulk_extractor.
*/
void File::Flush()
{
#ifdef _WIN_ALL
  FlushFileBuffers(hFile);
#else
  fflush(hFile);
#endif
}

/**
This is not called in bulk_extractor.
*/
bool File::Delete()
{ 
if (HandleType!=FILE_HANDLENORMAL)
    return(false);
  if (hFile!=BAD_HANDLE)
    Close();
  if (!AllowDelete)
    return(false);
  return(DelFile(FileName,FileNameW));
}

/**
This function should never be called since the file name should never be changed
*/
bool File::Rename(const char *NewName,const wchar *NewNameW)
{ 
  // we do not need to rename if names are already same
  bool Success=strcmp(FileName,NewName)==0;
  if (Success && *FileNameW!=0 && *NullToEmpty(NewNameW)!=0)
    Success=wcscmp(FileNameW,NewNameW)==0;

  if (!Success)
    Success=RenameFile(FileName,FileNameW,NewName,NewNameW);

  if (Success)
  {
    // renamed successfully, storing the new name
    strcpy(FileName,NewName);
    wcscpy(FileNameW,NullToEmpty(NewNameW));
  }
  return(Success);
}

/**
This is not called in bulk_extractor
*/
void File::Write(const void *Data,size_t Size)
{ //This is not called in bulk_extractor 
  if (Size==0)
    return;
#ifndef _WIN_CE
  if (HandleType!=FILE_HANDLENORMAL)
    switch(HandleType)
    {
      case FILE_HANDLESTD:
#ifdef _WIN_ALL
        hFile=GetStdHandle(STD_OUTPUT_HANDLE);
#else
        hFile=stdout;
#endif
        break;
      case FILE_HANDLEERR:
#ifdef _WIN_ALL
        hFile=GetStdHandle(STD_ERROR_HANDLE);
#else
        hFile=stderr;
#endif
        break;
      default:
        break;
    }
#endif
  while (1)
  {
    bool Success=false;
#ifdef _WIN_ALL
    DWORD Written=0;
    if (HandleType!=FILE_HANDLENORMAL)
    {
      // writing to stdout can fail in old Windows if data block is too large
      const size_t MaxSize=0x4000;
      for (size_t I=0;I<Size;I+=MaxSize)
      {
        Success=WriteFile(hFile,(byte *)Data+I,(DWORD)Min(Size-I,MaxSize),&Written,NULL)==TRUE; //Writing happens here
        if (!Success)
          break;
      }
    }
    else
      Success=WriteFile(hFile,Data,(DWORD)Size,&Written,NULL)==TRUE;
#else
    size_t Written=fwrite(Data,1,Size,hFile);
    Success=Written==Size && !ferror(hFile);
#endif
    if (!Success && AllowExceptions && HandleType==FILE_HANDLENORMAL)
    {
#if defined(_WIN_ALL) && !defined(SFX_MODULE) && !defined(RARDLL)
      int ErrCode=GetLastError();
      int64 FilePos=Tell();
      uint64 FreeSize=GetFreeDisk(FileName);
      SetLastError(ErrCode);
      if (FreeSize>Size && FilePos-Size<=0xffffffff && FilePos+Size>0xffffffff)
        ErrHandler.WriteErrorFAT(FileName,FileNameW);
#endif
      if (ErrHandler.AskRepeatWrite(FileName,FileNameW,false))
      {
#ifndef _WIN_ALL
        clearerr(hFile);
#endif
        if (Written<Size && Written>0)
          Seek(Tell()-Written,SEEK_SET);
        continue;
      }
      ErrHandler.WriteError(NULL,NULL,FileName,FileNameW);
    }
    break;
  }
  LastWrite=true;
}

/**
Read data from memory of size <code>Size</code>. Calls the <code>DirectRead(Data,Size)</code> function.
@param Data - a pointer to the memory location of the RAR file to be extracted 
@param Size - the length, in bytes, of the <code>Data</code> variable
@return the size that was read from the memory location
*/
int File::Read(void *Data,size_t Size)
{ //Read data from memory of size 'Size'

	//call directRead
	int readsize = DirectRead(Data, Size);
	
	return readsize;

/*int64 FilePos=0; // Initialized only to suppress some compilers warning.

  if (IgnoreReadErrors)
    FilePos=Tell();
  int ReadSize;
  while (true)
  {
    ReadSize=DirectRead(Data,Size);
    if (ReadSize==-1)
    {
      ErrorType=FILE_READERROR;
      if (AllowExceptions)
        if (IgnoreReadErrors)
        {
          ReadSize=0;
          for (size_t I=0;I<Size;I+=512)
          {
            Seek(FilePos+I,SEEK_SET);
            size_t SizeToRead=Min(Size-I,512);
            int ReadCode=DirectRead(Data,SizeToRead);
            ReadSize+=(ReadCode==-1) ? 512:ReadCode;
          }
        }
        else
        {
          if (HandleType==FILE_HANDLENORMAL && ErrHandler.AskRepeatRead(FileName,FileNameW))
            continue;
          ErrHandler.ReadError(FileName,FileNameW);
        }
    }
    break;
  }
  return(ReadSize);*/
}

/**
Read data from memory of size <code>Size</code>. Calls the <code>DirectRead(Data,Size)</code> function.
@param Data - a pointer to the memory location of the RAR file to be extracted 
@param Size - the length, in bytes, of the <code>Data</code> variable
@return the size that was read from the memory location
*/
int File::DirectRead(void *Data,size_t Size)
{ //Calls DirectRead function
	return DirectRead((byte*)Data, Size);
}


/**
Read data from memory of size <code>Size</code>. Calls the <code>DirectRead(Data,Size)</code> function.
@param Data - a pointer to the memory location of the RAR file to be extracted 
@param Size - the length, in bytes, of the <code>Data</code> variable
@return the size that was read from the memory location. If a '-1' is returned, an error has occurred.
*/
int File::DirectRead(byte *Data,size_t Size)
{ //Reads from memory the size as spcified by 'Size'

	//read the amount of information from memory
	//set the pointers to the correct location
	//send off results

	//int result = -1;
	int result = 0;
	if(Tell(ptrlocation + Size) <= ptrlength)
	{ //verifies that we will be in bounds
		result = Size;
		memcpy(Data, ptrlocation, Size);
		
		
		ptrlocation += Size; //adjust the pointer location
		
		//Tell(); //for debugging purposes
		/*for(int i = 0; i < result; i++)
		{
			Data = ptrlocation;
			Data++;
			ptrlocation++;
		}*/
	}


	return result;

	/*
#ifdef _WIN_ALL
  const size_t MaxDeviceRead=20000;
#endif
#ifndef _WIN_CE
  if (HandleType==FILE_HANDLESTD)
  {
#ifdef _WIN_ALL
    if (Size>MaxDeviceRead)
      Size=MaxDeviceRead;
    hFile=GetStdHandle(STD_INPUT_HANDLE);
#else
    hFile=stdin;
#endif
  }
#endif
#ifdef _WIN_ALL
  DWORD Read;
  if (!ReadFile(hFile,Data,(DWORD)Size,&Read,NULL))
  {
    if (IsDevice() && Size>MaxDeviceRead)
      return(DirectRead(Data,MaxDeviceRead));
    if (HandleType==FILE_HANDLESTD && GetLastError()==ERROR_BROKEN_PIPE)
      return(0);
    return(-1);
  }
  return(Read);
#else
  if (LastWrite)no 
  {
    fflush(hFile);
    LastWrite=false;
  }
  clearerr(hFile);
  size_t ReadSize=fread(Data,1,Size,hFile);
  if (ferror(hFile))
    return(-1);
  return((int)ReadSize);
#endif*/
}

/**
Moves the pointer in the file to a specified location. Calls the <code>RawSeek</code> function.
@param Offset - the length from the current position
@param Method - the method to move the pointer. 
If <code>SEEK_SET</code>, move the pointer <code>Offset</code> number of bytes to the new location in memory.
If <code>SEEK_END</code>, move the pointer to the end of the file (NOT IMPLEMENTED)
If <code>SEEK_CUR</code>, move the pointer (NOT IMPLEMENTED).
*/
void File::Seek(int64 Offset,int Method)
{ //Calls RawSeek function
	RawSeek(Offset,Method);
  /*if (!RawSeek(Offset,Method) && AllowExceptions)
    ErrHandler.SeekError(FileName,FileNameW);*/
}

/**
Moves the pointer in the file to a specified location
@param Offset - the length from the current position
@param Method - the method to move the pointer. 
If <code>SEEK_SET</code>, move the pointer <code>Offset</code> number of bytes to the new location in memory.
If <code>SEEK_END</code>, move the pointer to the end of the file (NOT IMPLEMENTED)
If <code>SEEK_CUR</code>, move the pointer (NOT IMPLEMENTED).
*/
bool File::RawSeek(int64 Offset,int Method)
{ //Seeks file location
 /* if (hFile==BAD_HANDLE)
    return(true);*/  //we will assume all is well if we have gone this far.

	
	if(Method == SEEK_SET)
	{
		/*if(&ptrlocation + Offset > &initptrlocation + ptrlength)
			return(false); //we have overun the bounds

		if(&ptrlocation + Offset < &initptrlocation)
			return(false); //we have overrun the bounds*/

		//ptrlocation = (byte*)Offset;

		bool result = true;
		byte * tempptrlocation = ptrlocation;

		if(Offset > Tell())
		{
			byte newdiff = (byte)Offset - Tell();
			tempptrlocation += newdiff;
			//int64 telldebugging = Tell();
			//int64 telldebugging2 = Tell(tempptrlocation);
			//ptrlocation += newdiff;
			if(Tell(tempptrlocation)  > ptrlength )
			{//the new pointer length is farther than the length of the RAR file
				result = false;
			}
			else
			{// the new pointer length is within range of the length of the RAR file
				ptrlocation += newdiff;
				result = true;
			}
		}
		else if(Offset < Tell())
		{
			byte newdiff = Tell() - (byte)Offset;
			tempptrlocation -= newdiff;

			//ptrlocation -= newdiff;
			if(Tell(tempptrlocation)  < Tell(initptrlocation))
			{//the new pointer length is before the range of the RAR file
				result = false;
			}
			else{//the new pointer is within range of the length of the file
				ptrlocation -= newdiff;
				result = true;
			}

		}

		//Tell(); //debugging function
		return(result);
	}

	else if(Method == SEEK_END)
	{
		//we should go to the end of the file
		ptrlocation = initptrlocation; 	//reset the pointer location to the beginning of the RAR file
		ptrlocation += ptrlength;	//set the pointer to the end of the RAR file
		return true;
	}
	else if(Method == SEEK_CUR)
	{
		//we should do something else
		return false;
	}

	else
	{//unknown method...
		return false;
	}

/*  if (Offset<0 && Method!=SEEK_SET)
  {
    Offset=(Method==SEEK_CUR ? Tell():FileLength())+Offset;
    Method=SEEK_SET;
  }
#ifdef _WIN_ALL
  LONG HighDist=(LONG)(Offset>>32);
  if (SetFilePointer(hFile,(LONG)Offset,&HighDist,Method)==0xffffffff &&
      GetLastError()!=NO_ERROR)
    return(false);
#else
  LastWrite=false;
#if defined(_LARGEFILE_SOURCE) && !defined(_OSF_SOURCE) && !defined(__VMS)
  if (fseeko(hFile,Offset,Method)!=0)
#else
  if (fseek(hFile,(long)Offset,Method)!=0)
#endif
    return(false);
#endif
  return(true);*/
}

/**
@param ptrlocation - location that should be used to find the offset from the beginning of the file class
@return the location of the pointer offset that has been supplied in a <code>int64</code> format.
*/
int64 File::Tell(byte* theptrlocation)
{
	return ((int64)theptrlocation - (int64)initptrlocation);
}

/**
@return the location of the pointer offset in a <code>int64</code> format.
*/
int64 File::Tell()
{//returns the offset value where the iterator lies

	return ((int64)ptrlocation - (int64)initptrlocation);
/*  if (hFile==BAD_HANDLE)
    if (AllowExceptions)
      ErrHandler.SeekError(FileName,FileNameW);
    else
      return(-1);
#ifdef _WIN_ALL
  LONG HighDist=0;
  uint LowDist=SetFilePointer(hFile,0,&HighDist,FILE_CURRENT);
  if (LowDist==0xffffffff && GetLastError()!=NO_ERROR)
    if (AllowExceptions)
      ErrHandler.SeekError(FileName,FileNameW);
    else
      return(-1);
  return(INT32TO64(HighDist,LowDist));
#else
#if defined(_LARGEFILE_SOURCE) && !defined(_OSF_SOURCE)
  return(ftello(hFile));
#else
  return(ftell(hFile));
#endif
#endif*/
}

/**
this is not needed for bulk_extractor
*/
void File::Prealloc(int64 Size)
{ 
#ifdef _WIN_ALL
  if (RawSeek(Size,SEEK_SET))
  {
    Truncate();
    Seek(0,SEEK_SET);
  }
#endif

#if defined(_UNIX) && defined(USE_FALLOCATE)
  // fallocate is rather new call. Only latest kernels support it.
  // So we are not using it by default yet.
  int fd = fileno(hFile);
  if (fd >= 0)
    fallocate(fd, 0, 0, Size);
#endif
}

/**
Obtain a byte worth of data from the memory space
*/
byte File::GetByte()
{
  byte Byte=0;
  Read(&Byte,1);
  return(Byte);
}

/**
this is not needed for bulk_extractor
*/
void File::PutByte(byte Byte)
{
  Write(&Byte,1);
}

/**
this is not needed for bulk_extractor
*/
bool File::Truncate()
{
#ifdef _WIN_ALL
  return(SetEndOfFile(hFile)==TRUE);
#else
  return(false);
#endif
}

/**
this is not needed for bulk_extractor
*/
void File::SetOpenFileTime(RarTime *ftm,RarTime *ftc,RarTime *fta)
{ //We probably don't need to worry about this at all
#ifdef _WIN_ALL
  bool sm=ftm!=NULL && ftm->IsSet();
  bool sc=ftc!=NULL && ftc->IsSet();
  bool sa=fta!=NULL && fta->IsSet();
  FILETIME fm,fc,fa;
  if (sm)
    ftm->GetWin32(&fm);
  if (sc)
    ftc->GetWin32(&fc);
  if (sa)
    fta->GetWin32(&fa);
  SetFileTime(hFile,sc ? &fc:NULL,sa ? &fa:NULL,sm ? &fm:NULL);
#endif
}

/**
this is not needed for bulk_extractor
*/
void File::SetCloseFileTime(RarTime *ftm,RarTime *fta)
{ //We probably don't need to worry about this at all.
	//int i = 1+1;
#if defined(_UNIX) || defined(_EMX)
  SetCloseFileTimeByName(FileName,ftm,fta);
#endif
}

/**
this is not needed for bulk_extractor
*/
void File::SetCloseFileTimeByName(const char *Name,RarTime *ftm,RarTime *fta)
{ //We probably don't need to worry about this at all.
	//int i = 1+1;

#if defined(_UNIX) || defined(_EMX)
  bool setm=ftm!=NULL && ftm->IsSet();
  bool seta=fta!=NULL && fta->IsSet();
  if (setm || seta)
  {
    utimbuf ut;
    if (setm)
      ut.modtime=ftm->GetUnix();
    else
      ut.modtime=fta->GetUnix();
    if (seta)
      ut.actime=fta->GetUnix();
    else
      ut.actime=ut.modtime;
    utime(Name,&ut);
  }
#endif
}

/**
this is not needed for bulk_extractor
*/
void File::GetOpenFileTime(RarTime *ft)
{ //We probably don't need to worry about this at all.
#ifdef _WIN_ALL
  FILETIME FileTime;
  GetFileTime(hFile,NULL,NULL,&FileTime);
  *ft=FileTime;
#endif
#if defined(_UNIX) || defined(_EMX)
  struct stat st;
  fstat(fileno(hFile),&st);
  *ft=st.st_mtime;
#endif
}

/**
@return the length of the memory, in bytes, that has been allocated for the <code>File</code> class.
*/
int64 File::FileLength()
{ //returns length of file
  return ptrlength;
	
/*  SaveFilePos SavePos(*this);
  Seek(0,SEEK_END);
  return(Tell());*/
  //Tell();
}

/**
this is not needed for bulk_extractor
*/
void File::SetHandleType(FILE_HANDLETYPE Type)
{
  HandleType=Type;
}

/**
Always returns false as we are never reading from a device
*/
bool File::IsDevice()
{ //This really doesn't have to be called
	return false; //No, we are not reading from a device
/*  if (hFile==BAD_HANDLE)
    return(false);
#ifdef _WIN_ALL
  uint Type=GetFileType(hFile);
  return(Type==FILE_TYPE_CHAR || Type==FILE_TYPE_PIPE);
#else
  return(isatty(fileno(hFile)));
#endif*/
}


#ifndef SFX_MODULE
/**
this is not needed for bulk_extractor
*/
void File::fprintf(const char *fmt,...)
{ //This function is not called in bulk_extractor
  va_list argptr;
  va_start(argptr,fmt);
  safebuf char Msg[2*NM+1024],OutMsg[2*NM+1024];
  vsprintf(Msg,fmt,argptr);
#ifdef _WIN_ALL
  for (int Src=0,Dest=0;;Src++)
  {
    char CurChar=Msg[Src];
    if (CurChar=='\n')
      OutMsg[Dest++]='\r';
    OutMsg[Dest++]=CurChar;
    if (CurChar==0)
      break;
  }
#else
  strcpy(OutMsg,Msg);
#endif
  Write(OutMsg,strlen(OutMsg));
  va_end(argptr);
}
#endif

/**
this is not needed for bulk_extractor
*/
bool File::RemoveCreated()
{ //This file is not called in bulk_extractor
    return false;
}


#ifndef SFX_MODULE
/**
this is not needed for bulk_extractor
*/
int64 File::Copy(File &Dest,int64 Length)
{ //This file is not called in bulk_extractor
  Array<char> Buffer(0x10000);
  int64 CopySize=0;
  bool CopyAll=(Length==INT64NDF);

  while (CopyAll || Length>0)
  {
    Wait();
    size_t SizeToRead=(!CopyAll && Length<(int64)Buffer.Size()) ? (size_t)Length:Buffer.Size();
    int ReadSize=Read(&Buffer[0],SizeToRead);
    if (ReadSize==0)
      break;
    Dest.Write(&Buffer[0],ReadSize);
    CopySize+=ReadSize;
    if (!CopyAll)
      Length-=ReadSize;
  }
  return(CopySize);
}

/**
@return the current pointer location as a <code>void*</code> type pointer
*/
void* File::GetPtrLocation()
{
	return ptrlocation;
}

/**
@return the initial pointer location when the <code>File</code> object was created
*/
void * File::GetInitPtrLocation()
{
	return initptrlocation;
}

#endif
