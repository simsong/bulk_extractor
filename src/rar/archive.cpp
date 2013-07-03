#include "rar.hpp"

#ifndef SHELL_EXT
//#include "arccmt.cpp"
#endif

/**
Special Constructor
*/
Archive::Archive(RAROptions *InitCmd) :
    SubDataIO(), Cmd(), DummyCmd(), MarkHead(), OldMhd(), RecoverySectors(),
    RecoveryPos(), FailedHeaderDecryption(), LatestTime(), LastReadBlock(),
    CurHeaderType(), SilentOpen(), ShortBlock(), NewMhd(), NewLhd(),
    EndArcHead(), SubBlockHead(), SubHead(), CommHead(), ProtectHead(),
    AVHead(), SignHead(), UOHead(), MACHead(), EAHead(), StreamHead(),
    CurBlockPos(), NextBlockPos(), OldFormat(), Solid(), Volume(),
    MainComment(), Locked(), Signed(), NotFirstVolume(), Protected(),
    Encrypted(), SFXSize(), BrokenFileHeader(), Splitting(), HeaderCRC(),
    VolWrite(), AddingFilesSize(), AddingHeadersSize(), NewArchive(),
    FirstVolumeName(), FirstVolumeNameW()
{
  Cmd=InitCmd==NULL ? &DummyCmd:InitCmd;
  OpenShared=Cmd->OpenShared;
  OldFormat=false;
  Solid=false;
  Volume=false;
  MainComment=false;
  Locked=false;
  Signed=false;
  NotFirstVolume=false;
  SFXSize=0;
  LatestTime.Reset();
  Protected=false;
  Encrypted=false;
  FailedHeaderDecryption=false;
  BrokenFileHeader=false;
  LastReadBlock=0;

  CurBlockPos=0;
  NextBlockPos=0;

  RecoveryPos=SIZEOF_MARKHEAD;
  RecoverySectors=-1;

  // FIXME obliterates vtable pointer
  memset((void*)&NewMhd,0,sizeof(NewMhd));
  NewMhd.HeadType=MAIN_HEAD;
  NewMhd.HeadSize=SIZEOF_NEWMHD;
  HeaderCRC=0;
  VolWrite=0;
  AddingFilesSize=0;
  AddingHeadersSize=0;
#if !defined(SHELL_EXT) && !defined(RAR_NOCRYPT)
  *HeadersSalt=0;
  *SubDataSalt=0;
#endif
  *FirstVolumeName=0;
  *FirstVolumeNameW=0;

  Splitting=false;
  NewArchive=false;

  SilentOpen=false;

}

Archive::Archive(const Archive &copy) :
    SubDataIO(), Cmd(), DummyCmd(), MarkHead(), OldMhd(), RecoverySectors(),
    RecoveryPos(), FailedHeaderDecryption(), LatestTime(), LastReadBlock(),
    CurHeaderType(), SilentOpen(), ShortBlock(), NewMhd(), NewLhd(),
    EndArcHead(), SubBlockHead(), SubHead(), CommHead(), ProtectHead(),
    AVHead(), SignHead(), UOHead(), MACHead(), EAHead(), StreamHead(),
    CurBlockPos(), NextBlockPos(), OldFormat(), Solid(), Volume(),
    MainComment(), Locked(), Signed(), NotFirstVolume(), Protected(),
    Encrypted(), SFXSize(), BrokenFileHeader(), Splitting(), HeaderCRC(),
    VolWrite(), AddingFilesSize(), AddingHeadersSize(), NewArchive(),
    FirstVolumeName(), FirstVolumeNameW()
{
    *this = copy;
}

