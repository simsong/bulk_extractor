README

RAR feature extractor software for bulk_extractor
Benjamin Salazar
JHU/APL
benjamin.salazar@jhuapl.edu

-----
PURPOSE:

The provided code is meant to be used in the bulk_extractor software system. 
This code will allow the software system to detect, decompress, and unpack RAR
files with the following file signatures:
0x 52 61 72 21 1A 07 00
0x 52 45 7E 5E

-----
ABOUT RAR FILES:

Originally created in 1993 by Eugene and Alexander Roshal, The file format has
gone through multiple revisions and is currently at the stable version of 4.11.
The algorithm and source code for the compression and packing technology is not
open source, but the unpacking and decompression algorithms are open source. 
More information can be located at: http://www.forensicswiki.org/wiki/RAR 

-----
THEORY OF OPERATION:

The software takes advantage of multiple file classes in order to execute the 
opening of a RAR file. The launching point of the feature extractor is the 
"scan_rar.cpp" file. This file performs the searching of raw bytes to find the
RAR file signature (see the "PURPOSE" section above). Once this has been found,
the location is recorded and the decompression and unpacking methods are 
configured. 

If one would like to decompress/unpack a file with a password, call the 
"StartWithPassword" function and provide a 'char*' password in addition to the 
other variables. If one does not need to provide a password, utilize the 
"StartWithoutPassword" function. Both of these functions setup parameter values
in order to run the "StartDecompression" function within the scan_rar.cpp file.

The "StartDecompression" function performs all of the heavy lifting and will
extract the information from the RAR file. There are a couple of items to be
aware about:
*Since the normal unrar software requires a file within the file system to be
supplied and does not support reading from memory, some of the software had to
be changed. One will notice the name "aRarFile.rar" to appear numerous times; 
this allows the software to think it is reading from a file within the file 
system while it is really not doing so. 
*The 'memoryspace' variable is crucial to extracting the information from the 
RAR file. This variable is used to store ALL of the output obtained from the 
RAR file. This variable should be read after the "DoExtract" function has 
completed to obtain the contents.
*Other variables are initialized with special flags and settings. These 
variables are 'mydataio', 'extract', 'myarch', and 'data'. Please see inline 
comments for more information.

In the "DoExtract" function, part of the extract.cpp file, the data is 
extracted by calling the "ExtractArchive" function with the appropriate 
variables. Within the "ExtractArchive" function, custom code has been added in
order to initialize location of the RAR file within the memory provided by 
bulk_extractor; this can be noticed in the first three lines of the function. 
From there, the function will go into the in-depth process of opening the RAR 
file and extracting the information.

Within the "ExtractCurrentFile" function, part of the extract.cpp file, the
data of one packed and compressed file is extracted. At the end of the 
extraction, heavy modification of reporting tools has been done to allow for 
the XML output of bulk_extractor. The XML output includes the packed and 
compressed file name, the size of the name, the unpacked size of the file, the 
data size of the file, the pack size of the file, the high pack size of the
file, the high unpack size of the file, the operating system used to create the
RAR file, the method of compression and packing, the CRC of the file, the file
time, the version of RAR that was used for packing and compression, as well as
the mtime, ctime, atime, and arctime variables.

Another class that has been heavily modified is the "File.cpp". This class has 
been converted from reading files from the operating system to reading files 
from memory. It keeps track of the location in memory and makes sure that it 
does not try and overrun the bounds set in place. One must initialize the File
class, then IMMEDIATELY call the 'InitFile' function in order to get it to 
work. If this is not called, the class does not know where in memory the file 
is located. The other functions have the original lines of code commented out,
and the new code dropped in. The functions included perform some of the 
following functions, plus more: open a file, close a file, initialize the 
pointer, read from file, seek to another point in the file, and obtain the file
length.

Next, the "rawread.cpp" file was edited to work correctly with the File class. 
This class is rather important as the reading from the file class is performed 
here. This class contains extra variables that are used to record the file's 
length and the position in the file. It is able to obtain data of different 
sizes and types from the file class.

