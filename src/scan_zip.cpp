#include "bulk_extractor.h"
#include "xml.h"
#include "utf8.h"
#include "md5.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;

/** The start of a ZIP section?? */
inline bool zip_section_start(const u_char *buf){
    return (buf[0]==(unsigned char)'\xff'
	    && buf[1]==(unsigned char)'\xd8'
	    && buf[2]==(unsigned char)'\xff');
}

inline int int2(const u_char *cc){
    return (cc[1]<<8) + cc[0];
}

inline int int4(const u_char *cc){
    return (cc[3]<<24) + (cc[2]<<16) + (cc[1]<<8) + (cc[0]);
}

/* These are to eliminate compiler warnings */
#define ZLIB_CONST
#ifdef HAVE_DIAGNOSTIC_UNDEF
#  pragma GCC diagnostic ignored "-Wundef"
#endif
#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

bool has_control_characters(const string &name)
{
    for(string::const_iterator it = name.begin();it!=name.end();it++){
	unsigned char ch = *it;
	if(ch<' ') return true;
    }
    return false;
}

/* See:
 * http://gcc.gnu.org/onlinedocs/gcc-4.5.0/gcc/Atomic-Builtins.html
 * for information on on __sync_fetch_and_add
 *
 * When max_depth_count>=max_depth_count_bypass,
 * hash the buffer before decompressing and do not decompress if it has already been decompressed.
 */

