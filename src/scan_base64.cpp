#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "base64_forensic.h"

static bool  base64array[256];           // array of valid base64 characters
static u_int base64_min   = 128;			// don't bother with smaller than this
static int   minlinewidth = 60;


inline bool isbase64(unsigned char ch)
{
    return base64array[ch];
}

/* find64 - returns true if a region contains base64 code, but only if it also has something over F */
inline ssize_t find64(const sbuf_t &sbuf,unsigned char ch,size_t start)
{
    for(;start<sbuf.bufsize;start++){
	if(sbuf[start]==ch) return start;
	if(isbase64(sbuf[start])==false) return -1;
    }
    return -1;
}


/* get the next line line from the sbuf.
 * @param sbuf - the sbuf to process
 * @param pos  - on entry, current position. On exit, new position.
 *               pos[0] is the start of a line
 * @param start - the start of the line, a pointer into the sbuf
 * @param len   - the length of the line
 * @return true - a line was found; false - a line was not found
 */
inline bool sbuf_getline(const sbuf_t &sbuf,size_t &pos,size_t &start,size_t &len)
{
    /* Scan forward until pos is at the beginning of a line */
    if(pos >= sbuf.pagesize) return false;
    if(pos > 0){
        while((pos < sbuf.pagesize) && sbuf[pos-1]!='\n'){
            ++(pos);
        }
        if(pos >= sbuf.pagesize) return false; // didn't find another start of a line
    }
    start = pos;
    /* Now scan to end of the line, or the end of the buffer */
    while(++pos < sbuf.pagesize){
        if(sbuf[pos]=='\n'){
            break;
        }
    }
    len = (pos-start);
    return true;
}

/* Return true if the line only has base64 characters, space characters, or equal signs at the end */
inline bool sbuf_line_is_base64(const sbuf_t &sbuf,const size_t &start,const size_t &len,bool &found_equal)
{
    if(start>sbuf.pagesize) return false;
    bool inequal = false;
    for(size_t i=start;i<start+len;i++){
        if(sbuf[i]==' ' || sbuf[i]=='\t' || sbuf[i]=='\r') continue;
        if(sbuf[i]=='='){
            inequal=true;
            continue;
        }
        if(inequal) return false;       // after we find an equal, only space is acceptable
        if(!isbase64(sbuf[i])) return false; // non base64 character
    }
    if(inequal) found_equal = true;
    return true;
}

/* Found the end of the base64 string; process. */
inline void process(const class scanner_params &sp,const recursion_control_block &rcb,size_t start,size_t len)
{
    //fprintf(stderr,"process start=%zd  len=%zd\n",start,len);
    //fprintf(stderr,"To convert:\n");
    const sbuf_t &sbuf = sp.sbuf;
    managed_malloc<unsigned char>base64_target(len);
    const char *src = (const char *)(sbuf.buf+start);
    if(len + start > sbuf.bufsize){ // make sure it doesn't go beyond buffer
        len = sbuf.bufsize-start;
    }
    //fwrite(src,len,1,stderr);
    //fprintf(stderr,"\n======\n");
    int conv_len = b64_pton_forensic(src, len, // src,srclen
                                     base64_target.buf, len); // target, targetlen
    if(conv_len>0){
        //fprintf(stderr,"conv_len=%zd\n",conv_len);
        //fwrite(base64_target.buf,1,conv_len,stderr);

        const pos0_t pos0_base64 = (sbuf.pos0 + start) + rcb.partName;
        const sbuf_t sbuf_base64(pos0_base64, base64_target.buf,conv_len,conv_len,false); // we will free
        (*rcb.callback)(scanner_params(sp,sbuf_base64));
    }
}


