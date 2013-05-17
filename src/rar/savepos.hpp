#ifndef _RAR_SAVEPOS_
#define _RAR_SAVEPOS_

class SaveFilePos
{
  private:
    File *SaveFile;
    int64 SavePos;
    uint CloseCount;
  public:
    //SaveFilePos(BenFile &SaveFile);
	SaveFilePos(File &SaveFile);
    SaveFilePos(const SaveFilePos &copy);
    ~SaveFilePos();
    const SaveFilePos& operator=(const SaveFilePos &source);
};

#endif
