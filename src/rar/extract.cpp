#include "rar.hpp"

using namespace std;

CmdExtract::CmdExtract() :
    StartTime(), DataIO(), Unp(), TotalFileCount(), FileCount(), MatchedArgs(),
    FirstFile(), AllMatchesExact(), ReconstructDone(),
    AnySolidDataUnpackedWell(), PasswordAll(), PrevExtracted(),
    PasswordCancelled(), SignatureFound()
{
  TotalFileCount=0;
  *Password=0;
  Unp=new Unpack(&DataIO);
  Unp->Init(NULL);
}

CmdExtract::CmdExtract(const CmdExtract &copy) :
    StartTime(), DataIO(), Unp(), TotalFileCount(), FileCount(), MatchedArgs(),
    FirstFile(), AllMatchesExact(), ReconstructDone(),
    AnySolidDataUnpackedWell(), PasswordAll(), PrevExtracted(),
    PasswordCancelled(), SignatureFound()
{
    *this = copy;
}

const CmdExtract& CmdExtract::operator=(const CmdExtract &src)
{
    StartTime = src.StartTime;
    DataIO = src.DataIO;
    Unp = src.Unp;
    TotalFileCount = src.TotalFileCount;
    FileCount = src.FileCount;
    MatchedArgs = src.MatchedArgs;
    FirstFile = src.FirstFile;
    AllMatchesExact = src.AllMatchesExact;
    ReconstructDone = src.ReconstructDone;
    AnySolidDataUnpackedWell = src.AnySolidDataUnpackedWell;
    memcpy(ArcName, src.ArcName, sizeof(ArcName));
    memcpy(ArcNameW, src.ArcNameW, sizeof(ArcNameW));
    memcpy(Password, src.Password, sizeof(Password));
    PasswordAll = src.PasswordAll;
    PrevExtracted = src.PrevExtracted;
    PasswordAll = src.PasswordAll;
    memcpy(DestFileName, src.DestFileName, sizeof(DestFileName));
    memcpy(DestFileNameW, src.DestFileNameW, sizeof(DestFileNameW));
    PasswordCancelled = src.PasswordCancelled;
    SignatureFound = src.SignatureFound;

    return *this;
}

CmdExtract::~CmdExtract()
{
  delete Unp;
  memset(Password,0,sizeof(Password));
}


void CmdExtract::DoExtract(CommandData *Cmd, byte* ptrlocation, int64 ptrlength, std::string& xml)
{
  PasswordCancelled=false;
  DataIO.SetCurrentCommand(*Cmd->Command);

  Cmd->ArcNames->Rewind();
  while (Cmd->GetArcName(ArcName,ArcNameW,ASIZE(ArcName)))
  {
    while (true)
    {
      wchar PrevCmdPassword[MAXPASSWORD];
      wcscpy(PrevCmdPassword,Cmd->Password);

      EXTRACT_ARC_CODE Code=ExtractArchive(Cmd, ptrlocation, ptrlength, xml);  //this is where the files are extracted
	
	
      // Restore Cmd->Password, which could be changed in IsArchive() call
      // for next header encrypted archive.
      wcscpy(Cmd->Password,PrevCmdPassword);

      if (Code!=EXTRACT_ARC_REPEAT)
        break;
    }
  }

  if (TotalFileCount==0 && *Cmd->Command!='I')
  {
    ErrHandler.SetErrorCode(NO_FILES_ERROR);
  }
#ifndef GUI
  else
      if (!Cmd->DisableDone)
      {
          if (*Cmd->Command=='I')
          {
              xml.append("\n<result>");
              xml.append(MDone);
              xml.append("<\\result>");
          }
          else
          {
              if (ErrHandler.GetErrorCount()==0)
              {
                  xml.append("\n<result>");
                  xml.append(MExtrAllOk);
                  xml.append("<\\result>\n");
              }
              else
              {
                  xml.append("\n<result>");
                  xml.append(MExtrTotalErr);
                  //xml.append(ErrHandler.GetErrorCount());
                  xml.append("<\\result>");
              }
          }
      }
#endif
}