const Archive& Archive::operator=(const Archive &src)
{
    File::operator=(src);
#if !defined(SHELL_EXT) && !defined(RAR_NOCRYPT)
    memcpy(HeadersSalt, src.HeadersSalt, sizeof(HeadersSalt));
#endif
#ifndef SHELL_EXT
    SubDataIO = src.SubDataIO;
    memcpy(SubDataSalt, src.SubDataSalt, sizeof(SubDataSalt));
#endif
    Cmd = src.Cmd;
    DummyCmd = src.DummyCmd;

    MarkHead = src.MarkHead;
    OldMhd = src.OldMhd;

    RecoverySectors = src.RecoverySectors;
    RecoveryPos = src.RecoveryPos;

    FailedHeaderDecryption = src.FailedHeaderDecryption;

    LatestTime = src.LatestTime;
    LastReadBlock = src.LastReadBlock;
    CurHeaderType = src.CurHeaderType;

    SilentOpen = src.SilentOpen;

    ShortBlock = src.ShortBlock;
    NewMhd = src.NewMhd;
    NewLhd = src.NewLhd;
    EndArcHead = src.EndArcHead;
    SubBlockHead = src.SubBlockHead;
    SubHead = src.SubHead;
    CommHead = src.CommHead;
    ProtectHead = src.ProtectHead;
    AVHead = src.AVHead;
    SignHead = src.SignHead;
    UOHead = src.UOHead;
    MACHead = src.MACHead;
    EAHead = src.EAHead;
    StreamHead = src.StreamHead;

    CurBlockPos = src.CurBlockPos;
    NextBlockPos = src.NextBlockPos;

    OldFormat = src.OldFormat;
    Solid = src.Solid;
    Volume = src.Volume;
    MainComment = src.MainComment;
    Locked = src.Locked;
    Signed = src.Signed;
    NotFirstVolume = src.NotFirstVolume;
    Protected = src.Protected;
    Encrypted = src.Encrypted;
    SFXSize = src.SFXSize;
    BrokenFileHeader = src.BrokenFileHeader;

    Splitting = src.Splitting;

    HeaderCRC = src.HeaderCRC;

    VolWrite = src.VolWrite;
    AddingFilesSize = src.AddingFilesSize;
    AddingHeadersSize = src.AddingHeadersSize;

    NewArchive = src.NewArchive;

    memcpy(FirstVolumeName, src.FirstVolumeName, sizeof(FirstVolumeName));
    memcpy(FirstVolumeNameW, src.FirstVolumeNameW, sizeof(FirstVolumeNameW));

    return *this;
}

#ifndef SHELL_EXT
void Archive::CheckArc(bool EnableBroken)
{
  if (!IsArchive(EnableBroken))
  {
    ErrHandler.Exit(FATAL_ERROR);
  }
}
#endif


#if !defined(SHELL_EXT) && !defined(SFX_MODULE)
void Archive::CheckOpen(const char *Name,const wchar *NameW)
{
  TOpen(Name,NameW);
  CheckArc(false);
}
#endif


bool Archive::WCheckOpen(const char *Name,const wchar *NameW)
{
  if (!WOpen(Name,NameW))
    return(false);
  if (!IsArchive(false))
  {
    Close();
    return(false);
  }
  return(true);
}

/**
Checks for RAR file signature (magic number).
*/
bool Archive::IsSignature(byte *D)
{
  bool Valid=false;
  if (D[0]==0x52)
  {
      if (D[1]==0x45 && D[2]==0x7e && D[3]==0x5e)
      {
          OldFormat=true;
          Valid=true;
      }
      else if (D[1]==0x61 && D[2]==0x72 && D[3]==0x21 && D[4]==0x1a && D[5]==0x07 && D[6]==0x00)
      {
          OldFormat=false;
          Valid=true;
      }
  }
  return(Valid);
}

/**
This function must be called after an archive has been instantiated. This sets up the 
archive to read from the <code>File</code> class.
@param ptrlocation - a location in memory where the RAR file begins
@param length - the length of the memory location that can be read
*/
void Archive::InitArc (byte *ptrlocation_, int64 length)
{
	//Need to set Mark header
	byte * originalloc = ptrlocation_;

	for( int i = 0; i < 7 && i < length; i++)
	{
		MarkHead.Mark[i] = *ptrlocation_;
		ptrlocation_++;
	}

	ptrlocation_ = originalloc; //reset the pointer location

	this->InitFile(ptrlocation_, length);
}

