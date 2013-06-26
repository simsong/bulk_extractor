#include "rar.hpp"

RAROptions::RAROptions() :
    ExclFileAttr(), InclFileAttr(), InclAttrSet(), WinSize(), TempPath(),
    ConfigDisabled(), ExtrPath(), ExtrPathW(), CommentFile(), CommentFileW(),
    CommentCharset(), FilelistCharset(), ArcPath(), ArcPathW(), Password(),
    EncryptHeaders(), LogName(), MsgStream(), Sound(), Overwrite(), Method(),
    Recovery(), RecVolNumber(), DisablePercentage(), DisableCopyright(),
    DisableDone(), Solid(), SolidCount(), ClearArc(), AddArcOnly(), AV(),
    DisableComment(), FreshFiles(), UpdateFiles(), ExclPath(), Recurse(),
    VolSize(), NextVolSizes(), CurVolNum(), AllYes(), DisableViewAV(),
    DisableSortSolid(), ArcTime(), ConvertNames(), ProcessOwners(),
    SaveLinks(), Priority(), SleepTime(), KeepBroken(), OpenShared(),
    DeleteFiles(),
#ifndef SFX_MODULE
    GenerateArcName(), GenerateMask(),
#endif
    SyncFiles(), ProcessEA(), SaveStreams(), SetCompressedAttr(),
    IgnoreGeneralAttr(), FileTimeBefore(), FileTimeAfter(), FileSizeLess(),
    FileSizeMore(), OldNumbering(), Lock(), Test(), VolumePause(),
    FilterModes(), EmailTo(), VersionControl(), NoEndBlock(),
    AppendArcNameToPath(), Shutdown(), xmtime(), xctime(), xatime(),
    xarctime(), CompressStdin()
#ifdef PACK_SMP
    ,
    Threads()
#endif
#ifdef RARDLL
    ,
    DllDestName(), DllDestNameW(), DllOpMode(), DllError(), UserData(),
    Callback(), ChangeVolProc(), ProcessDataProc()
#endif
{
  Init();
}


RAROptions::~RAROptions()
{
  // It is important for security reasons, so we do not have the unnecessary
  // password data left in memory.
  memset((void*)this,0,sizeof(RAROptions));
}

const RAROptions& RAROptions::operator=(const RAROptions &src)
{
    ExclFileAttr = src.ExclFileAttr;
    InclFileAttr = src.InclFileAttr;
    InclAttrSet = src.InclAttrSet;
    WinSize = src.WinSize;
    memcpy(TempPath, src.TempPath, sizeof(TempPath));
    ConfigDisabled = src.ConfigDisabled;
    memcpy(ExtrPath, src.ExtrPath, sizeof(ExtrPath));
    memcpy(ExtrPathW, src.ExtrPathW, sizeof(ExtrPathW));
    memcpy(CommentFile, src.CommentFile, sizeof(CommentFile));
    memcpy(CommentFileW, src.CommentFileW, sizeof(CommentFileW));
    CommentCharset = src.CommentCharset;
    FilelistCharset = src.FilelistCharset;
    memcpy(ArcPath, src.ArcPath, sizeof(ArcPath));
    memcpy(ArcPathW, src.ArcPathW, sizeof(ArcPathW));
    memcpy(Password, src.Password, sizeof(Password));
    EncryptHeaders = src.EncryptHeaders;
    memcpy(LogName, src.LogName, sizeof(LogName));
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
    memcpy(GenerateMask, src.GenerateMask, sizeof(GenerateMask));
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
    memcpy(FilterModes, src.FilterModes, sizeof(FilterModes));
    memcpy(EmailTo, src.EmailTo, sizeof(EmailTo));
    VersionControl = src.VersionControl;
    NoEndBlock = src.NoEndBlock;
    AppendArcNameToPath = src.AppendArcNameToPath;
    Shutdown = src.Shutdown;
    xmtime = src.xmtime;
    xctime = src.xctime;
    xatime = src.xatime;
    xarctime = src.xarctime;
    memcpy(CompressStdin, src.CompressStdin, sizeof(CompressStdin));
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
  // FIXME obliterating vtable pointer
  memset((void*)this,0,sizeof(RAROptions));
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
