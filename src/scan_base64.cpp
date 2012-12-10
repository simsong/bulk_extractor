#include "bulk_extractor.h"
#include "base64_forensic.h"

static bool base64array[256];
u_int base64_min = 128;			// don't bother with smaller than this


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

int minlinewidth = 60;

extern "C"
void scan_base64(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::startup){
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
    if(sp.phase==scanner_params::shutdown) return;
    if(sp.phase==scanner_params::scan){
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
	for(size_t i=0;i<sbuf.pagesize;i++){
	    if(i==0 || sbuf[i]=='\n' ){
		/* Try to figure out the line width; we only decode base64
		 * if we see two lines of the same width.
		 */
		ssize_t w1 = find64(sbuf,'\n',i+1);
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

			/* Found the end of the base64 string; process. */

			unsigned char *base64_target = (unsigned char *)malloc(base64_len);
			const char *src = (const char *)(sbuf.buf+i);
			if(base64_len + i > sbuf.bufsize){ // make sure it doesn't go beyond buffer
			    base64_len = sbuf.bufsize-i;
			}
			int conv_len = b64_pton_forensic(src, base64_len, // src,srclen
							 base64_target, base64_len); // target, targetlen
			if(conv_len>0){
			    const pos0_t pos0_base64 = (sbuf.pos0 + i) + rcb.partName;
			    const sbuf_t sbuf_base64(pos0_base64, base64_target,conv_len,conv_len,false); // we will free
			    (*rcb.callback)(scanner_params(sp,sbuf_base64));
			    if(rcb.returnAfterFound){
				free(base64_target);
				return;
			    }
			}
			free(base64_target);
			i = j;			// advance past this section
			break;			// break out of the j loop
		    }
		}
	    }
	}
    }
}
