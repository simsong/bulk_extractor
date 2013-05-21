#include "rar.hpp"

RAROptions::RAROptions()
{
  Init();
}


RAROptions::~RAROptions()
{
  // It is important for security reasons, so we do not have the unnecessary
  // password data left in memory.
  memset(this,0,sizeof(RAROptions));
}

const RAROptions& RAROptions::operator=(const RAROptions &src)
{
    ExclFileAttr = src.ExclFileAttr;
    InclFileAttr = src.InclFileAttr;
    InclAttrSet = src.InclAttrSet;
    WinSize = src.WinSize;
    TempPath = src.TempPath;
    ConfigDisabled = src.ConfigDisabled;
    ExtrPath = src.ExtrPath;
    ExtrPathW = src.ExtrPathW;
    CommentFile = src.CommentFile;
    CommentFileW = src.CommentFileW;
    CommentCharset = src.CommentCharset;
    FilelistCharset = src.FilelistCharset;
    ArcPath = src.ArcPath;
    ArcPathW = src.ArcPathW;
    Password = src.Password;
    EncryptHeaders = src.EncryptHeaders;
    LogName = src.LogName;
    MsgStream = src.MsgStream;
    Sound = src.Sound;
    Overwrite = src.Overwrite;
    Method = src.Method;
    Recovery = src.Recovery;
    RecVolNumber = src.RecVolNumber;
    DisablePercentage = src.DisablePercentage;
    DisableCopyright = src.DisableCopyright;
    DisableDone = src.DisableDone;
    Solid = src.Solid;
    SolidCount = src.SolidCount;
    ClearArc = src.ClearArc;
    AddArcOnly = src.AddArcOnly;
    AV = src.AV;
    DisableComment = src.DisableComment;
    FreshFiles = src.FreshFiles;
    UpdateFiles = src.UpdateFiles;
    ExclPath = src.ExclPath;
    Recurse = src.Recurse;
    VolSize = src.VolSize;
    NextVolSizes = src.NextVolSizes;
    CurVolNum = src.CurVolNum;
    AllYes = src.AllYes;
    DisableViewAV = src.DisableViewAV;
    DisableSortSolid = src.DisableSortSolid;
    ArcTime = src.ArcTime;
    ConvertNames = src.ConvertNames;
    ProcessOwners = src.ProcessOwners;
    SaveLinks = src.SaveLinks;
    Priority = src.Priority;
    SleepTime = src.SleepTime;
    KeepBroken = src.KeepBroken;
    OpenShared = src.OpenShared;
    DeleteFiles = src.DeleteFiles;
#ifndef SFX_MODULE
    GenerateArcName = src.GenerateArcName;
    GenerateMask = src.GenerateMask;
#endif
    SyncFiles = src.SyncFiles;
    ProcessEA = src.ProcessEA;
    SaveStreams = src.SaveStreams;
    SetCompressedAttr = src.SetCompressedAttr;
    IgnoreGeneralAttr = src.IgnoreGeneralAttr;
    FileTimeBefore = src.FileTimeBefore;
    FileTimeAfter = src.FileTimeAfter;
    FileSizeLess = src.FileSizeLess;
    FileSizeMore = src.FileSizeMore;
    OldNumbering = src.OldNumbering;
    Lock = src.Lock;
    Test = src.Test;
    VolumePause = src.VolumePause;
    FilterModes = src.FilterModes;
    EmailTo = src.EmailTo;
    VersionControl = src.VersionControl;
    NoEndBlock = src.NoEndBlock;
    AppendArcNameToPath = src.AppendArcNameToPath;
    Shutdown = src.Shutdown;
    xmtime = src.xmtime;
    xctime = src.xctime;
    xatime = src.xatime;
    xarctime = src.xarctime;
    CompressStdin = src.CompressStdin;
#ifdef PACK_SMP
    Threads = src.Threads;
#endif
#ifdef RARDLL
    DllDestName = src.DllDestName;
    DllDestNameW = src.DllDestNameW;
    DllOpMode = src.DllOpMode;
    DllError = src.DllError;
    UserData = src.UserData;
    Callback = src.Callback;
    ChangeVolProc = src.ChangeVolProc;
    ProcessDataProc = src.ProcessDataProc;
#endif
    return *this;
}

void RAROptions::Init()
{
  memset(this,0,sizeof(RAROptions));
  WinSize=0x400000;
  Overwrite=OVERWRITE_DEFAULT;
  Method=3;
  MsgStream=MSG_STDOUT;
  ConvertNames=NAMES_ORIGINALCASE;
  ProcessEA=true;
  xmtime=EXTTIME_HIGH3;
  CurVolNum=0;
  FileSizeLess=INT64NDF;
  FileSizeMore=INT64NDF;
}
