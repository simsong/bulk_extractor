/* 
 * Misc support functions
 */

#include "bulk_extractor.h"
#include <stdarg.h>
#include <ctype.h>

/****************************************************************
 *** WIN32 SPECIAL AND DEBUG
 ****************************************************************/

#if 0
std::string ssprintf(const char *fmt,...)
{
    char buf[65536];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(buf,sizeof(buf),fmt,ap);
    va_end(ap);
    return string(buf);
}
#endif

/* Truncate a string after a character */
void truncate_at(string &line,char ch)
{
    size_t pos = line.find(ch);
    if(pos!=string::npos) line.erase(pos);
}


bool ends_with(const std::string &buf,const std::string &with)
{
    size_t buflen = buf.size();
    size_t withlen = with.size();
    return buflen>withlen && buf.substr(buflen-withlen,withlen)==with;
}

bool ends_with(const std::wstring &buf,const std::wstring &with)
{
    size_t buflen = buf.size();
    size_t withlen = with.size();
    return buflen>withlen && buf.substr(buflen-withlen,withlen)==with;
}


/****************************************************************/
/* C++ string splitting code from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}


