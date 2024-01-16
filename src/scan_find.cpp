/**
 * A simple regex finder.
 */

#include "config.h"

#include "be20_api/scanner_params.h"
#include "be20_api/scanner_set.h"
#include "be20_api/regex_vector.h"
#include "be20_api/utils.h" // needs config.h
#include "be20_api/dfxml_cpp/src/dfxml_writer.h"

// We need the defaults for page scan and margin. We really should get the current ones...
#import "phase1.h"

// anonymous namespace hides symbols from other cpp files (like "static" applied to functions)
// TODO: make this not a global variable
namespace {
    regex_vector find_list;
    void add_find_pattern(const std::string &pat) {
        find_list.push_back("(" + pat + ")"); // make a group
    }

    void process_find_file(scanner_params &sp, std::filesystem::path findfile) {
        std::ifstream in;

        in.open(findfile, std::ifstream::in);
        if(!in.good()) {
            std::cerr << "Cannot open " << findfile << "\n";
            throw std::runtime_error(Formatter() << findfile);
        }
        if (sp.ss->writer) {
            sp.ss->writer->push("process_find_file", Formatter() << findfile);
        }
        while(!in.eof()){
            std::string line;
            getline(in,line);
            truncate_at(line,'\r');         // remove a '\r' if present
            if(line.size()>0) {
                if(line[0]=='#') continue;  // ignore lines that begin with a comment character
                add_find_pattern(line);
                if (sp.ss->writer) { sp.ss->writer->xmlout("find_pattern", line); }
            }
        }
        if (sp.ss->writer) sp.ss->writer->pop("process_find_file");
    }
}

void scan_find_sbuf(scanner_params &sp, sbuf_t &sbuf)
{
    feature_recorder &f = sp.named_feature_recorder("find");

    auto *tbuf = sbuf_t::sbuf_malloc(sp.sbuf->pos0, sp.sbuf->bufsize+1, sp.sbuf->bufsize+1);
    memcpy(tbuf->malloc_buf(), sp.sbuf->get_buf(), sp.sbuf->bufsize);
    const char *tbase = static_cast<const char *>(tbuf->malloc_buf());
    tbuf->wbuf(sp.sbuf->bufsize, 0); // null terminate

    /* Now see if we can find a string */
    for (size_t pos = 0; pos < sp.sbuf->pagesize && pos < sp.sbuf->bufsize;) {
        std::string found;
        size_t offset=0;
        size_t len = 0;

        if ( find_list.search_all( tbase+pos, &found, &offset, &len)) {
            if (len == 0) {
                pos += 1;
                continue;
            }
            f.write_buf( *sp.sbuf, pos+offset, len);
            pos += offset+len;
        } else {
            /* nothing was found; skip past the first \0 and repeat. */
            const char *eos = static_cast<const char *>(memchr( tbase+pos, '\000', sp.sbuf->bufsize-pos));
            if (eos){
                pos = (eos - tbase) + 1;		// skip 1 past the \0
            } else {
                break;
            }
        }
    }
    delete tbuf;
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
    if (sp.phase == scanner_params::PHASE_INIT2 ) {
        for (const auto &it : sp.ss->find_patterns()) {
            add_find_pattern(it);
            if (sp.ss->writer) { sp.ss->writer->xmlout("find_pattern", it); }
        }
        for (const auto &it : sp.ss->find_files()) {
            process_find_file(sp, it);
        }
    }

    if(sp.phase==scanner_params::PHASE_SCAN) {

        if (find_list.size()==0){
            /* Nothing to find */
            return;
        }

        /* The current regex library treats \0 as the end of a string.
         * So we make a copy of the current buffer to search that's one bigger, and the copy has a \0 at the end.
         * This is super-wasteful. Does Lightgrep have this problem?
         *
         * We also want to not scan more than a full 'page' if we were scanning an image. Because a memory-mapped
         * file will have an sbuf the size of the whole file, we split it up and scan scan_find_sbuf()
         */

        Phase1::Config local_cfg;

        for(size_t pos = 0; pos < sp.sbuf->pagesize && pos < sp.sbuf->bufsize; pos+=local_cfg.opt_pagesize){
            sbuf_t sbuf(*sp.sbuf, pos, pos+local_cfg.opt_pagesize + local_cfg.opt_marginsize);
            scan_find_sbuf(sp, sbuf);
        }
    }
}
