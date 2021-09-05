/**
 * A simple regex finder.
 */

#include "config.h"

#include "be13_api/scanner_params.h"
#include "be13_api/scanner_set.h"
#include "be13_api/regex_vector.h"
#include "be13_api/utils.h" // needs config.h
#include "findopts.h"

// anonymous namespace hides symbols from other cpp files (like "static" applied to functions)
// TODO: make this not a global variable
namespace {
    regex_vector find_list;
    void add_find_pattern(const std::string &pat) {
        find_list.push_back("(" + pat + ")"); // make a group
    }

    void process_find_file(const char *findfile) {
        std::ifstream in;

        in.open(findfile,std::ifstream::in);
        if(!in.good()) {
            std::cerr << "Cannot open " << findfile << "\n";
            throw std::runtime_error(findfile);
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
void scan_find(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT) {
        sp.info->set_name("find");
        sp.info->name		= "find";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Simple search for patterns";
        sp.info->scanner_version= "1.1";
        sp.info->scanner_flags.find_scanner = true; // this is a find scanner
        sp.info->feature_defs.push_back( feature_recorder_def("find"));
        auto lowercase = histogram_def::flags_t(); lowercase.lowercase = true;
      	sp.info->histogram_defs.push_back( histogram_def("find", "find", "", "","histogram", lowercase));
        return;
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;

    if (scanner_params::PHASE_INIT == sp.phase) {
        for (const auto &it : FindOpts::get().Patterns) {
            add_find_pattern(it);
        }
        for (const auto &it : FindOpts::get().Files) {
            process_find_file(it.c_str());
        }
    }

    if(sp.phase==scanner_params::PHASE_SCAN) {
        /* The current regex library treats \0 as the end of a string.
         * So we make a copy of the current buffer to search that's one bigger, and the copy has a \0 at the end.
         * This is super-wasteful. Does Lightgrep have this problem?
         */
        feature_recorder &f = sp.named_feature_recorder("find");

        auto *tbuf = sbuf_t::sbuf_malloc(sp.sbuf->pos0, sp.sbuf->bufsize+1, sp.sbuf->bufsize+1);
        memcpy(tbuf->malloc_buf(), sp.sbuf->get_buf(), sp.sbuf->bufsize);
        const char *base = static_cast<const char *>(tbuf->malloc_buf());
        tbuf->wbuf(sp.sbuf->bufsize, 0); // null terminate

        /* Now see if we can find a string */
        for (size_t pos = 0; pos < sp.sbuf->pagesize && pos < sp.sbuf->bufsize;) {
            std::string found;
            size_t offset=0;
            size_t len = 0;
            if ( find_list.search_all( base+pos, &found, &offset, &len)) {
                if(len == 0) {
                    len+=1;
                    continue;
                }
                f.write_buf( *sp.sbuf, pos+offset, len);
                pos += offset+len;
            } else {
                /* nothing was found; skip past the first \0 and repeat. */
                const char *eos = static_cast<const char *>(memchr( base+pos, '\000', sp.sbuf->bufsize-pos));
                if (eos) pos=(eos-base)+1;		// skip 1 past the \0
                else     pos=sp.sbuf->bufsize;	// skip to the end of the buffer
            }
        }
        delete tbuf;
    }
}
