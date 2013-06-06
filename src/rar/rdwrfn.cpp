#include "rar.hpp"

ComprDataIO::ComprDataIO() :
    UnpackFromMemory(), UnpackFromMemorySize(), UnpackFromMemoryAddr(),
    UnpackToMemory(), UnpackToMemorySize(), UnpackToMemoryAddr(), UnpWrSize(),
    UnpWrAddr(), UnpPackedSize(), ShowProgress(), TestMode(), SkipUnpCRC(),
    SrcFile(), DestFile(), Command(), SubHead(), SubHeadPos(), LastPercent(),
    CurrentCommand(), PackVolume(), UnpVolume(), NextVolumeMissing(),
    TotalPackRead(), UnpArcSize(), CurPackRead(), CurPackWrite(), CurUnpRead(),
    CurUnpWrite(), ProcessedArcSize(), TotalArcSize(), PackFileCRC(),
    UnpFileCRC(), PackedCRC(), Encryption(), Decryption()
{
  Init();
}


void ComprDataIO::Init()
{
  UnpackFromMemory=false;
  UnpackToMemory=false;
  UnpPackedSize=0;
  ShowProgress=true;
  TestMode=false;
  SkipUnpCRC=false;
  PackVolume=false;
  UnpVolume=false;
  NextVolumeMissing=false;
  SrcFile=NULL;
  DestFile=NULL;
  UnpWrSize=0;
  Command=NULL;
  Encryption=0;
  Decryption=0;
  TotalPackRead=0;
  CurPackRead=CurPackWrite=CurUnpRead=CurUnpWrite=0;
  PackFileCRC=UnpFileCRC=PackedCRC=0xffffffff;
  LastPercent=-1;
  SubHead=NULL;
  SubHeadPos=NULL;
  CurrentCommand=0;
  ProcessedArcSize=TotalArcSize=0;
}




int ComprDataIO::UnpRead(byte *Addr,size_t Count)
{
  int RetCode=0,TotalRead=0;
  byte *ReadAddr;
  ReadAddr=Addr;
  while (Count > 0)
  {
    Archive *SrcArc=(Archive *)SrcFile;

    size_t ReadSize=((int64)Count>UnpPackedSize) ? (size_t)UnpPackedSize:Count;
    if (UnpackFromMemory)
    {
      memcpy(Addr,UnpackFromMemoryAddr,UnpackFromMemorySize);
      RetCode=(int)UnpackFromMemorySize;
      UnpackFromMemorySize=0;
    }
    else
    {
      if (!SrcFile->IsOpened())
        return(-1);
      RetCode=SrcFile->Read(ReadAddr,ReadSize);
      FileHeader *hd=SubHead!=NULL ? SubHead:&SrcArc->NewLhd;
      if (hd->Flags & LHD_SPLIT_AFTER)
        PackedCRC=CRC(PackedCRC,ReadAddr,RetCode);
    }
    CurUnpRead+=RetCode;
    TotalRead+=RetCode;
#ifndef NOVOLUME
    // These variable are not used in NOVOLUME mode, so it is better
    // to exclude commands below to avoid compiler warnings.
    ReadAddr+=RetCode;
    Count-=RetCode;
#endif
    UnpPackedSize-=RetCode;
    if (UnpPackedSize == 0 && UnpVolume)
    {
      {
        NextVolumeMissing=true;
        return(-1);
      }
    }
    else
      break;
  }
  Archive *SrcArc=(Archive *)SrcFile;
  if (SrcArc!=NULL)
    ShowUnpRead(SrcArc->CurBlockPos+CurUnpRead,UnpArcSize);
  if (RetCode!=-1)
  {
    RetCode=TotalRead;
  }
  Wait();
  return(RetCode);
}


#if defined(RARDLL) && defined(_MSC_VER) && !defined(_WIN_64)
// Disable the run time stack check for unrar.dll, so we can manipulate
// with ProcessDataProc call type below. Run time check would intercept
// a wrong ESP before we restore it.
#pragma runtime_checks( "s", off )
#endif

