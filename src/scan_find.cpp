/**
 * A simple regex finder.
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "histogram.h"

#include "bulk_extractor.h" // for regex_list type
#include "findopts.h"

using namespace std;

namespace { // anonymous namespace hides symbols from other cpp files (like "static" applied to functions)

    regex_list find_list;

    void add_find_pattern(const string &pat)
    {
        find_list.add_regex("(" + pat + ")"); // make a group
    }

    void process_find_file(const char *findfile)
    {
        std::ifstream in;
        
        in.open(findfile,std::ifstream::in);
        if(!in.good()) {
            err(1,"Cannot open %s",findfile);
        }
        while(!in.eof()){
            std::string line;
            getline(in,line);
            truncate_at(line,'\r');         // remove a '\r' if present
            if(line.size()>0) {
                if(line[0]=='#') continue;  // ignore lines that begin with a comment character
                add_find_pattern(line);
            }
        }
    }
}

extern "C"
void scan_find(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::PHASE_STARTUP) {
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name		= "find";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Simple search for patterns";
        sp.info->scanner_version= "1.1";
        sp.info->flags		= scanner_info::SCANNER_FIND_SCANNER;
        sp.info->feature_names.insert("find");
      	sp.info->histogram_defs.insert(histogram_def("find","","histogram",HistogramMaker::FLAG_LOWERCASE));
        return;
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;

    if (scanner_params::PHASE_INIT == sp.phase) {
        for (vector<string>::const_iterator itr(FindOpts::get().Patterns.begin()); itr != FindOpts::get().Patterns.end(); ++itr) {
            add_find_pattern(*itr);
        }
        for (vector<string>::const_iterator itr(FindOpts::get().Files.begin()); itr != FindOpts::get().Files.end(); ++itr) {
            process_find_file(itr->c_str());
        }
    }

    if(sp.phase==scanner_params::PHASE_SCAN) {
        /* The current regex library treats \0 as the end of a string.
         * So we make a copy of the current buffer to search that's one bigger, and the copy has a \0 at the end.
         */
        feature_recorder *f = sp.fs.get_name("find");

        managed_malloc<u_char> tmpbuf(sp.sbuf.bufsize+1);
        if(!tmpbuf.buf) return;				     // no memory for searching
        memcpy(tmpbuf.buf, sp.sbuf.buf, sp.sbuf.bufsize);
        tmpbuf.buf[sp.sbuf.bufsize]=0;
        for(size_t pos = 0; pos < sp.sbuf.pagesize && pos < sp.sbuf.bufsize;) {
            /* Now see if we can find a string */
            std::string found;
            size_t offset=0;
            size_t len = 0;
            if(find_list.check((const char *)tmpbuf.buf+pos,&found,&offset,&len)) {
                if(len == 0) {
                    len+=1;
                    continue;
                }
                f->write_buf(sp.sbuf,pos+offset,len);
                pos += offset+len;
            } else {
                /* nothing was found; skip past the first \0 and repeat. */
                const u_char *eos = (const u_char *)memchr(tmpbuf.buf+pos,'\000',sp.sbuf.bufsize-pos);
                if(eos) pos=(eos-tmpbuf.buf)+1;		// skip 1 past the \0
                else    pos=sp.sbuf.bufsize;	// skip to the end of the buffer
            }
        }
    }
}
