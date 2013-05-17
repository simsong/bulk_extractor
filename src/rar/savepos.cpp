#include "rar.hpp"


SaveFilePos::SaveFilePos(File &SaveFile)
{
  SaveFilePos::SaveFile=&SaveFile;
  SavePos=SaveFile.Tell();
  CloseCount=SaveFile.CloseCount;
  return;
}

SaveFilePos::SaveFilePos(const SaveFilePos &copy)
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
}
