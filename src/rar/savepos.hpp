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
    ~SaveFilePos();
};

#endif
