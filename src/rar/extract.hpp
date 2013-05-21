#ifndef _RAR_EXTRACT_
#define _RAR_EXTRACT_

enum EXTRACT_ARC_CODE {EXTRACT_ARC_NEXT,EXTRACT_ARC_REPEAT};

class CmdExtract
{
  private:
    EXTRACT_ARC_CODE ExtractArchive(CommandData *Cmd, byte * ptrlocation, int64 length, std::string& xml);
    RarTime StartTime; // time when extraction started

    ComprDataIO DataIO;
    Unpack *Unp;
    unsigned long TotalFileCount;

    unsigned long FileCount;
    unsigned long MatchedArgs;
    bool FirstFile;
    bool AllMatchesExact;
    bool ReconstructDone;

    // If any non-zero solid file was successfully unpacked before current.
    // If true and if current encrypted file is broken, obviously
    // the password is correct and we can report broken CRC without
    // any wrong password hints.
    bool AnySolidDataUnpackedWell;

    char ArcName[NM];
    wchar ArcNameW[NM];

    wchar Password[MAXPASSWORD];
    bool PasswordAll;
    bool PrevExtracted;
    char DestFileName[NM];
    wchar DestFileNameW[NM];
    bool PasswordCancelled;
  public:
    CmdExtract();
    CmdExtract(const CmdExtract &copy);
    const CmdExtract& operator=(const CmdExtract &src);
    ~CmdExtract();
    void DoExtract(CommandData *Cmd, byte * ptrlocation, int64 length, std::string& xml);
    void ExtractArchiveInit(CommandData *Cmd,Archive &Arc);
    bool ExtractCurrentFile(CommandData *Cmd,Archive &Arc,size_t HeaderSize,
                            bool &Repeat, std::string& xml);
    static void UnstoreFile(ComprDataIO &DataIO,int64 DestUnpSize);

	void SetComprDataIO(ComprDataIO dataio);
    bool SignatureFound;
};

#endif