int scan_zip_name_len_max = 1024;
int zip_show_all=1;
uint32_t max_depth_count = 0;
const uint32_t max_depth_count_bypass = 5;
set<md5_t>seen_set;
pthread_mutex_t seen_set_lock;
extern "C"
void scan_zip(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "zip";
	sp.info->feature_names.insert("zip");
	pthread_mutex_init(&seen_set_lock,NULL);
	return;
    }
    if(sp.phase==scanner_params::scan){
	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;
	feature_recorder_set &fs = sp.fs;
	feature_recorder *zip_recorder = fs.get_name("zip");
	zip_recorder->set_flag(feature_recorder::FLAG_XML); // because we are sending through XML
	for(const unsigned char *cc=sbuf.buf ; cc < sbuf.buf+sbuf.pagesize && cc < sbuf.buf+sbuf.bufsize-38; cc++){
	    /** Look for signature for beginning of a ZIP component. */
	    if(cc[0]==0x50 && cc[1]==0x4B && cc[2]==0x03 && cc[3]==0x04){
		int version=int2(cc+4);
		int compression_method=int2(cc+6);
		int lastmodtime=int2(cc+8);
		int lastmoddate=int2(cc+10);
		int crc32=int4(cc+14);	/* not used needed */
		int compr_size=int4(cc+18);
		int uncompr_size=int4(cc+22);
		int name_len=int2(cc+26);
		int extra_field_len=int2(cc+28);

		if(name_len<=0) continue;				 // no name, must not be valid.
		if(extra_field_len<0) continue;			 // invalid?

		if(name_len > scan_zip_name_len_max) continue;	 // unreasonable name length
		if(cc+30+name_len > sbuf.buf+sbuf.bufsize) continue; // name is bigger than what's left

		string name = string((char *)cc+30,name_len);
		/* scan for unprintable characters.
		 * Name may contain UTF-8
		 */
		if(utf8::find_invalid(name.begin(),name.end()) != name.end()) continue; // invalid utf8 in name; not valid zip header
		if(has_control_characters(name)) continue; // no control characters allowed.
		name=xml::xmlescape(name);

		if(compr_size<0 || uncompr_size<0) continue; // sanity check

		if(name.size()==0){
		    name="<NONAME>";
		}

		/* Save details of the zip header */
		char b2[1024];
		snprintf(b2,sizeof(b2),
			 "<zipinfo><name>%s</name><name_len>%d</name_len>"
			 "<version>%d</version><compression_method>%d</compression_method>"
			 "<uncompr_size>%d</uncompr_size><compr_size>%d</compr_size>"
			 "<lastmodtime>%d</lastmodtime><lastmoddate>%d</lastmoddate><crc32>%u</crc32>"
			 "<extra_field_len>%d</extra_field_len>",
			 name.c_str(),name_len,version,compression_method,uncompr_size,compr_size,lastmodtime,
			 lastmoddate,crc32,extra_field_len);
		stringstream ss;
		ss << b2;

		ssize_t pos = cc-sbuf.buf;				 // position of the buffer
		const unsigned char *data_buf = cc+30+name_len+extra_field_len; // where the data is
		if(data_buf > sbuf.buf+sbuf.bufsize){ // past the end of buffer?
		    ss << "<disposition>end-of-buffer</disposition></zipinfo>";
		    zip_recorder->write(pos0+pos,name,ss.str());
		    continue; 
		}

		/* OpenOffice makes invalid ZIP files with compr_size=0 and uncompr_size=0.
		 * If compr_size==uncompr_size==0, then assume it may go to the end of the sbuf.
		 */
		if(uncompr_size==0 && compr_size==0){
		    uncompr_size = max_uncompr_size;
		    compr_size = max_uncompr_size;
		}

		/* See if we can decompress */
		if(version==20 && uncompr_size>=min_uncompr_size){ 
		    if(uncompr_size > max_uncompr_size){
			uncompr_size = max_uncompr_size; // don't uncompress bigger than 16MB
		    }

		    // don't decompress beyond end of buffer
		    if((u_int)compr_size > sbuf.bufsize - (data_buf-sbuf.buf)){ 
			compr_size = sbuf.bufsize - (data_buf-sbuf.buf);
		    }

		    /* See if we should perform a bypass check */
		    if(__sync_fetch_and_add(&max_depth_count,0)>=max_depth_count_bypass){
			md5_t hash = md5_generator::hash_buf(data_buf,compr_size);
			pthread_mutex_lock(&seen_set_lock);
			bool in_buf = seen_set.find(hash) != seen_set.end();
			seen_set.insert(hash);
			pthread_mutex_unlock(&seen_set_lock);
			if(in_buf){
			    ss << "<disposition>compression-bomb</disposition></zipinfo>";
			    zip_recorder->write(pos0+pos,name,ss.str());
			    continue;
			}
		    }

		    managed_malloc<Bytef>dbuf(uncompr_size);

		    if(!dbuf.buf){
			ss << "<disposition>calloc-failed</disposition></zipinfo>";
			zip_recorder->write(pos0+pos,name,ss.str());
			continue;
		    }

		    z_stream zs;
		    memset(&zs,0,sizeof(zs));
		
		    zs.next_in = (Bytef *)data_buf; // note that next_in should be typedef const but is not
		    zs.avail_in = compr_size;
		    zs.next_out = dbuf.buf;
		    zs.avail_out = uncompr_size;
		
		    int r = inflateInit2(&zs,-15);
		    if(r==0){
			r = inflate(&zs,Z_SYNC_FLUSH);
			ss << "<disposition bytes='" << zs.total_out << "'>decompressed</disposition></zipinfo>";
			zip_recorder->write(pos0+pos,name,ss.str());

			/* Ignore the error return; process data if we got anything */
			if(zs.total_out>0){
			    const pos0_t pos0_zip = (pos0 + pos) + rcb.partName;
			    const sbuf_t sbuf_new(pos0_zip, dbuf.buf,zs.total_out,zs.total_out,false); // sbuf w/ decompressed data
			    scanner_params spnew(sp,sbuf_new); // scanner_params that points to the sbuf
			
			    /* defend against compression bomb */
			    if(spnew.depth>=scanner_def::max_depth){
				__sync_fetch_and_add(&max_depth_count,1);
				feature_recorder_set::alert_recorder->write(pos0,"scan_zip: MAX DEPTH DETECTED","");
			    } else {
				// process the sbuf
				(*rcb.callback)(spnew); // calling recurisvely
				if(rcb.returnAfterFound){
				    return;
				}
			    }
			}
			r = inflateEnd(&zs);
		    } else {
			ss << "<disposition>decompress-failed</disposition></zipinfo>";
			zip_recorder->write(pos0+pos,name,ss.str());
		    }
		}
	    }
	}
    }
}
