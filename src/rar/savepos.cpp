#include "rar.hpp"


SaveFilePos::SaveFilePos(File &SaveFile_) :
    SaveFile(), SavePos(), CloseCount()
{
  SaveFilePos::SaveFile=&SaveFile_;
  SavePos=SaveFile_.Tell();
  CloseCount=SaveFile_.CloseCount;
  return;
}

SaveFilePos::SaveFilePos(const SaveFilePos &copy) :
    SaveFile(), SavePos(), CloseCount()
{
    *this = copy;
}

SaveFilePos::~SaveFilePos()
{
  if (CloseCount==SaveFile->CloseCount)
    SaveFile->Seek(SavePos,SEEK_SET);
}

const SaveFilePos& SaveFilePos::operator=(const SaveFilePos &source)
{
    SaveFile = source.SaveFile;
    SavePos = source.SavePos;
    CloseCount = source.CloseCount;

    return *this;
}
