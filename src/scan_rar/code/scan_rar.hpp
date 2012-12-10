#ifndef _SCANRAR_
#define _SCANRAR_

#include <vector>
/**
A class to hold some of the functions necessary to parse a RAR file.
*/
using namespace std;
class ScanRar
{
  public:
	ScanRar(){}; //dummy constructor
	void StartWithoutPassword(std::vector<char>& filebytes, int location, int length, const unsigned int myunpacksize);
	void StartWithPassword(std::vector<char>& filebytes, int location, int length, const unsigned int myunpacksize,char* pword);
	void StartDecompression(std::vector<char>& filebytes, int location, int length, const unsigned int myunpacksize, char ** parameters, int numparameters);
	void PrintResults(unsigned int amount, byte *unpackedfiles);
};


#endif