/**
Checks to make sure that an archive actually exists
@param EnableBroken - makes the archive opening process continue even when errors occur
*/
bool Archive::IsArchive(bool EnableBroken)
{
  Encrypted=false;
#ifndef SFX_MODULE
  if (IsDevice())
  {
    return(false);
  }
#endif
  if (Read(MarkHead.Mark,SIZEOF_MARKHEAD)!=SIZEOF_MARKHEAD)
    return(false);
  SFXSize=0;
  if (IsSignature(MarkHead.Mark))
  {
    if (OldFormat)
      Seek(0,SEEK_SET);
  }
  else
  {
    Array<char> Buffer(MAXSFXSIZE);
    long CurPos=(long)Tell();
    int ReadSize=Read(&Buffer[0],Buffer.Size()-16);
    for (int I=0;I<ReadSize;I++)
      if (Buffer[I]==0x52 && IsSignature((byte *)&Buffer[I]))
      {
        if (OldFormat && I>0 && CurPos<28 && ReadSize>31)
        {
          char *D=&Buffer[28-CurPos];
          if (D[0]!=0x52 || D[1]!=0x53 || D[2]!=0x46 || D[3]!=0x58)
            continue;
        }
        SFXSize=CurPos+I;
        Seek(SFXSize,SEEK_SET);
        if (!OldFormat)
          Read(MarkHead.Mark,SIZEOF_MARKHEAD);
        break;
      }
    if (SFXSize==0)
      return(false);
  }
  ReadHeader();
  SeekToNext();
#ifndef SFX_MODULE
  if (OldFormat)
  {
    NewMhd.Flags=OldMhd.Flags & 0x3f;
    NewMhd.HeadSize=OldMhd.HeadSize;
  }
  else
#endif
  {
    if (HeaderCRC!=NewMhd.HeadCRC)
    {
      if (!EnableBroken)
        return(false);
    }
  }
  Volume=(NewMhd.Flags & MHD_VOLUME);
  Solid=(NewMhd.Flags & MHD_SOLID)!=0;
  MainComment=(NewMhd.Flags & MHD_COMMENT)!=0;
  Locked=(NewMhd.Flags & MHD_LOCK)!=0;
  Signed=(NewMhd.PosAV!=0);
  Protected=(NewMhd.Flags & MHD_PROTECT)!=0;
  Encrypted=(NewMhd.Flags & MHD_PASSWORD)!=0;

  if (NewMhd.EncryptVer>UNP_VER)
  {
#ifdef RARDLL
    Cmd->DllError=ERAR_UNKNOWN_FORMAT;
#else
    ErrHandler.SetErrorCode(WARNING);
#endif
    return(false);
  }
#ifdef RARDLL
  // If callback function is not set, we cannot get the password,
  // so we skip the initial header processing for encrypted header archive.
  // It leads to skipped archive comment, but the rest of archive data
  // is processed correctly.
  if (Cmd->Callback==NULL)
    SilentOpen=true;
#endif

  // If not encrypted, we'll check it below.
  NotFirstVolume=Encrypted && (NewMhd.Flags & MHD_FIRSTVOLUME)==0;

  if (!SilentOpen || !Encrypted)
  {
    SaveFilePos SavePos(*this);
    int64 SaveCurBlockPos=CurBlockPos,SaveNextBlockPos=NextBlockPos;

    NotFirstVolume=false;
    while (ReadHeader()!=0)
    {
      int HeaderType=GetHeaderType();
      if (HeaderType==NEWSUB_HEAD)
      {
        if (SubHead.CmpName(SUBHEAD_TYPE_CMT))
          MainComment=true;
        if ((SubHead.Flags & LHD_SPLIT_BEFORE) ||
            (Volume && (NewMhd.Flags & MHD_FIRSTVOLUME)==0))
          NotFirstVolume=true;
      }
      else
      {
        if (HeaderType==FILE_HEAD && ((NewLhd.Flags & LHD_SPLIT_BEFORE)!=0 ||
            (Volume && NewLhd.UnpVer>=29 && (NewMhd.Flags & MHD_FIRSTVOLUME)==0)))
          NotFirstVolume=true;
        break;
      }
      SeekToNext();
    }
    CurBlockPos=SaveCurBlockPos;
    NextBlockPos=SaveNextBlockPos;
  }
  if (!Volume || !NotFirstVolume)
  {
    strcpy(FirstVolumeName,FileName);
    wcscpy(FirstVolumeNameW,FileNameW);
  }

  return(true);
}




void Archive::SeekToNext()
{
  Seek(NextBlockPos,SEEK_SET);
}


#ifndef SFX_MODULE
int Archive::GetRecoverySize(bool Required)
{
  if (!Protected)
    return(0);
  if (RecoverySectors!=-1 || !Required)
    return(RecoverySectors);
  SaveFilePos SavePos(*this);
  Seek(SFXSize,SEEK_SET);
  SearchSubBlock(SUBHEAD_TYPE_RR);
  return(RecoverySectors);
}
#endif




