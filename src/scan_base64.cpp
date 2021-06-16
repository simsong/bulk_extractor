/* scan_base64 - rewritten for bulk_extractor 2.0
 * Does not create any feature files.
 */

#include <cassert>
#include <cstdint>
#include <cstring>

#include "config.h"
#include "be13_api/scanner_params.h"

//#include "be13_api/bulk_extractor_i.h"
#include "base64_forensic.h"
#include "scan_base64.h"

/* These create bitfields so we can quickly assess the character classes in a potential base64 block */
static const uint32_t B64_LOWERCASE=1;
static const uint32_t B64_UPPERCASE=2;
static const uint32_t B64_NUMBER=4;
static const uint32_t B64_SYMBOL=8;
static int   base64array[256];           // array of valid base64 characters,
static bool  base64array_initialized = false;
static size_t minlinewidth = 60;
static size_t maxlinewidth_needed_for_character_classes = 160;


inline bool isbase64(unsigned char ch)
{
    assert(base64array_initialized==true);
    return base64array[ch];
}

void base64array_initialize()
{
    /* Create the base64 array */
    std::memset(base64array,0,sizeof(base64array));
    base64array[ static_cast<int>('+') ] = B64_SYMBOL;
    base64array[ static_cast<int>('/') ] = B64_SYMBOL;
    base64array[ static_cast<int>('-') ] = B64_SYMBOL; // RFC 4648
    base64array[ static_cast<int>('_') ] = B64_SYMBOL; // RFC 4648
    for (int ch='a';ch<='z';ch++){ base64array[ch] = B64_LOWERCASE; }
    for (int ch='A';ch<='Z';ch++){ base64array[ch] = B64_UPPERCASE; }
    for (int ch='0';ch<='9';ch++){ base64array[ch] = B64_NUMBER; }
    base64array_initialized=true;
}

/* Return true if the line only has base64 characters, space characters, or equal signs at the end */
bool sbuf_line_is_base64(const sbuf_t &sbuf, const size_t &start, const size_t &len, bool &found_equal)
{
    assert(base64array_initialized==true);
    int  b64_classes = 0;
    bool only_A = true;
    if (start>sbuf.pagesize) return false;
    bool inequal = false;
    for (size_t i=start;i<start+len;i++){
        if (sbuf[i]==' ' || sbuf[i]=='\t' || sbuf[i]=='\r') continue;
        if (sbuf[i]=='='){
            inequal=true;
            continue;
        }
        if (inequal) return false;       // after we find an equal, only space is acceptable
        uint8_t ch = sbuf[i];
        if (base64array[ch]==0){
            //fprintf(stderr,"NON CHAR '%c'\n",ch);
            return false;// non base64 character
        }
        b64_classes |= base64array[ch];      // record the classes we have found
        if (ch!='A') only_A = false;
    }
    if (inequal) found_equal = true;

    /* Additional tweak during 1.5 alpha testing. base64 scanner was
     * taking too long on very long lines of HEX that are seen in some
     * files. HEX typically has all lowercase or all uppercase but not
     * both, so we now require that every base64 line have both
     * uppercase and lowercase. The one exception is a line of all
     * capital As, which is commonly seen in BASE64 (because all capital As are nulls)
     */

    if (len>maxlinewidth_needed_for_character_classes){
        if (only_A) return true;                            // all capital As are true
        if ((b64_classes & B64_UPPERCASE)==0) return false; // must have an uppercase character
        if ((b64_classes & B64_LOWERCASE)==0) return false; // must have an lowercase character
    }
    return true;
}

/* Found the end of the base64 string and make the recursive call */
sbuf_t *decode_base64(const sbuf_t &sbuf, size_t start, size_t src_len)
{
    const char *src    = reinterpret_cast<const char *>(sbuf.get_buf() + start);
    if (src_len + start > sbuf.bufsize){ // make sure it doesn't go beyond buffer
        src_len = sbuf.bufsize-start;
    }

    // Make room for the destination.
    size_t dst_len = src_len + 4; // it can only get smaller, but give some extra space
    unsigned char *dst = (unsigned char *)malloc(dst_len);

    // Perform the conversion
    int conv_len = b64_pton_forensic(src, src_len, dst, dst_len);
    if (conv_len>0){
        const pos0_t pos0_base64 = (sbuf.pos0 + start) + "BASE64";
        return sbuf_t::sbuf_new(pos0_base64, dst, conv_len, conv_len);
    }
    return nullptr;
}

void process_base64(const scanner_params &sp, size_t start, size_t src_len)
{
    auto sbuf2 = decode_base64(*sp.sbuf, start, src_len);
    if (sbuf2) {
        sp.recurse(sbuf2);
    }
}


extern "C"
void scan_base64(scanner_params &sp)
{
    const int debug=0;

    sp.check_version();
    if ( sp.phase == scanner_params::PHASE_INIT){
        auto info = new scanner_params::scanner_info(scan_base64,"base64");
        info->author         = "Simson L. Garfinkel";
        info->description    = "scans for Base64-encoded data";
        info->scanner_version= "1.1";
        info->pathPrefix     = "BASE64";

        base64array_initialize();
        sp.info = info;
	return;
    }
    if ( sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = (*sp.sbuf);

	/* base64 is a newline followed by at least two lines of constant length,
	 * followed by an incomplete line ending with an equal sign.
	 * Lines can be termianted by \n or \r\n. This code simply ignores \r,
	 * which means that you can't have some lines terminated by \n and other
	 * terminated by \n\r.
	 *
	 * Note that this doesn't scan base64-encoded blobs smaller than two lines.
	 * Perhaps we should do that.
	 */
        assert(base64array_initialized==true);

        bool   inblock    = false;      // are we in a base64 block?
        size_t blockstart = 0;          // where the base64 started
        size_t prevlen    = 0;          // length of previous line
        size_t linecount  = 0;          // number of lines in region
        size_t pos        = 0;          // where we are scanning
        size_t line_start = 0;          // start of the line that was found
        size_t line_len   = 0;          // length of the line
        bool   found_equal = false;
        while (sbuf.getline(pos, line_start, line_len)){
            if (debug) fprintf(stderr,"BASE64 pos=%zd line_start=%zd line_len=%zd\n",pos,line_start,line_len);
            if (sbuf_line_is_base64(sbuf,line_start,line_len,found_equal)){
                if (inblock==false){
                    /* First line of a block! */
                    if (line_len >= minlinewidth){
                        inblock   = true;
                        blockstart= line_start;
                        linecount = 1;
                        prevlen   = line_len;
                    }
                    continue;
                }
                if (line_len!=prevlen){   // whoops! Lines are different lengths
                    // equal signs no longer required at end of BASE64 blocks
                    if (linecount>1){
                        if (debug) fprintf(stderr,"BASE64 1. linecount=%zd (%zd!=%zd) \n",linecount,line_len,prevlen);
                        process_base64(sp, blockstart, pos-blockstart);
                    }
                    inblock=false;
                    continue;
                }
                linecount++;
                continue;
            } else {
                /* Next line is not Base64. If we had more than 2 lines, process them.
                 * Note: hopefully the first line was the beginning of a BASE64 block to address
                 * alignment issues.
                 */
                if (linecount>2 && inblock){
                    if (debug){
                        fprintf(stderr,"BASE64 2. blockstart=%zd line_start=%zd pos=%zd linecount=%zd\n",
                                blockstart,line_start,pos,linecount);
                    }
                    process_base64(sp, blockstart, pos-blockstart);
                }
                inblock = false;
            }
        }
    }
}
