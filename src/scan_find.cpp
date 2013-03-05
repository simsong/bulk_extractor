/**
 * A simple regex finder.
 */

#include "bulk_extractor.h"
#include "histogram.h"

regex_list find_list;

/****************************************************************
 *** find support. This will be moved to the plug-in some day.
 ***/

void add_find_pattern(const string &pat)
{
    find_list.add_regex("(" + pat + ")"); // make a group
}


void process_find_file(const char *findfile)
{
    ifstream in;

    in.open(findfile,ifstream::in);
    if(!in.good()){
	err(1,"Cannot open %s",findfile);
    }
    while(!in.eof()){
	string line;
	getline(in,line);
	truncate_at(line,'\r');         // remove a '\r' if present
	if(line.size()>0){
	    if(line[0]=='#') continue;	// ignore lines that begin with a comment character
	    add_find_pattern(line);
	}
    }
}

extern "C"
void scan_find(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "find";
        sp.info->author         = "Simson Garfinkel";
        sp.info->description    = "Searches for regular expressions";
        sp.info->scanner_version= "1.1";
	sp.info->flags		= scanner_info::SCANNER_NO_USAGE;
        sp.info->feature_names.insert("find");
	sp.info->histogram_defs.insert(histogram_def("find","","histogram",HistogramMaker::FLAG_LOWERCASE));
	return;
    }
    if(sp.phase==scanner_params::shutdown) return;
    if(sp.phase==scanner_params::scan){

	/* The current regex library treats \0 as the end of a string.
	 * So we make a copy of the current buffer to search that's one bigger, and the copy has a \0 at the end.
	 */
	feature_recorder *f = sp.fs.get_name("find");

	u_char *tmpbuf = (u_char *)malloc(sp.sbuf.bufsize+1);
	if(!tmpbuf) return;				     // no memory for searching
	memcpy(tmpbuf,sp.sbuf.buf,sp.sbuf.bufsize);
	tmpbuf[sp.sbuf.bufsize]=0;
	for(size_t pos=0;pos<sp.sbuf.pagesize && pos<sp.sbuf.bufsize;){
	    /* Now see if we can find a string */
	    string found;
	    size_t offset=0;
	    size_t len = 0;
            if(find_list.check((const char *)tmpbuf+pos,&found,&offset,&len)){
		if(len==0){
		    len+=1;
		    continue;
		}
		f->write_buf(sp.sbuf,pos+offset,len);
		pos += offset+len;
	    } else {
		/* nothing was found; skip past the first \0 and repeat. */
		const u_char *eos = (const u_char *)memchr(tmpbuf+pos,'\000',sp.sbuf.bufsize-pos);
		if(eos) pos=(eos-tmpbuf)+1;		// skip 1 past the \0
		else    pos=sp.sbuf.bufsize;	// skip to the end of the buffer
	    }
	}
	free(tmpbuf);
    }
}
