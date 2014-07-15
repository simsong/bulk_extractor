#include "config.h"
#include "be13_api/bulk_extractor_i.h"

/**
 * scan_ascii85.cpp
 * Simson L. Garfinkel
 *
 * Originally from ascii85.c by Remy Oukaour (see below)
 *
 */

/*
 * ascii85 - Ascii85 encode/decode data and print to standard output
 *
 * Copyright (C) 2011 Remy Oukaour
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

int getc_nospace(FILE *f) {
	int c;
	while (isspace(c = getc(f)));
	return c;
}

void putc_wrap(char c, int wrap, int *len) {
	if (wrap && *len >= wrap) {
		putchar('\n');
		*len = 0;
	}
	putchar(c);
	(*len)++;
}

void encode_tuple(unsigned long tuple, int count, int wrap, int *plen, int y_abbr) {
	int i, lim;
	char out[5];
	if (tuple == 0 && count == 4) {
		putc_wrap('z', wrap, plen);
	}
	else if (tuple == 0x20202020 && count == 4 && y_abbr) {
		putc_wrap('y', wrap, plen);
	}
	else {
		for (i = 0; i < 5; i++) {
			out[i] = tuple % 85 + '!';
			tuple /= 85;
		}
		lim = 4 - count;
		for (i = 4; i >= lim; i--) {
			putc_wrap(out[i], wrap, plen);
		}
	}
}

void decode_tuple(unsigned long tuple, int count) {
	int i;
	for (i = 1; i < count; i++) {
		putchar(tuple >> ((4 - i) * 8));
	}
}

void ascii85_decode(FILE *fp, int delims, int ignore_garbage) {
	int c, count = 0, end = 0;
	unsigned long tuple = 0, pows[] = {85*85*85*85, 85*85*85, 85*85, 85, 1};
	while (delims) {
		c = getc_nospace(fp);
		if (c == '<') {
			c = getc_nospace(fp);
			if (c == '~') break;
			ungetc(c, fp);
		}
		else if (c == EOF) {
			eprintf("ascii85: missing <~\n");
			exit(1);
		}
	}
	for (;;) {
		c = getc_nospace(fp);
		if (c == 'z' && count == 0) {
			decode_tuple(0, 5);
			continue;
		}
		if (c == 'y' && count == 0) {
			decode_tuple(0x20202020, 5);
			continue;
		}
		if (c == '~' && delims) {
			c = getc_nospace(fp);
			if (c != '>') {
				eprintf("ascii85: ~ without >\n");
				exit(1);
			}
			c = EOF;
			end = 1;
		}
		if (c == EOF) {
			if (delims && !end) {
				eprintf("ascii85: missing ~>\n");
				exit(1);
			}
			if (count > 0) {
				tuple += pows[count-1];
				decode_tuple(tuple, count);
			}
			break;
		}
		if (c < '!' || c > 'u') {
			if (ignore_garbage) continue;
			eprintf("ascii85: invalid character '%c'\n", c);
			exit(1);
		}
		tuple += (c - '!') * pows[count++];
		if (count == 5) {
			decode_tuple(tuple, count);
			tuple = 0;
			count = 0;
		}
	}
}



extern "C"
void scan_ascii85(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "ascii85";
        sp.info->author         = "Simson L. Garfinkel";
        sp.info->description    = "scans for ASCII85 encoded data";
        sp.info->scanner_version= "1.0";
        sp.info->flags          = scanner_info::SCANNER_RECURSE;
	return;	/* No feature files created */
    }
    if(sp.phase==scanner_params::shutdown) return;		// no cleanup
    if(sp.phase==scanner_params::scan){
	const sbuf_t &sbuf = sp.sbuf;
	for(int ch='0';ch<='9';ch++){ base64array[ch] = true; }
	base64array_init=true;
    }
}


