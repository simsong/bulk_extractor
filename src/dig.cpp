/*
 * dig.cpp:
 * Originally from md5deep. This version of dig creates a nice C++ iterator
 * that returns path names and which runs under Unix or Windows.
 *
 * 
 * This is a work of the US Government. In accordance with 17 USC 105,
 *  copyright protection is not available for any work of the US Government.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "dig.h"
#include "utf8.h"
#include <iostream>

#ifndef _TEXT
#define _TEXT(x) x
#endif

const dig::filename_t dig::DIRSEP(_TEXT("/"));

dig::const_iterator dig::begin()
{
    dig::const_iterator it;
    if(it.open(start)==false){
	++it;				// advance past the unusable
    }
    return it;
}

dig::const_iterator dig::end()
{
    return dig::const_iterator();
}

/* open a name.
 * If what we open can be used immediately (that is, it's not a directory), return true.
 */
bool dig::const_iterator::open(const filename_t &fn)
{
    /* See if we can process as a directory. */
#ifdef WIN32
    /* We might want to stat here first */
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = FindFirstFile((fn+_TEXT("/*")).c_str(), &FindFileData);
    if(hFind != INVALID_HANDLE_VALUE){
	dirstack.push(dirstackelement(fn,hFind)); // we can
	current_file = fn + DIRSEP + FindFileData.cFileName;
	return true;		// we can use the first entry!
    }
#else
    DIR *d = opendir(fn.c_str());
    if(d){
	dirstack.push(dirstackelement(fn,d));
	return false;			// need to read the directory to get the first entry
    }
#endif
    /* Not a directory. See if we can process it as a file-like object */
    current_file = fn;
    return true;
}

/**
 * Dereferencing the iterator returns the current file.
 * This is pretty simple, given everything else.
 */
dig::filename_t dig::const_iterator::operator*() {
    return current_file;
}

bool dig::ignore_file_name(const filename_t &name)
{
    return name==_TEXT(".") || name==_TEXT("..");
}

#ifdef WIN32
bool dig::const_iterator::ignore_file_attributes(const WIN32_FIND_DATA &FindFileData)
{
    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
	if (IO_REPARSE_TAG_MOUNT_POINT == FindFileData.dwReserved0) {
	    return true;
	} 
	if (IO_REPARSE_TAG_SYMLINK == FindFileData.dwReserved0) {
	    /* Perhaps we should do these? */
	    return true;
	}
	/* Unknown reparse point */
	return true;
    }
    return false;
}
#endif

/* Incrementing goes to the next file at the top of the stack.
 * If we reach the end of the directory, the directory gets closed, it's popped off the stack, and we repeat.
 * If it is a directory, the directory gets opened, it gets pushed on the stack, and we repeat.
 *
 * If it is a directory, put the directory on the stack,
 * get the first entry from the directory that's valid,
 * and repeat.
 * 
 */
dig::const_iterator & operator++(dig::const_iterator &it)
{
    if(it.dirstack.size()==0){
	it.current_file = _TEXT("");
	return it;
    }

    while(it.dirstack.size()!=0){
	dig::filename_t filename;		// that is read
#ifdef WIN32
	/* MICROSOFT; UTF-16 */
	WIN32_FIND_DATA FindFileData;
	memset(&FindFileData,0,sizeof(FindFileData));
	if(FindNextFile(it.dirstack.top().hFind,&FindFileData)==0){
	    it.dirstack.pop();		// end of directory
	    it.current_file=_TEXT("");
	    continue;
	}
	filename = FindFileData.cFileName;
#else
	/* POSIX; UTF-8 */
	struct dirent *entry = readdir(it.dirstack.top().dir);
	if(entry==NULL){		// end of the directory
	    closedir(it.dirstack.top().dir);
	    it.dirstack.pop();
	    it.current_file="";		// see if there is more?
	    continue;
	}
	filename = entry->d_name;
#endif
	if(dig::ignore_file_name(filename)){
	    continue;		// ignore this
	}

	/* Get the full path name */
	dig::filename_t pathname = it.dirstack.top().name;
	pathname.append(_TEXT("/"));
	pathname.append(filename);

	/* Get the device and inode number */
	struct stat st;
#if defined(_WIN32) || defined(WIN32)
	memset(&st,0,sizeof(st));
	HANDLE filehandle = CreateFile(pathname.c_str(),
				       0,   // desired access
				       FILE_SHARE_READ,
				       NULL,  
				       OPEN_EXISTING,
				       (FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS),
				       NULL);
	BY_HANDLE_FILE_INFORMATION fileinfo;
	(void)GetFileInformationByHandle(filehandle, &fileinfo);
	CloseHandle(filehandle);
	st.st_dev = 0;
	st.st_ino = (((uint64_t)fileinfo.nFileIndexHigh)<<32) | (fileinfo.nFileIndexLow);
#else
	if(stat(pathname.c_str(),&st)){
	    continue;			// can't stat it
	}
        if(S_ISFIFO(st.st_mode)) {      // don't process FIFOs
            continue;                   // 
        }
        if(S_ISSOCK(st.st_mode)) {      // don't process sockets
            continue;                   // 
        }
        if(S_ISBLK(st.st_mode)) {      // don't process block devices
            continue;                   // 
        }
        if(S_ISCHR(st.st_mode)) {      // don't process character devices
            continue;                   // 
        }

#endif
	dig::const_iterator::devinode di(st.st_dev,st.st_ino);
	
	/* Should this be ignored? */
	if(it.seen.find(di)!=it.seen.end()){
	    continue;			// seen it before; don't process it
	}
	/* Well, we've seen it now */
	it.seen.insert(di);

	/* See if it can be opened list a directory */
	if(it.open(pathname.c_str())){
	    return it;				// it's now the current_file
	}
    }
    return it;				// reached the end
}

bool dig::const_iterator::operator == (const const_iterator &i2) {
    return this->current_file==i2.current_file 
	&& this->dirstack.size()==0
	&& i2.dirstack.size()==0;
};


/**
 * the constructor isn't all that exciting.
 */

dig::dig(const dig::filename_t &start_):start(start_)
{
}

#ifdef WIN32
static std::wstring utf8to16(const std::string &fn8)
{
    std::wstring fn16;
    utf8::utf8to16(fn8.begin(),fn8.end(),back_inserter(fn16));
    return fn16;
}
#if 0
static std::string utf16to8(const std::wstring &fn16)
{
    std::string fn8;
    utf8::utf16to8(fn16.begin(),fn16.end(),back_inserter(fn8));
    return fn8;
}
#endif

dig::dig(const std::string &start_):start(utf8to16(start_))
{
}
#endif

#ifdef STANDALONE

int main(int argc,char **argv)
{
    if(argc!=2){
	std::cerr << "Usage: " << argv[0] << " path\n";
	exit(1);
    }

    dig d(argv[1]);

    for(dig::const_iterator it = d.begin();it!=d.end();++it){
#ifdef WIN32
	std::cout << utf16to8(*it) << "\n";
#else
	std::cout << (*it) << "\n";
#endif
    }
    exit(0);
}

#endif

