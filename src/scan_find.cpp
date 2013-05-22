/**
 * A simple regex finder.
 */

#include "config.h"
#include "bulk_extractor_i.h"
#include "histogram.h"


extern "C"
void scan_find(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::PHASE_STARTUP){
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
    if(sp.phase==scanner_params::PHASE_SCAN){

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
