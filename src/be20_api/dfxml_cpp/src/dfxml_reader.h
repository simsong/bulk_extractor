#ifndef DFXML_READER_H
#define DFXML_READER_H

/**
 ** NOTE 1:
 ** THIS IS NOT A COMPLETE IMPLEMENTATION.
 ** This is a skeletal implementation of a DFXML reader to solve an immediate problem.
 ** For a full implementation, please see ../python/dfxml.py
 **
 ** If you want to add support for a specific DFXML tag, please add it
 ** and submit your patch as a pull request on github.
 **
 **
 ** NOTE 2:
 ** You *must* include an autoconf-generated config.h (or equivallent) file before including this file.
 **/

/*
 * Revision History:
 * 2012 - Simson L. Garfinkel - Developed as test program.
 * 2021 - Cleaned up. Added LGPL copyright notice.
 *
 * Copyright (C) 2021 Simson L. Garfinkel.
 *
 * LICENSE: LGPL Version 3. See COPYING.md for further information.
 */

#ifndef PACKAGE_NAME
#error This file requires that an autoconf-generated config.h (or equivallent) file be included first.
#endif

#include <cstdio>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <sstream>
#include <functional>
#include <cstdint>
#include <fstream>

#ifdef HAVE_EXPAT_H
#include <expat.h>
#else
#error dfxml_reader.h requires expat.h
#endif

/* We need netinet/in.h or windowsx.h */
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_WINSOCK2_H
#  include <winsock2.h>
#  include <windows.h>
#  include <windowsx.h>
#endif


#include "hash_t.h"

namespace dfxml {

    extern const char *dfxml_version();

    class saxobject {
    public:
        typedef std::map<std::string,std::string> hashmap_t;
        typedef std::map<std::string,std::string> tagmap_t;
        virtual ~saxobject(){}
        saxobject():hashdigest(),_tags(){}
        saxobject(const saxobject &that):hashdigest(that.hashdigest),_tags(that._tags){}
        hashmap_t hashdigest; // any object can have hashes
        tagmap_t _tags; // any object can tags
    };
    std::ostream & operator <<(std::ostream &os,const saxobject::hashmap_t &h) {
        for(dfxml::saxobject::hashmap_t::const_iterator it = h.begin(); it!=h.end(); it++){
            os << it->first << ":" << it->second << " ";
        }
        return os;
    };



    class no_hash:public std::exception {
        virtual const char *what() const throw() {
            return "requested hash not found";
        }
    };


    class byte_run:public saxobject {
    public:
        virtual ~byte_run(){};
        byte_run():saxobject(),img_offset(0),file_offset(0),len(0),sector_size(0){}
        byte_run(const byte_run &that):saxobject(that),
                                       img_offset(that.img_offset),
                                       file_offset(that.file_offset),
                                       len(that.len),
                                       sector_size(that.sector_size){}

        int64_t img_offset;
        int64_t file_offset;
        int64_t len;
        int64_t sector_size;
        md5_t md5() const {
            hashmap_t::const_iterator it = hashdigest.find("md5");
            if(it==hashdigest.end()) std::cout << "end found\n";
            if(it!=hashdigest.end()) std::cout << it-> first << "=" /* << it->second */ << "\n";
            if(it!=hashdigest.end()) return md5_t::fromhex(it->second);
            throw new no_hash();
        }
    };
    std::ostream & operator <<(std::ostream &os,const byte_run &b) {
        os << "byte_run[";
        if (b.img_offset) os << "img_offset=" << b.img_offset << ";";
        if (b.file_offset) os << "file_offset=" << b.file_offset << ";";
        if (b.len) os << "len=" << b.len << ";";
        if (b.sector_size) os << "sector_size=" << b.sector_size << ";";
        os << "]";
        return os;
    };

    class imageobject_sax:public dfxml::saxobject {
    public:
        virtual ~imageobject_sax(){};
    };

    class volumeobject_sax:public dfxml::saxobject {
    public:;
        volumeobject_sax():saxobject(),block_size(),image(){}
        uint64_t block_size;
        imageobject_sax image;
    };

    class file_object:public dfxml::saxobject {
    public:;
        file_object():saxobject(),volumeobject(0),byte_runs() { };
        file_object(const file_object &that):saxobject(that),volumeobject(that.volumeobject),
                                             byte_runs(that.byte_runs) {
        };
        const file_object &operator=(const file_object &fo){
            this->hashdigest = fo.hashdigest;
            this->_tags      = fo._tags;
            this->volumeobject = fo.volumeobject;
            this->byte_runs = fo.byte_runs;
            return *this;
        }

        typedef std::vector<dfxml::byte_run> byte_runs_t;
        volumeobject_sax *volumeobject;
        byte_runs_t byte_runs;

        std::string filename(){return _tags["filename"];}
        dfxml::md5_t md5() const {
            std::map<std::string,std::string>::const_iterator it = hashdigest.find("md5");
            if(it!=hashdigest.end()) return dfxml::md5_t::fromhex(it->second);
            throw new dfxml::no_hash();
        }
    };
};

typedef std::function<void (dfxml::file_object&)> fileobject_callback_t;