extern "C"
void scan_base64(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "base64";
        sp.info->author         = "Simson L. Garfinkel";
        sp.info->description    = "scans for Base64-encoded data";
        sp.info->scanner_version= "1.0";

	/* Create the base64 array */
	memset(base64array,0,sizeof(base64array));
	base64array[(int)'+'] = true;
	base64array[(int)'/'] = true;
	for(int ch='a';ch<='z';ch++){ base64array[ch] = true; }
	for(int ch='A';ch<='Z';ch++){ base64array[ch] = true; }
	for(int ch='0';ch<='9';ch++){ base64array[ch] = true; }
	return;	/* No feature files created */
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = sp.sbuf;

	/* base64 is a newline followed by at least two lines of constant length,
	 * followed by an incomplete line ending with an equal sign.
	 * Lines can be termianted by \n or \r\n. This code simply ignores \r,
	 * which means that you can't have some lines terminated by \n and other
	 * terminated by \n\r.
	 *
	 * Note that this doesn't scan base64-encoded blobs smaller than two lines.
	 * Perhaps we should do that.
	 */
        size_t blockstart = 0;
        bool   inblock    = false;
        size_t prevlen    = 0;
        size_t linecount  = 0;
        size_t pos = 0;
        size_t start = 0;
        size_t len   = 0;
        bool   found_equal = false;
        while(sbuf_getline(sbuf,pos,start,len)){
            //fprintf(stderr,"pos=%zd\n",pos);
            if(sbuf_line_is_base64(sbuf,start,len,found_equal)){
                if(inblock==false){
                    /* First line of a block! */
                    inblock = true;
                    linecount = 1;
                    blockstart = start;
                    prevlen = len;
                    continue;
                }
                if(len!=prevlen){   // whoops! Lines are different lengths
                    if(found_equal && linecount>1){
                        //fprintf(stderr,"1. linecount=%zd\n",linecount);
                        process(sp,rcb,blockstart,pos-blockstart);
                    }
                    inblock=false;
                    continue;
                }
                linecount++;
                continue;
            }
            /* Ran out of the base64 block */
            if(linecount>2){
                //fprintf(stderr,"2. linecount=%zd\n",linecount);
                process(sp,rcb,blockstart,pos-blockstart);
            }
            inblock = false;
        }
        //fprintf(stderr,"done\n");
    }
}
#if 0
        if(inblock==false){     // first line in a block
                        inblock = true;
                        linecount = 1;
                        blockstart = start;
                        prevlen = len;
                    } else {
                        linecount++;
                        if(found_equal==false){
                            if(len!=prevlen){ // not the end and different line lengths; abort
                                inblock = false;
                            }
                                
                    }
                    if(found_equal){
                        process();
                        inblock = false;
                    }
                }
                else {
                    /* No longer in a base64 block. If we had more than two lines the same length, process */
                }
                    continue;
                }
                    
            } 
            else {
            }


	    if(i==0 || sbuf[i]=='\n' ){
		/* Try to figure out the line width; we only decode base64
		 * if we see two lines of the same width.
		 */
		ssize_t w1 = find64(sbuf,'\n',i+1);
                fprintf(stderr,"i=%d  w1=%d\n",i,w1);
		if(w1<0){
		    return;		// no second delim
		}

		ssize_t linewidth1 = w1-i;	// including \n
		if(i==0) linewidth1 += 1;	// if we were not on a newline, add one
	    
		if(linewidth1 < minlinewidth){
		    i=w1;		// skip past this block
		    continue;
		} 

		ssize_t w2 = find64(sbuf,'\n',w1+1);
		if(w2<0){
		    return;		// no third delim
		}
		ssize_t linewidth2 = w2-w1;
	
                fprintf(stderr,"   w1=%d  w2=%d  linewidth1=%d  linewidth2=%d\n",w1,w2,linewidth1,linewidth2);

		if(linewidth1 != linewidth2){
		    i=w2;	// lines are different sized; skip past both
		    continue;
		}
	
		/* Now scan from w2 until we find a terminator:
		 * - the '='.
		 * - characters not in base64
		 * - the end of the sbuf.
		 */
		for(size_t j=w2+1;j<sbuf.size();j++){
		    /* Each line should be the same size. if this is an even module of
		     * the start of the line and we don't have a line end, then the lines
		     * are not properly formed.
		     */
		    if(((j-w2) % linewidth1==0) && sbuf[j]!='\n'){
			i = j;		// advance to the end of this section
			break;		// break out of the j loop
		    }

		    /* If we found a character that indicates the end of a BASE64 block
		     * (a '=' or a '-' or a space), or we found an invalid base64
		     * charcter, or if we are on the last character of the sbuf,
		     * then attempt to decode.
		     */
		    char ch = sbuf[j];
		    bool eof = (j+1==sbuf.size());
		    if(eof || ch=='=' || ch=='-' || ch==' ' || (!isbase64(ch) && ch!='\n' && ch!='\r')){
			size_t base64_len = j-i;
			if(eof || ch=='-') base64_len += 1;	// we can include the termination character

			if(!eof && base64_len<base64_min){	// a short line?
			    i = j;			// skip this junk
			    continue; 
			}

		}
	    }
	}
    }
}
#endif