void CmdExtract::ExtractArchiveInit(CommandData *Cmd,Archive &Arc)
{
  DataIO.UnpArcSize=Arc.FileLength();

  FileCount=0;
  MatchedArgs=0;
#ifndef SFX_MODULE
  FirstFile=true;
#endif

  if (*Cmd->Password!=0)
    wcscpy(Password,Cmd->Password);
  PasswordAll=(*Cmd->Password!=0);

  DataIO.UnpVolume=false;

  PrevExtracted=false;
  SignatureFound=false;
  AllMatchesExact=true;
  ReconstructDone=false;
  AnySolidDataUnpackedWell=false;

  StartTime.SetCurrentTime();
}


EXTRACT_ARC_CODE CmdExtract::ExtractArchive(CommandData *Cmd, byte *ptrlocation, int64 ptrlength, std::string& xml)
{
  Archive Arc(Cmd);
  Arc.InitArc(ptrlocation, ptrlength); //initialize the pointer location
  //void *myptr = Arc.GetPtrLocation(); //verify the pointer location when debugging

  if (!Arc.WOpen(ArcName,ArcNameW))
  {
    ErrHandler.SetErrorCode(OPEN_ERROR);
    return(EXTRACT_ARC_NEXT);
  }

#if 0
  if (!Arc.IsArchive(true))
  {
#ifndef GUI
    mprintf(St(MNotRAR),ArcName);
#endif
    if (CmpExt(ArcName,"rar"))
      ErrHandler.SetErrorCode(WARNING);
    return(EXTRACT_ARC_NEXT);
  }

  // Archive with corrupt encrypted header can be closed in IsArchive() call.
//  if (!Arc.IsOpened())
//    return(EXTRACT_ARC_NEXT);

#ifndef SFX_MODULE
  if (Arc.Volume && Arc.NotFirstVolume)
  {
    char FirstVolName[NM];
    VolNameToFirstName(ArcName,FirstVolName,(Arc.NewMhd.Flags & MHD_NEWNUMBERING)!=0);
    // If several volume names from same volume set are specified
    // and current volume is not first in set and first volume is present
    // and specified too, let's skip the current volume.
    if (stricomp(ArcName,FirstVolName)!=0 && FileExist(FirstVolName) &&
        Cmd->ArcNames->Search(FirstVolName,NULL,false))
      return(EXTRACT_ARC_NEXT);
  }
#endif

  int64 VolumeSetSize=0; // Total size of volumes after the current volume.

  if (Arc.Volume)
  {
    // Calculate the total size of all accessible volumes.
    // This size is necessary to display the correct total progress indicator.

    char NextName[NM];
    wchar NextNameW[NM];

    strcpy(NextName,Arc.FileName);
    wcscpy(NextNameW,Arc.FileNameW);

    while (true)
    {
      // First volume is already added to DataIO.TotalArcSize 
      // in initial TotalArcSize calculation in DoExtract.
      // So we skip it and start from second volume.
      NextVolumeName(NextName,NextNameW,ASIZE(NextName),(Arc.NewMhd.Flags & MHD_NEWNUMBERING)==0 || Arc.OldFormat);
      struct FindData FD;
      if (FindFile::FastFind(NextName,NextNameW,&FD))
        VolumeSetSize+=FD.Size;
      else
        break;
    }
    DataIO.TotalArcSize+=VolumeSetSize;
  }

  ExtractArchiveInit(Cmd,Arc);

  if (*Cmd->Command=='T' || *Cmd->Command=='I')
    Cmd->Test=true;


#ifndef GUI
  if (*Cmd->Command=='I')
    Cmd->DisablePercentage=true;
  else
    if (Cmd->Test)
      mprintf(St(MExtrTest),ArcName);
    else
	{
		//xml.append(St(MExtracting));
		xml.append("Extracting from: ");
		xml.append(ArcName);
      //mprintf(St(MExtracting),ArcName);
	}
#endif

  	//Arc.ViewComment();
#endif
	std::ostringstream ss;
#if 0
	
	/*Array<byte> CmtBuf;
	if(Arc.GetComment(&CmtBuf,NULL))
	{
		ss << "\n\n<Archive_Comment>" << (char*)&CmtBuf << "</Archive_Comment>\n\n";
	}
	xml.append(ss.str());*/
  // RAR can close a corrupt encrypted archive
//  if (!Arc.IsOpened())
//    return(EXTRACT_ARC_NEXT);
#endif


  while (1)
  {
    size_t Size=Arc.ReadHeader(); //Head error can occur here
    bool Repeat=false;
     if (!ExtractCurrentFile(Cmd,Arc,Size,Repeat, xml))
     {
      if (Repeat)
      {
        return(EXTRACT_ARC_REPEAT);
      }
      else
        break;
     }
     else {
         return(EXTRACT_ARC_NEXT);
     }
  }
	ss << "\n<TotalArcSize>" << DataIO.TotalArcSize << "</TotalArcSize>\n";
	xml.append(ss.str());
  return(EXTRACT_ARC_NEXT);
}