class dfxml_reader {
public:
    dfxml_reader():tagstack(),cdata(){}
    virtual ~dfxml_reader(){}
    static std::string getattrs(const char **attrs,const std::string &name) {
        for(int i=0;attrs[i];i+=2){
            if(name==attrs[i]) return std::string(attrs[i+1]);
        }
        return std::string("");
    }
    static uint64_t getattri(const char **attrs,const std::string &name) {
        std::stringstream ss;
        for(int i=0;attrs[i];i+=2){
            if(name==attrs[i]){
                ss << attrs[i+1];
                uint64_t val;
                ss >> val;
                return val;
            }
        }
        return 0;
    }
    std::stack<std::string> tagstack;
    std::stringstream cdata;
};

namespace dfxml {

    class file_object_reader:public dfxml_reader{
    private:
        /*** neither copying nor assignment is implemented ***/
        file_object_reader(const file_object_reader &) = delete;
        file_object_reader &operator=(const file_object_reader&) = delete;
    public:;

        static void startElement(void *userData, const char *name_, const char **attrs) {
            class file_object_reader &self = *(file_object_reader *)userData;
            std::string name(name_);

            self.cdata.str("");
            self.tagstack.push(name);
            if(name=="volume"){
                self.volumeobject = new dfxml::volumeobject_sax();
                self.volumeobject->block_size = 512; // default
            }
            if(name=="block_size"){
                /* pass */
            }
            if(name=="fileobject"){
                self.fileobject = new dfxml::file_object();
                self.fileobject->volumeobject = self.volumeobject;
                return;
            }
            if(name=="hashdigest"){
                self.hashdigest_type = getattrs(attrs,"type");
                return;
            }
            if(self.fileobject && (name=="run" || name=="byte_run")){
                dfxml::byte_run run;
                for(int i=0;attrs[i];i+=2){
                    if(run.img_offset==0 && !strcmp(attrs[i],"img_offset")){run.img_offset = std::atoi(attrs[i+1]);continue;}
                    if(run.file_offset==0 && !strcmp(attrs[i],"file_offset")){run.file_offset = std::atoi(attrs[i+1]);continue;}
                    if(run.len==0 && !strcmp(attrs[i],"len")){run.len = std::atoi(attrs[i+1]);continue;}
                    if(run.sector_size==0 && !strcmp(attrs[i],"sector_size")){run.sector_size = std::atoi(attrs[i+1]);continue;}
                }
                self.fileobject->byte_runs.push_back(run); // is there a more efficient way to do this?
            }
        }
        static void endElement(void *userData, const char *name_) {
            std::string name(name_);

            file_object_reader &self = *(file_object_reader *)userData;
            if(self.tagstack.top() != name){
                std::cout << "close tag '" << name << "' found; '" << self.tagstack.top() << "' expected.\n";
                exit(1);
            }
            self.tagstack.pop();
            std::string cdata = self.cdata.str();
            self.cdata.str("");

            if(name=="volume"){
                self.volumeobject = 0;
                return;
            }
            if(name=="block_size" && self.tagstack.size()>1){
                if(self.tagstack.top()=="volume"){
                    self.volumeobject->block_size = atoi(cdata.c_str());
                }
                return;
            }
            if(name=="fileobject"){
                self.callback(*self.fileobject);
                delete self.fileobject;
                self.fileobject = 0;
                return;
            }
            if(name=="hashdigest" and self.tagstack.size()>0){
                std::string alg = self.hashdigest_type;
                std::transform(alg.begin(), alg.end(), alg.begin(), ::tolower);
                if(self.tagstack.top()=="byte_run"){
                    self.fileobject->byte_runs.back().hashdigest[alg] = cdata;
                }
                if(self.tagstack.top()=="fileobject"){
                    self.fileobject->hashdigest[alg] = cdata;
                }
                return;
            }
            if(self.fileobject){
                self.fileobject->_tags[name] = cdata;
                return;
            }

        };
        static void read_dfxml(const std::string &fname,fileobject_callback_t process) {
            file_object_reader r;

            r.callback = process;

            XML_Parser parser = XML_ParserCreate(NULL);
            XML_SetUserData(parser, &r);
            XML_SetElementHandler(parser, startElement, endElement);
            XML_SetCharacterDataHandler(parser,characterDataHandler);

            std::fstream in(fname.c_str());

            if(!in.is_open()){
                std::cout << "Cannot open " << fname << ": " << strerror(errno) << "\n";
                exit(1);
            }
            try {
                std::string line;
                while(getline(in,line)){
                    if (!XML_Parse(parser, line.c_str(), line.size(), 0)) {
                        std::cout << "XML Error: " << XML_ErrorString(XML_GetErrorCode(parser))
                                  << " at line " << XML_GetCurrentLineNumber(parser) << "\n";
                        XML_ParserFree(parser);
                        return;
                    }
                }
                XML_Parse(parser, "", 0, 1);
            }
            catch (const std::exception &e) {
                std::cout << "ERROR: " << e.what() << "\n";
            }
            XML_ParserFree(parser);

        }
        static void characterDataHandler(void *userData,const XML_Char *s,int len) {
            class file_object_reader &self = *(file_object_reader *)userData;
            self.cdata.write(s,len);
        };

        virtual ~file_object_reader(){};
        file_object_reader(): dfxml_reader(),volumeobject(),fileobject(),callback(),hashdigest_type(){}
        dfxml::volumeobject_sax *volumeobject;
        dfxml::file_object *fileobject;		// the object currently being read
        fileobject_callback_t callback;
        std::string hashdigest_type;
    };
};

#endif
