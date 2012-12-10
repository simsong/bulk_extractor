
#include "rar.hpp"
#include  <iostream>
#include <cstdio>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iterator>
using namespace std;

const unsigned int myunpacksize = 99999; //This should be the length of the buffer in bulk_extractor, and the max size that a RAR file can open up

/**
@author Benjamin Salazar
@company JHU/APL
@email benjamin.salazar@jhuapl.edu

@License - this software is open source and needs to comply with the RAR file license.
This special license can be located in the <b>license.txt</b> file. This program in no way
 attempts to discover the RAR file algorithm or attempt to implement the RAR file
 algorithm. This software only attempts of open RAR files, as described in the special
 license.

 Start of the software
 @param argv - Arguments to run the software. The first argument is the file that should be read. The second optional parameter is the password to decrypt the archive.
**/
int main(int argc, char *argv[])
{
	bool password = false;
	if(argc < 2)
	{
		cout << "Not enough arguments. Exiting. Should be " << argv[0] << " RARFileName [password]" << endl;
		return 1;
	}

	if (argc == 3)
	{
		password = true;
	}

	if(argc > 3)
	{
		cout << "Too many arguments. Exiting. Should be " << argv[0] << " RARFileName [password]" << endl;
		return 1;
	}

	char* thearchivefile = argv[1];
	ifstream myfile;
	myfile.open(thearchivefile,ios::binary | ios::in);
	ScanRar rarfunction;

	if(myfile.is_open())
	{
		//cout << "Successful opening" << endl;

		//Get length of file
		myfile.seekg(0,ios::end);
		int length = (int)myfile.tellg(); //Gets length of file to import
		myfile.seekg(0,ios::beg);

		//setup variables
		std::vector<char> newbytes = vector<char>();
		char *oldbytes = new char[length];

		//read data
		myfile.read(oldbytes, length); //Read in the bytes to variable 'b'
		myfile.clear(); //clear any errors
		myfile.close();//the file is in memory, now we can close th file and let the OS have it back

		std::copy(oldbytes, oldbytes + length, std::back_inserter(newbytes));
		delete[] oldbytes;

		for (int i = 2; i < length; i++)
		{//we start at 3 because the shortest RAR signature is 3 bytes in length
			if (i > 5)
			{
				if (newbytes[i-6] == 0x52 && newbytes[i-5] == 0x61 && newbytes[i-4] == 0x72 && newbytes[i-3] == 0x21 && newbytes[i-2] == 0x1A && newbytes[i-1] == 0x07 && newbytes[i] == 0x00)
				{//a newer version of the RAR file

					if (password) rarfunction.StartWithPassword(newbytes, i - 6, length, myunpacksize, argv[2]);
					else rarfunction.StartWithoutPassword(newbytes, i - 6, length, myunpacksize);
				}
			}
			if (newbytes[i-2] == 0x45 && newbytes[i-1] == 0x7e && newbytes[i] == 0x5e)
			{//this represents an older type of RAR file
				if (password) rarfunction.StartWithPassword(newbytes, i - 2, length, myunpacksize, argv[2]);
				else rarfunction.StartWithoutPassword(newbytes, i - 2, length, myunpacksize);
			}
		}

	}else
	{
		cout << "File could not be opened" << endl;
		myfile.clear(); //clear any errors
		myfile.close();
	}


	return 0;
}

/**
 A function to start the unrar process that needs to be decrypted with a password

 @param filebytes - The bytes that need to be read in order to obtain the data
 @param location - The location of the start of the RAR file signature within the <code>filebytes</code> memory
 @param length - The length of the <code>filebytes</code> argument in terms of bytes
 @param myunpacksize - The size of the buffer where the information obtained in the RAR file can be written. This is in terms of bytes
 @param pword - The password to decrypt the file.
*/
void ScanRar::StartWithPassword(std::vector<char>& filebytes, int location, int length, const unsigned int myunpacksize, char* pword)
{
	const int numparameters = 6;
	char * achar[numparameters];
	string password = "-p";
	password.append(pword);
	char* thepword = (char*)password.c_str();
	achar[1] = "p";
	achar[2] = "-y"; //say yes to everything
	achar[3] = "-ai"; //Ignore file attributes
	achar[4] = thepword; //Supply a password
	//achar[5] = pword;
	achar[5] = "-kb"; //Keep broken extracted files
	achar[numparameters] = "aRarFile.rar"; //dummy file name

		StartDecompression(filebytes, location, length, myunpacksize, achar, numparameters);
}