void ComprDataIO::UnpWrite(byte *Addr,size_t Count)
{

#ifdef RARDLL
  RAROptions *Cmd=((Archive *)SrcFile)->GetRAROptions();
  if (Cmd->DllOpMode!=RAR_SKIP)
  {
    if (Cmd->Callback!=NULL &&
        Cmd->Callback(UCM_PROCESSDATA,Cmd->UserData,(LPARAM)Addr,Count)==-1)
      ErrHandler.Exit(USER_BREAK);
    if (Cmd->ProcessDataProc!=NULL)
    {
      // Here we preserve ESP value. It is necessary for those developers,
      // who still define ProcessDataProc callback as "C" type function,
      // even though in year 2001 we announced in unrar.dll whatsnew.txt
      // that it will be PASCAL type (for compatibility with Visual Basic).
#if defined(_MSC_VER)
#ifndef _WIN_64
      __asm mov ebx,esp
#endif
#elif defined(_WIN_ALL) && defined(__BORLANDC__)
      _EBX=_ESP;
#endif
      int RetCode=Cmd->ProcessDataProc(Addr,(int)Count);

      // Restore ESP after ProcessDataProc with wrongly defined calling
      // convention broken it.
#if defined(_MSC_VER)
#ifndef _WIN_64
      __asm mov esp,ebx
#endif
#elif defined(_WIN_ALL) && defined(__BORLANDC__)
      _ESP=_EBX;
#endif
      if (RetCode==0)
        ErrHandler.Exit(USER_BREAK);
    }
  }
#endif // RARDLL

  UnpWrAddr=Addr;
  UnpWrSize=Count;
  if (UnpackToMemory)
  {
    if (Count <= UnpackToMemorySize)
    {
      memcpy(UnpackToMemoryAddr,Addr,Count);
      UnpackToMemoryAddr+=Count;
      UnpackToMemorySize-=Count;
    }
  }
  else
    if (!TestMode)
      DestFile->Write(Addr,Count); //The writing actually happens right here
  CurUnpWrite+=Count;
  if (!SkipUnpCRC)
#ifndef SFX_MODULE
  {
    if (((Archive *)SrcFile)->OldFormat)
      UnpFileCRC=OldCRC((ushort)UnpFileCRC,Addr,Count);
  }
    else
#endif
      UnpFileCRC=CRC(UnpFileCRC,Addr,Count);
  ShowUnpWrite();
  Wait();
}

#if defined(RARDLL) && defined(_MSC_VER) && !defined(_WIN_64)
// Restore the run time stack check for unrar.dll.
#pragma runtime_checks( "s", restore )
#endif






void ComprDataIO::ShowUnpRead(int64 ArcPos,int64 ArcSize)
{
  if (ShowProgress && SrcFile!=NULL)
  {
    if (TotalArcSize!=0)
    {
      // important when processing several archives or multivolume archive
      ArcSize=TotalArcSize;
      ArcPos+=ProcessedArcSize;
    }

    Archive *SrcArc=(Archive *)SrcFile;
    RAROptions *Cmd=SrcArc->GetRAROptions();

    int CurPercent=ToPercent(ArcPos,ArcSize);
    if (!Cmd->DisablePercentage && CurPercent!=LastPercent)
    {
      //mprintf("\b\b\b\b%3d%%",CurPercent);
		LastPercent=CurPercent;
    }
  }
}


void ComprDataIO::ShowUnpWrite()
{
}








void ComprDataIO::SetFiles(File *SrcFile_,File *DestFile_)
{
  if (SrcFile_!=NULL)
    ComprDataIO::SrcFile=SrcFile_;
  if (DestFile_!=NULL)
    ComprDataIO::DestFile=DestFile_;
  LastPercent=-1;
}


void ComprDataIO::GetUnpackedData(byte **Data,size_t *Size)
{
  *Data=UnpWrAddr;
  *Size=UnpWrSize;
}


void ComprDataIO::SetEncryption(int Method,const wchar *Password,const byte *Salt,bool Encrypt,bool HandsOffHash)
{
  if (Encrypt)
  {
    Encryption=*Password ? Method:0;
  }
  else
  {
    Decryption=*Password ? Method:0;
  }
}





void ComprDataIO::SetUnpackToMemory(byte *Addr,uint Size)
{
  UnpackToMemory=true;
  UnpackToMemoryAddr=Addr;
  UnpackToMemorySize=Size;
}

/*void ComprDataIO::SetUnpackFromMemory(byte *Addr, uint Size)
{
	UnpackFromMemory = true;
    UnpackFromMemorySize  = Size;
    UnpackFromMemoryAddr = (byte*)Addr;
}*/