The "archive.cpp" file is important as well, and is directly implemented in the 
"scan_rar.cpp" file. This class has an additional function titled 'InitArc' 
which initializes the program to record the starting location of the archive.
This then initializes the File class that will be used. This class is also
important because this assists in how the archive is decompressed and unpacked.
In "scan_rar.cpp", the archive is set to continue opening up a corrupted
archive if it encountered; this will get out all as much of the data as 
possible.

The "arcread.cpp" file is important as well as it handles how to read the 
archive that has been instantiated. At line 145, inside the 'ReadHeader' 
function, the switch statement discovers the type of header present 
(please see http://www.forensicswiki.org/wiki/RAR for more information about 
headers in RAR files). From this one can verify that the correct headers are 
being extracted.

The last important file is the "arccmt.cpp" file. This provides a way to read 
archive comments as they are stored when creating an RAR file.

The other files within the this portion of bulk_extractor did not need to be 
edited in order to work properly. The author was able to use them and implement
them without any consequences. There are a total of 44 *.cpp files listed, each
implementing a different class.

-----
INSTRUCTIONS FOR UPGRADES:

Should another developer need to upgrade bulk_extractor in the future to 
handler newer versions of RAR files (at the time of publication, the RAR file 
format was at version 4.11), the original author recommends the following 
actions:

*Download the latest STABLE version of the RAR archive source code, titled 
"UnRAR source" here: http://www.rarlab.com/rar_add.htm

*Next, open the "rar.cpp" file and comment out the main function. You will NOT 
need this.

*Next, open up "rar.hpp" and add the "scan_rar.hpp" file to the list. Now open
the "scan_rar.cpp" file and attempt to run the software. The developer will be
greeted with errors where functions have not been implemented.

*Now, READ all of the notes in the "THEORY OF OPERATION" section of this 
document. It will provide good guidance on what to do in order to get the new 
version to work properly.

*The developer may be able to integrate a lot work that has been done in some 
classes. The "File.cpp" class needs a major overhaul, so take advantage of the 
code already written.

*Lastly, if anything interesting is discovered in the process, please record it
on the Forensics Wiki page: http://www.forensicswiki.org/wiki/RAR . The 
community would greatly appreciate it.

-----
POTENTIAL PROBLEMS:

Developing software can be buggy at times, and not everything is perfect. In 
the scenario of computer forensics, people may have partially deleted files, 
and they may not be able to be recovered. This RAR file extractor will go as 
much as possible, but may fail if too much key information is missing (mainly
headers). The software has been tested with all types of RAR files from version
2.0 all the way through version 4.11, including solid, locked, self-extracting 
(sfx), multimedia, and encrypted files. By default, it should be able to handle
those file types.

-----
BUGS:

At the moment, there are some bugs involving the opening of certain RAR files.
Any RAR file that is created with multiple options makes the software go into a
bad state. The data may be extracted, but cannot be guarenteed. This will be 
noted in the output of the software. Currently, the following samples do not
extract correctly:
	rar3.20-all.exe
	rar4.11_all_but_sfx.rar (contains a recovery record with the same magic 
		number of a RAR file, and tries to open the recovery portion of
		the file)
	rar4.11_all_extracted_rar_contents (contains the end of an EXE file 
		with RAR data inside of it. This file is not the same as the 
		'rar4.11_all_but_sfx.rar' due to extra EXE related code)
	rar2.50-all.exe
	rar3.11-all.exe
	rar4.0-all(lock).exe
	rar4.11x64-everything.exe
	rar4.11x64-everything(lock).exe	

-----
FUTURE WORK:

As with any software, there is always room for improvement. The first item that
should be worked on is the logging feature. Currently, if any errors come up 
during the process of opening a RAR file, it will be produced to standard 
output. This should be re-routed to the xmloutput variable located in 
scan_rar.cpp file.

Additionally, it may be worth it to get the .dll and .so libraries compiled and
hooked into the software. This would allow for an easier interface to open RAR 
files and may simplify a lot of the code that is already written.

-----
SUMMARY:

This document provides the theory of operation for the RAR feature extractor 
software for bulk_extractor. It explains what the software does and what files 
have been edited in order to obtain the results. This document should provide 
all necessary information to understand the software. Should there be any 
questions, please email the author at the top of this document. Have a great 
day.

\EOF