/**
 A function to start the unrar process that needs to be decrypted without a password

 @param filebytes - The bytes that need to be read in order to obtain the data
 @param location - The location of the start of the RAR file signature within the <code>filebytes</code> memory
 @param length - The length of the <code>filebytes</code> argument in terms of bytes
 @param myunpacksize - The size of the buffer where the information obtained in the RAR file can be written. This is in terms of bytes
*/
void ScanRar::StartWithoutPassword(std::vector<char>& filebytes, int location, int length, const unsigned int myunpacksize)
{
	const int numparameters = 6;
	char * achar[numparameters];
	achar[1] = "p";
	achar[2] = "-y"; //say yes to everything
	achar[3] = "-ai"; //Ignore file attributes
	achar[4] = "-p-"; //Don't ask for password
	//achar[5] = "-inul"; //Disable all messages
	achar[5] = "-kb"; //Keep broken extracted files
	achar[numparameters] = "aRarFile.rar"; //dummy file name

	StartDecompression(filebytes, location, length, myunpacksize, achar, numparameters);
}

/**
The function performs the decompression, decryption, and unpacking of a RAR file.
The data will be available once the <code>extract.DoExtract(...)</code> has been called.
The actual data will be located in the <code>data</code> variable.

 @param filebytes - the RAR file in memory
 @param location - the location, in filebytes, where the start of the RAR file is located
 @param length - length of the file that we can search
 @param myunpacksize - length of the size that we are allowed
 @param parameters - Contains all the parameters for the parser to work correctly
 @param numparameters - number of parameters that have been supplied
*/

void ScanRar::StartDecompression(std::vector<char>& filebytes, int location, int length, const unsigned int myunpacksize, char ** parameters, int numparameters)
{

	string xmloutput = "<rar>\n";
	CommandData data; //this variable is for assigning the commands to execute
	data.ParseCommandLine(numparameters, parameters); //input the commands and have them parsed
	const wchar_t* c = L"aRarFile.rar"; //the 'L' prefix tells it to convert an ASCII Literal
	data.AddArcName("aRarFile.rar",c); //sets the name of the file

	/*IMPORTANT*/
	byte *memoryspace = new byte[myunpacksize]; //This is where we are offloading the data from the RAR file.
	/*IMPORTANT*/

	CmdExtract extract; //from the extract.cpp file; allows the extraction to occur

	byte *startingaddress = (byte*) &filebytes[location];

	ComprDataIO mydataio;
	mydataio.SetSkipUnpCRC(true); //skip checking the CRC to allow more processing to occur
	mydataio.SetUnpackToMemory(memoryspace,myunpacksize); //Sets flag to save output to memory

	extract.SetComprDataIO(mydataio); //Sets the ComprDataIO variable to the custom one that was just built

	Archive myarch;
	myarch.InitArc(startingaddress, length - location);
	extract.ExtractArchiveInit(&data, myarch);

	//Extraction occurs here
	extract.DoExtract(&data, startingaddress, length - location, xmloutput);
	
	xmloutput.append("\n<\\rar>\n");
	cout << xmloutput.c_str() << endl;
	/*IMPORTANT*/
	//Now, one can access the 'memoryspace' variable and access all the data that has been extracted from the RAR file

	data.Close();

	/*This function is for testing purposes only*/
	//PrintResults(myunpacksize, memoryspace);
	/*This function is for testing purposes only*/

	delete[] memoryspace; //clear memory in case there is more than one rar file in a buffer.
	//delete startingaddress;

	return;
}

/**
This function just prints the information from the <code>data</code> variable to standard out.
 @param amount - The amount of memory that should be printed, in bytes
 @param unpackedfiles - the raw data extracted from the RAR file
*/
void ScanRar::PrintResults(unsigned int amount, byte *unpackedfiles)
{
	cout << "\nThe following is what has been found in memory:\n" ;//<< memoryspace << endl;

	int64 k = 0;
	while(k < amount)
	{
		printf("%c", (char)unpackedfiles[k]);
		k++;
	}
}