bool CmdExtract::ExtractCurrentFile(CommandData *Cmd,Archive &Arc,size_t HeaderSize,bool &Repeat, std::string& xml)
{
  char Command=*Cmd->Command;
  if (HeaderSize==0)
  {
      if (DataIO.UnpVolume)
      {
          return(false);
      }
      else
      {
          return(false);
      }
  }
  int HeadType=Arc.GetHeaderType();
  if (HeadType!=FILE_HEAD)
  {
    if (HeadType==AV_HEAD || HeadType==SIGN_HEAD)
      SignatureFound=true;
#if !defined(SFX_MODULE) && !defined(_WIN_CE)
    if (HeadType==SUB_HEAD && PrevExtracted)
    {
    }
#endif
    if (HeadType==NEWSUB_HEAD)
    {
      if (Arc.SubHead.CmpName(SUBHEAD_TYPE_AV))
        SignatureFound=true;
#if !defined(NOSUBBLOCKS) && !defined(_WIN_CE)
      if (PrevExtracted)
      {
      }
#endif
    }
    if (HeadType==ENDARC_HEAD)
    {
        if (Arc.EndArcHead.Flags & EARC_NEXT_VOLUME)
        {
            Arc.Seek(Arc.CurBlockPos,SEEK_SET);
            return(true);
        }
        else
        {
            return(false);
        }
    }
    Arc.SeekToNext();
    return(true);
  }
  PrevExtracted=false;

  if (SignatureFound || (!Cmd->Recurse && MatchedArgs>=Cmd->FileArgs->ItemsCount() && AllMatchesExact))
  {
      return(false);
  }

  char ArcFileName[NM];
  IntToExt(Arc.NewLhd.FileName,Arc.NewLhd.FileName);
  strcpy(ArcFileName,Arc.NewLhd.FileName);

  wchar ArcFileNameW[NM];
  *ArcFileNameW=0;

  int MatchType=MATCH_WILDSUBPATH;

  bool EqualNames=false;
  int MatchNumber=Cmd->IsProcessFile(Arc.NewLhd,&EqualNames,MatchType);
  bool ExactMatch=MatchNumber!=0;
#if !defined(SFX_MODULE) && !defined(_WIN_CE)
  if (Cmd->ExclPath==EXCL_BASEPATH)
  {
    *Cmd->ArcPath=0;
    if (ExactMatch)
    {
      Cmd->FileArgs->Rewind();
      if (Cmd->FileArgs->GetString(Cmd->ArcPath,NULL,sizeof(Cmd->ArcPath),MatchNumber-1))
        *PointToName(Cmd->ArcPath)=0;
    }
  }
#endif
  if (ExactMatch && !EqualNames)
    AllMatchesExact=false;

#ifdef UNICODE_SUPPORTED
  bool WideName=(Arc.NewLhd.Flags & LHD_UNICODE) && UnicodeEnabled();
#else
  bool WideName=false;
#endif

#ifdef _APPLE
  if (WideName)
  {
    // Prepare UTF-8 name for OS X. Since we are sure that destination
    // is UTF-8, we can avoid calling the less reliable WideToChar function.
    WideToUtf(Arc.NewLhd.FileNameW,ArcFileName,ASIZE(ArcFileName));
    WideName=false;
  }
#endif

  wchar *DestNameW=WideName ? DestFileNameW:NULL;
  (void) DestNameW;

#ifdef UNICODE_SUPPORTED
  if (WideName)
  {
    // Prepare the name in single byte native encoding (typically UTF-8
    // for Unix-based systems). Windows does not really need it,
    // but Unix system will use this name instead of Unicode.
    ConvertPath(Arc.NewLhd.FileNameW,ArcFileNameW);
    char Name[NM];
    if (WideToChar(ArcFileNameW,Name) && IsNameUsable(Name))
      strcpy(ArcFileName,Name);
  }
#endif

  ConvertPath(ArcFileName,ArcFileName);

  if (Arc.IsArcLabel())
    return(true);

  if (Arc.NewLhd.Flags & LHD_VERSION)
  {
    if (Cmd->VersionControl!=1 && !EqualNames)
    {
      if (Cmd->VersionControl==0)
        ExactMatch=false;
      int Version=ParseVersionFileName(ArcFileName,ArcFileNameW,false);
      if (Cmd->VersionControl-1==(unsigned) Version)
        ParseVersionFileName(ArcFileName,ArcFileNameW,true);
      else
        ExactMatch=false;
    }
  }
  else
    if (!Arc.IsArcDir() && Cmd->VersionControl>1)
      ExactMatch=false;

  Arc.ConvertAttributes();

#ifndef SFX_MODULE
  if ((Arc.NewLhd.Flags & LHD_SPLIT_BEFORE)!=0 && FirstFile)
  {
    char CurVolName[NM];
    strcpy(CurVolName,ArcName);

    bool NewNumbering=(Arc.NewMhd.Flags & MHD_NEWNUMBERING)!=0;
    VolNameToFirstName(ArcName,ArcName,NewNumbering);
    if (*ArcNameW!=0)
      VolNameToFirstName(ArcNameW,ArcNameW,NewNumbering);

    if (stricomp(ArcName,CurVolName)!=0 && FileExist(ArcName,ArcNameW))
    {
      // If first volume name does not match the current name and if
      // such volume name really exists, let's unpack from this first volume.
      Repeat=true;
      return(false);
    }
#if !defined(RARDLL) && !defined(_WIN_CE)
    if (!ReconstructDone)
    {
      ReconstructDone=true;
    }
#endif
    strcpy(ArcName,CurVolName);
  }
#endif
  DataIO.UnpVolume=(Arc.NewLhd.Flags & LHD_SPLIT_AFTER)!=0;
  DataIO.NextVolumeMissing=false;

  Arc.Seek(Arc.NextBlockPos-Arc.NewLhd.FullPackSize,SEEK_SET);

  bool TestMode=false;
  bool ExtrFile=false;
  bool SkipSolid=false;

#ifndef SFX_MODULE
  if (FirstFile && (ExactMatch || Arc.Solid) && (Arc.NewLhd.Flags & (LHD_SPLIT_BEFORE/*|LHD_SOLID*/))!=0)
  {
    if (ExactMatch)
    {
#ifdef RARDLL
      Cmd->DllError=ERAR_BAD_DATA;
#endif
      ErrHandler.SetErrorCode(OPEN_ERROR);
    }
    ExactMatch=false;
  }

  FirstFile=false;
#endif

  if (ExactMatch || (SkipSolid=Arc.Solid)!=0)
  {
    if ((Arc.NewLhd.Flags & LHD_PASSWORD)!=0)
        return 0;
#if !defined(GUI) && !defined(SILENT)
      else
        if (!PasswordAll && (!Arc.Solid || (Arc.NewLhd.UnpVer>=20 && (Arc.NewLhd.Flags & LHD_SOLID)==0)))
        {
        }
#endif

#ifndef SFX_MODULE
    if (*Cmd->ExtrPath==0 && *Cmd->ExtrPathW!=0)
      WideToChar(Cmd->ExtrPathW,DestFileName);
    else
#endif
      strcpy(DestFileName,Cmd->ExtrPath);


#ifndef SFX_MODULE
    if (Cmd->AppendArcNameToPath)
    {
      strcat(DestFileName,PointToName(Arc.FirstVolumeName));
      SetExt(DestFileName,NULL);
      AddEndSlash(DestFileName);
    }
#endif

    char *ExtrName=ArcFileName;

    bool EmptyName=false;
#ifndef SFX_MODULE
    size_t Length=strlen(Cmd->ArcPath);
    if (Length>1 && IsPathDiv(Cmd->ArcPath[Length-1]) &&
        strlen(ArcFileName)==Length-1)
      Length--;
    if (Length>0 && strnicomp(Cmd->ArcPath,ArcFileName,Length)==0)
    {
      ExtrName+=Length;
      while (*ExtrName==CPATHDIVIDER)
        ExtrName++;
      if (*ExtrName==0)
        EmptyName=true;
    }
#endif

    // Use -ep3 only in systems, where disk letters are exist, not in Unix.
    bool AbsPaths=Cmd->ExclPath==EXCL_ABSPATH && Command=='X' && IsDriveDiv(':');

    // We do not use any user specified destination paths when extracting
    // absolute paths in -ep3 mode.
    if (AbsPaths)
      *DestFileName=0;

    if (Command=='E' || Cmd->ExclPath==EXCL_SKIPWHOLEPATH)
      strcat(DestFileName,PointToName(ExtrName));
    else
      strcat(DestFileName,ExtrName);

    char DiskLetter=etoupper(DestFileName[0]);

    if (AbsPaths)
    {
      if (DestFileName[1]=='_' && IsPathDiv(DestFileName[2]) &&
          DiskLetter>='A' && DiskLetter<='Z')
        DestFileName[1]=':';
      else
        if (DestFileName[0]=='_' && DestFileName[1]=='_')
        {
          // Convert __server\share to \\server\share.
          DestFileName[0]=CPATHDIVIDER;
          DestFileName[1]=CPATHDIVIDER;
        }
    }

#ifndef SFX_MODULE
    if (!WideName && *Cmd->ExtrPathW!=0)
    {
      DestNameW=DestFileNameW;
      WideName=true;
      CharToWide(ArcFileName,ArcFileNameW);
    }
#endif

    if (WideName)
    {
      if (*Cmd->ExtrPathW!=0)
        wcscpy(DestFileNameW,Cmd->ExtrPathW);
      else
        CharToWide(Cmd->ExtrPath,DestFileNameW);

#ifndef SFX_MODULE
      if (Cmd->AppendArcNameToPath)
      {
        wchar FileNameW[NM];
        if (*Arc.FirstVolumeNameW!=0)
          wcscpy(FileNameW,Arc.FirstVolumeNameW);
        else
          CharToWide(Arc.FirstVolumeName,FileNameW);
        wcscat(DestFileNameW,PointToName(FileNameW));
        SetExt(DestFileNameW,NULL);
        AddEndSlash(DestFileNameW);
      }
#endif
      wchar *ExtrNameW=ArcFileNameW;
#ifndef SFX_MODULE
      if (Length>0)
      {
        wchar ArcPathW[NM];
        GetWideName(Cmd->ArcPath,Cmd->ArcPathW,ArcPathW,ASIZE(ArcPathW));
        Length=wcslen(ArcPathW);
      }
      ExtrNameW+=Length;
      while (*ExtrNameW==CPATHDIVIDER)
        ExtrNameW++;
#endif

      if (AbsPaths)
        *DestFileNameW=0;

      if (Command=='E' || Cmd->ExclPath==EXCL_SKIPWHOLEPATH)
        wcscat(DestFileNameW,PointToName(ExtrNameW));
      else
        wcscat(DestFileNameW,ExtrNameW);

      if (AbsPaths && DestFileNameW[1]=='_' && IsPathDiv(DestFileNameW[2]))
        DestFileNameW[1]=':';
    }
    else
      *DestFileNameW=0;

    ExtrFile=!SkipSolid && !EmptyName && (Arc.NewLhd.Flags & LHD_SPLIT_BEFORE)==0;

    if ((Cmd->FreshFiles || Cmd->UpdateFiles) && (Command=='E' || Command=='X'))
    {
    }

    // Skip encrypted file if no password is specified.
    if ((Arc.NewLhd.Flags & LHD_PASSWORD)!=0 && *Password==0)
    {
      ErrHandler.SetErrorCode(WARNING);
#ifdef RARDLL
      Cmd->DllError=ERAR_MISSING_PASSWORD;
#endif
      ExtrFile=false;
    }

#ifdef RARDLL
    if (*Cmd->DllDestName)
    {
      strncpyz(DestFileName,Cmd->DllDestName,ASIZE(DestFileName));
      *DestFileNameW=0;
      if (Cmd->DllOpMode!=RAR_EXTRACT)
        ExtrFile=false;
    }
    if (*Cmd->DllDestNameW)
    {
      wcsncpyz(DestFileNameW,Cmd->DllDestNameW,ASIZE(DestFileNameW));
      DestNameW=DestFileNameW;
      if (Cmd->DllOpMode!=RAR_EXTRACT)
        ExtrFile=false;
    }
#endif

#ifdef SFX_MODULE
    if ((Arc.NewLhd.UnpVer!=UNP_VER && Arc.NewLhd.UnpVer!=29) &&
        Arc.NewLhd.Method!=0x30)
#else
    if (Arc.NewLhd.UnpVer<13 || Arc.NewLhd.UnpVer>UNP_VER)
#endif
    {
      ExtrFile=false;
      ErrHandler.SetErrorCode(WARNING);
#ifdef RARDLL
      Cmd->DllError=ERAR_UNKNOWN_FORMAT;
#endif
    }

    File CurFile;

    if (!IsLink(Arc.NewLhd.SubFlags))
    {
        if (Arc.IsArcDir())
        {
            return(true);
        }
        else
        {
            if (Cmd->Test && ExtrFile)
            {
                TestMode=true;
            }
#if !defined(GUI) && !defined(SFX_MODULE)
            if (Command=='P' && ExtrFile)
            {
                CurFile.SetHandleType(FILE_HANDLESTD);
            }
#endif
            if ((Command=='E' || Command=='X') && ExtrFile && !Cmd->Test)
            {
            }
        }
    }

    if (!ExtrFile && Arc.Solid)
    {
      SkipSolid=true;
      TestMode=true;
      ExtrFile=true;

    }
    if (ExtrFile)
    {

      if (!SkipSolid)
      {
        if (!TestMode && Command!='P' && CurFile.IsDevice())
        {
          ErrHandler.WriteError(Arc.FileName,Arc.FileNameW,DestFileName,DestFileNameW);
        }
        TotalFileCount++;
      }
      FileCount++;
	string theos = "";
	ostringstream ss;
#ifndef GUI
      if (Command=='I')
          switch(Cmd->Test ? 'T':Command)
          {
            case 'T':
              break;
#ifndef SFX_MODULE
            case 'P':
              //mprintf(St(MExtrPrinting),ArcFileName); //Prints File name to stdout
			  //xml.append(St(MExtrPrinting));

	if (Arc.NewLhd.HostOS == HOST_MSDOS)
		theos = "MSDOS";
	else if (Arc.NewLhd.HostOS == HOST_OS2)
		theos = "OS2";
	else if (Arc.NewLhd.HostOS == HOST_WIN32)
		theos = "Win32";
	else if (Arc.NewLhd.HostOS == HOST_UNIX)
		theos = "UNIX";
	else if (Arc.NewLhd.HostOS == HOST_MACOS)
		theos = "MACOS";
	else if (Arc.NewLhd.HostOS == HOST_BEOS)
		theos = "BEOS";
	else    theos = "Other OS";


	ss << "\n<File>\n<FileName>" << Arc.NewLhd.FileName << "</FileName>\n<Name_Size>" 
		<< Arc.NewLhd.NameSize << "</Name_Size>\n<Unpack_Size>" 
		<< Arc.NewLhd.UnpSize << "</Unpack_Size>\n<Data_Size>"
		<< Arc.NewLhd.DataSize << "</Data_Size\n<Pack_Size>"
		<< Arc.NewLhd.DataSize << "</Pack_Size>\n<High_Pack_Size>"
		<< Arc.NewLhd.HighPackSize << "</High_Pack_Size>\n<High_Unp_Size>"
		<< Arc.NewLhd.HighUnpSize << "</High_Unp_Size>\n<Host_OS>" 
		<< theos << "</Host_OS>\n<Method>" 
		<< Arc.NewLhd.Method << "</Method>\n<File_CRC>" 
		<< Arc.NewLhd.FileCRC << "</File_CRC>\n<File_Time>" 
		<< Arc.NewLhd.FileTime << "</File_Time>\n<Unpack_Version>" 
		<< Arc.NewLhd.UnpVer << "</Unpack_Version>\n";
	ss << "<TimeInfo><mtime>" << Arc.NewLhd.mtime.GetLocalTimeXML() 
		<< "</mtime>\n<ctime>" << Arc.NewLhd.ctime.GetLocalTimeXML() 
		<< "</ctime>\n<atime>" << Arc.NewLhd.atime.GetLocalTimeXML()
		<< "</atime>\n<arctime>" << Arc.NewLhd.arctime.GetLocalTimeXML()
		<< "</arctime>\n</TimeInfo>";
	ss << "</File>\n\n";
	xml.append(ss.str());
				//xml.append("\n<file>");
			  //xml.append(ArcFileName);
			  //xml.append("<\\file>");
			  //xml.append("\n");
              break;
#endif
            case 'X':
            case 'E':
              break;
          }
#endif
      DataIO.CurUnpRead=0;
      DataIO.CurUnpWrite=0;
      DataIO.UnpFileCRC=Arc.OldFormat ? 0 : 0xffffffff;
      DataIO.PackedCRC=0xffffffff;

      wchar FilePassword[MAXPASSWORD];
#ifdef _WIN_ALL
      if (Arc.NewLhd.HostOS==HOST_MSDOS/* && Arc.NewLhd.UnpVer<=25*/)
      {
        // We need the password in OEM encoding if file was encrypted by
        // native RAR/DOS (not extender based). Let's make the conversion.
        char PswA[MAXPASSWORD];
        CharToOemBuffW(Password,PswA,ASIZE(PswA));
        CharToWide(PswA,FilePassword,ASIZE(FilePassword));
        FilePassword[ASIZE(FilePassword)-1]=0;
      }
      else
#endif
        wcscpy(FilePassword,Password);
      
      DataIO.SetEncryption(
        (Arc.NewLhd.Flags & LHD_PASSWORD)!=0 ? Arc.NewLhd.UnpVer:0,FilePassword,
        (Arc.NewLhd.Flags & LHD_SALT)!=0 ? Arc.NewLhd.Salt:NULL,false,
        Arc.NewLhd.UnpVer>=36);
      DataIO.SetPackedSizeToRead(Arc.NewLhd.FullPackSize);
      DataIO.SetFiles(&Arc,&CurFile);
      DataIO.SetTestMode(TestMode);
      DataIO.SetSkipUnpCRC(SkipSolid);
#ifndef _WIN_CE
      if (!TestMode && !Arc.BrokenFileHeader &&
          (Arc.NewLhd.FullPackSize<<11)>Arc.NewLhd.FullUnpSize &&
          (Arc.NewLhd.FullUnpSize<100000000 || Arc.FileLength()>Arc.NewLhd.FullPackSize))
        CurFile.Prealloc(Arc.NewLhd.FullUnpSize);
#endif

      CurFile.SetAllowDelete(!Cmd->KeepBroken);

      bool LinkCreateMode=!Cmd->Test && !SkipSolid;
      if (false)
      {
          PrevExtracted=LinkCreateMode;
      }
      else
      {
          if ((Arc.NewLhd.Flags & LHD_SPLIT_BEFORE)==0)
          {
              if (Arc.NewLhd.Method==0x30)
              {
                  UnstoreFile(DataIO,Arc.NewLhd.FullUnpSize);
              }
              else
              {
                  Unp->SetDestSize(Arc.NewLhd.FullUnpSize);
                  if (Arc.NewLhd.UnpVer<=15)
                  {
                      Unp->DoUnpack(15,FileCount>1 && Arc.Solid);
                  }
                  else
                  {
                      Unp->DoUnpack(Arc.NewLhd.UnpVer,(Arc.NewLhd.Flags & LHD_SOLID)!=0); //unpacking of the file
                  }
              }
          }
      }

//      if (Arc.IsOpened())
        Arc.SeekToNext();

      bool ValidCRC=(Arc.OldFormat && UINT32(DataIO.UnpFileCRC)==UINT32(Arc.NewLhd.FileCRC)) ||
                   (!Arc.OldFormat && UINT32(DataIO.UnpFileCRC)==UINT32(Arc.NewLhd.FileCRC^0xffffffff));

      // We set AnySolidDataUnpackedWell to true if we found at least one
      // valid non-zero solid file in preceding solid stream. If it is true
      // and if current encrypted file is broken, we do not need to hint
      // about a wrong password and can report CRC error only.
      if ((Arc.NewLhd.Flags & LHD_SOLID)==0)
        AnySolidDataUnpackedWell=false; // Reset the flag, because non-solid file is found.
      else
        if (Arc.NewLhd.Method!=0x30 && Arc.NewLhd.FullUnpSize>0 && ValidCRC)
          AnySolidDataUnpackedWell=true;
 
      bool BrokenFile=false;
      if (!SkipSolid)
      {
        if (ValidCRC)
        {
        }
        else
        {
          if ((Arc.NewLhd.Flags & LHD_PASSWORD)!=0 && !AnySolidDataUnpackedWell)
          {
          }
          else
          {
          }
          BrokenFile=true;
          ErrHandler.SetErrorCode(CRC_ERROR);
#ifdef RARDLL
          Cmd->DllError=ERAR_BAD_DATA;
#endif
        }
      }
#ifndef GUI
      else
        {/*mprintf("\b\b\b\b\b     ");*/}
#endif

      if (!TestMode && (Command=='X' || Command=='E') &&
          !IsLink(Arc.NewLhd.SubFlags))
      {
#if defined(_WIN_ALL) || defined(_EMX)
        if (Cmd->ClearArc)
          Arc.NewLhd.SubFlags&=~FA_ARCH;
#endif
        if (!BrokenFile || Cmd->KeepBroken)
        {
          if (BrokenFile)
            CurFile.Truncate();
          CurFile.SetOpenFileTime(
            Cmd->xmtime==EXTTIME_NONE ? NULL:&Arc.NewLhd.mtime,
            Cmd->xctime==EXTTIME_NONE ? NULL:&Arc.NewLhd.ctime,
            Cmd->xatime==EXTTIME_NONE ? NULL:&Arc.NewLhd.atime);
          CurFile.Close();
          CurFile.SetCloseFileTime(
            Cmd->xmtime==EXTTIME_NONE ? NULL:&Arc.NewLhd.mtime,
            Cmd->xatime==EXTTIME_NONE ? NULL:&Arc.NewLhd.atime);
          if (!Cmd->IgnoreGeneralAttr)
            SetFileAttr(CurFile.FileName,CurFile.FileNameW,Arc.NewLhd.SubFlags);
          PrevExtracted=true;
        }
      }
    }
  }
  if (ExactMatch)
    MatchedArgs++;
  if (DataIO.NextVolumeMissing/* || !Arc.IsOpened()*/)
    return(false);
  if (!ExtrFile)
  {
      if (!Arc.Solid)
      {
          Arc.SeekToNext();
      }
      else if (!SkipSolid)
      {
          return(false);
      }
  }
  return(true);
}


void CmdExtract::UnstoreFile(ComprDataIO &DataIO,int64 DestUnpSize)
{
  Array<byte> Buffer(0x10000);
  while (1)
  {
    uint Code=DataIO.UnpRead(&Buffer[0],Buffer.Size());
    if (Code==0 || (int)Code==-1)
      break;
    Code=Code<DestUnpSize ? Code:(uint)DestUnpSize;
    DataIO.UnpWrite(&Buffer[0],Code);
    if (DestUnpSize>=0)
      DestUnpSize-=Code;
  }
}

void CmdExtract::SetComprDataIO(ComprDataIO dataio)
{
	DataIO = dataio;
}

