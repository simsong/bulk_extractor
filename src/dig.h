/*
 * dig.h:
 * class interface for dig - recursively scan files in a file system, never repeating.
 */

#ifndef DIG_H
#define DIG_H

#include <string>
#include <stack>
#include <dirent.h>
#include <iostream>
#include <set>
#include <sys/stat.h>
#include <stdint.h>

#if defined(WIN32) || defined(MINGW) || defined(__MINGW32__) || defined(__MINGW64__)
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#endif

class dig {
public:
#ifdef WIN32
    typedef std::wstring filename_t;
#else    
    typedef std::string filename_t;
#endif
    static bool ignore_file_name(const filename_t &name);
    static const filename_t DIRSEP;
    class const_iterator {
    public:
#ifdef WIN32
	static bool ignore_file_attributes(const WIN32_FIND_DATA &FindFileData);
#endif
	class devinode {
	public:
	    devinode(dev_t dev_,ino_t ino_):dev(dev_),ino(ino_){}
	    devinode(const devinode &di):dev(di.dev),ino(di.ino){}
	    dev_t dev;
	    ino_t ino;
	    bool operator<(const devinode t2) const{
		return this->dev < t2.dev || (this->dev==t2.dev && this->ino < t2.ino);
	    }
	};
	class dirstackelement {
	private:
	    const dirstackelement &operator=(const dig::const_iterator::dirstackelement &d){return *this;  }
	public:;
#ifdef WIN32
	    dirstackelement(filename_t name_,HANDLE hFind_):name(name_),hFind(hFind_){}
	    dirstackelement(const dirstackelement &d):name(d.name),hFind(d.hFind){}
	    filename_t name;
	    HANDLE hFind;
#else
	    dirstackelement(filename_t name_,DIR *dir_):name(name_),dir(dir_){}
	    dirstackelement(const dirstackelement &d):name(d.name),dir(d.dir){}
	    filename_t name;
	    DIR *dir;
#endif
	};

	std::set<devinode> seen;	// things not to repeat
	std::stack<dirstackelement>dirstack;	// stack of open directories
	filename_t current_file;	// file I'm supposed to get
	bool ready;			// has this file been processed?

    public:
	const_iterator():
	    seen(),dirstack(),current_file(),ready(false){ };
	const_iterator(filename_t current_file_):
	    seen(),dirstack(),current_file(current_file_),ready(false){ };
	bool operator == (const const_iterator &i2);
	bool operator != (const const_iterator &i2) { return !(*this == i2); };
	filename_t operator*();		// returns the file currently pointed to
	/**
	 * open fn and put it on the stack. Return true if it can be used immediately
	 */
	bool open(const filename_t &fn);
    };

    dig(const filename_t &fn);
#ifdef WIN32
    dig(const std::string &fn); // for 8-bit filenames
#endif
    filename_t start;
    dig::const_iterator begin();
    dig::const_iterator end();

};
dig::const_iterator & operator++(dig::const_iterator &it);
#endif
