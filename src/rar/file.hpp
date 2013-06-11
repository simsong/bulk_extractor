#ifndef _RAR_FILE_
#define _RAR_FILE_

#define NM 1024

#ifdef _WIN_ALL
typedef HANDLE FileHandle;
#define BAD_HANDLE INVALID_HANDLE_VALUE
#else
typedef FILE* FileHandle;
#define BAD_HANDLE NULL
#endif

class RAROptions;

enum FILE_HANDLETYPE {FILE_HANDLENORMAL,FILE_HANDLESTD,FILE_HANDLEERR};

enum FILE_ERRORTYPE {FILE_SUCCESS,FILE_NOTFOUND,FILE_READERROR};

struct FileStat
{
  uint FileAttr;
  uint FileTime;
  int64 FileSize;
  bool IsDir;
};
/**
This <b>edited</b> file class contains all the variables and functions to read a RAR file 
from memory.
*/
class File
{
  private:
    void AddFileToList(FileHandle hFile);
	
	byte * ptrlocation;
	byte * initptrlocation;
	int64 ptrlength;
    FileHandle hFile;
    bool LastWrite;
    FILE_HANDLETYPE HandleType;
    bool SkipClose;
    bool IgnoreReadErrors;
    bool NewFile;
    bool AllowDelete;
    bool AllowExceptions;
#ifdef _WIN_ALL
    bool NoSequentialRead;
#endif
  protected:
    bool OpenShared;
  public:
    char FileName[NM];
    wchar FileNameW[NM];

    FILE_ERRORTYPE ErrorType;

    uint CloseCount;
  public:
    File();
    File(const File& copy);
    virtual ~File();
	void InitFile(void* ptr, int64 ptrlength); //sets the ptr location, and the length that can be read
    const File& operator=(const File &SrcFile);
    bool Open(const char *Name,const wchar *NameW=NULL,bool OpenShared=false,bool Update=false);
    void TOpen(const char *Name,const wchar *NameW=NULL);
    bool WOpen(const char *Name,const wchar *NameW=NULL);
    bool Create(const char *Name,const wchar *NameW=NULL,bool ShareRead=true);
    void TCreate(const char *Name,const wchar *NameW=NULL,bool ShareRead=true);
    bool WCreate(const char *Name,const wchar *NameW=NULL,bool ShareRead=true);
    bool Close();
    void Flush();
    bool Delete();
    bool Rename(const char *NewName,const wchar *NewNameW=NULL);
    void Write(const void *Data,size_t Size);
    int Read(void *Data,size_t Size);
    int DirectRead(void *Data,size_t Size); //Edited by Ben
	int DirectRead(byte *Data,size_t Size); //Edited by Ben
    void Seek(int64 Offset,int Method);
    bool RawSeek(int64 Offset,int Method);
    int64 Tell();
	int64 Tell(byte* addr);
    void Prealloc(int64 Size);
    byte GetByte();
    void PutByte(byte Byte);
    bool Truncate();
    void SetOpenFileTime(RarTime *ftm,RarTime *ftc=NULL,RarTime *fta=NULL);
    void SetCloseFileTime(RarTime *ftm,RarTime *fta=NULL);
    static void SetCloseFileTimeByName(const char *Name,RarTime *ftm,RarTime *fta);
    void GetOpenFileTime(RarTime *ft);
    bool IsOpened() {return true;/*(hFile!=BAD_HANDLE);*/}; //file will be opened
    int64 FileLength();
    void SetHandleType(FILE_HANDLETYPE Type);
    FILE_HANDLETYPE GetHandleType() {return(HandleType);};
    bool IsDevice();
    void fprintf(const char *fmt,...);
    static bool RemoveCreated();
    FileHandle GetHandle() {return(hFile);};
    void SetIgnoreReadErrors(bool Mode) {IgnoreReadErrors=Mode;};
    char *GetName() {return(FileName);}
    int64 Copy(File &Dest,int64 Length=INT64NDF);
    void SetAllowDelete(bool Allow) {AllowDelete=Allow;}
    void SetExceptions(bool Allow) {AllowExceptions=Allow;}
	void *GetPtrLocation(); //Returns pointer location
	void *GetInitPtrLocation();

#ifdef _WIN_ALL
    void RemoveSequentialFlag() {NoSequentialRead=true;}
#endif
};

// work around cyclic dependency
#include "filefn.hpp"

#endif
