#include "config.h"
#include "be13_api/bulk_extractor_i.h"
#include "dfxml/src/dfxml_writer.h"
#include "utf8.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <cassert>

#define ZIP_RECORDER_NAME "zip"
#define UNZIP_RECORDER_NAME "unzip_carved"

// these are not tunable
static uint32_t  zip_max_uncompr_size = 256*1024*1024; // don't decompress objects larger than this
static uint32_t  zip_min_uncompr_size = 6;	// don't bother with objects smaller than this
static uint32_t  zip_name_len_max = 1024;
const uint32_t   MIN_ZIP_SIZE = 38;     // minimum size of a zip header and file name

// these are tunable
static uint32_t unzip_carve_mode = feature_recorder::CARVE_ENCODED;

/* These are to eliminate compiler warnings */
#define ZLIB_CONST
#ifdef HAVE_DIAGNOSTIC_UNDEF
#  pragma GCC diagnostic ignored "-Wundef"
#endif
#ifdef HAVE_DIAGNOSTIC_CAST_QUAL
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <zlib.h>

bool has_control_characters(const std::string &name)
{
    for(std::string::const_iterator it = name.begin();it!=name.end();it++){
	unsigned char ch = *it;
	if(ch<' ') return true;
    }
    return false;
}

/* We should have a threadsafe set */

/**
 * code from tsk3
 */

/* We have a private version of these #include files in case the system one is not present */
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include "tsk3/libtsk.h"
#include "tsk3/fs/tsk_fatfs.h"
#include "tsk3/fs/tsk_ntfs.h"
#pragma GCC diagnostic warning "-Wshadow"
#pragma GCC diagnostic warning "-Weffc++"
#pragma GCC diagnostic warning "-Wredundant-decls"
//using namespace std;


inline uint16_t fat16int(const uint8_t buf[2]){
    return buf[0] | (buf[1]<<8);
}

inline uint32_t fat32int(const uint8_t buf[4]){
    return buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
}

inline uint32_t fat32int(const uint8_t high[2],const uint8_t low[2]){
    return low[0] | (low[1]<<8) | (high[0]<<16) | (high[1]<<24);
}


inline int fatYear(int x){  return (x & FATFS_YEAR_MASK) >> FATFS_YEAR_SHIFT;}
inline int fatMonth(int x){ return (x & FATFS_MON_MASK) >> FATFS_MON_SHIFT;}
inline int fatDay(int x){   return (x & FATFS_DAY_MASK) >> FATFS_DAY_SHIFT;}
inline int fatHour(int x){  return (x & FATFS_HOUR_MASK) >> FATFS_HOUR_SHIFT;}
inline int fatMin(int x){   return (x & FATFS_MIN_MASK) >> FATFS_MIN_SHIFT;}
inline int fatSec(int x){   return (x & FATFS_SEC_MASK) >> FATFS_SEC_SHIFT;}

inline std::string fatDateToISODate(const uint16_t d,const uint16_t t)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
	     fatYear(d)+1980,fatMonth(d),fatDay(d),
	     fatHour(t),fatMin(t),fatSec(t)); // local time
    return std::string(buf);
}

/**
 * given a location in an sbuf, determine if it contains a zip component.
 * If it does and if it passes validity tests, unzip and recurse.
 */
inline void scan_zip_component(const class scanner_params &sp,const recursion_control_block &rcb,
                               feature_recorder *zip_recorder,feature_recorder *unzip_recorder,size_t pos)
{
    const sbuf_t &sbuf = sp.sbuf;
    const pos0_t &pos0 = sp.sbuf.pos0;
                
    /* Local file header */
    uint16_t version_needed_to_extract= sbuf.get16u(pos+4);
    uint16_t general_purpose_bit_flag = sbuf.get16u(pos+6);
    uint16_t compression_method=sbuf.get16u(pos+8);
    uint16_t lastmodtime=sbuf.get16u(pos+10);
    uint16_t lastmoddate=sbuf.get16u(pos+12);
    uint32_t crc32=sbuf.get32u(pos+14);	/* not used needed */
    uint32_t compr_size=sbuf.get32u(pos+18);
    uint32_t uncompr_size=sbuf.get32u(pos+22);
    uint16_t name_len=sbuf.get16u(pos+26);
    uint16_t extra_field_len=sbuf.get16u(pos+28);

    if((name_len<=0) || (name_len > zip_name_len_max)) return;	 // unreasonable name length
    if(pos+30+name_len > sbuf.bufsize) return; // name is bigger than what's left
    //if(compr_size<0 || uncompr_size<0) return; // sanity check

    std::string name = sbuf.substr(pos+30,name_len);
    /* scan for unprintable characters.
     * Name may contain UTF-8
     */
    if(utf8::find_invalid(name.begin(),name.end()) != name.end()) return; // invalid utf8 in name; not valid zip header
    if(has_control_characters(name)) return; // no control characters allowed.
    name=dfxml_writer::xmlescape(name);     // make sure it is escaped

    if(name.size()==0) name="<NONAME>"; // we want at least something

    /* Save details of the zip header */
    std::string mtime = fatDateToISODate(lastmoddate,lastmodtime);
    char b2[1024];
    snprintf(b2,sizeof(b2),
             "<zipinfo><name>%s</name>"
             "<version>%d</version><general>%d</general><compression_method>%d</compression_method>"
             "<uncompr_size>%d</uncompr_size><compr_size>%d</compr_size>"
             "<mtime>%s</mtime><crc32>%u</crc32>"
             "<extra_field_len>%d</extra_field_len>",
             name.c_str(),version_needed_to_extract,
             general_purpose_bit_flag,compression_method,uncompr_size,compr_size,
             mtime.c_str(),
             crc32,extra_field_len);
    std::stringstream xmlstream;
    xmlstream << b2;

    const unsigned char *data_buf = sbuf.buf+pos+30+name_len+extra_field_len; // where the data starts
    if(data_buf > sbuf.buf+sbuf.bufsize){ // past the end of buffer?
        xmlstream << "<disposition>end-of-buffer</disposition></zipinfo>";
        zip_recorder->write(pos0+pos,name,xmlstream.str());
        return; 
    }

    /* OpenOffice makes invalid ZIP files with compr_size=0 and uncompr_size=0.
     * If compr_size==uncompr_size==0, then assume it may go to the end of the sbuf.
     */
    if(uncompr_size==0 && compr_size==0){
        uncompr_size = zip_max_uncompr_size;
        compr_size = zip_max_uncompr_size;
    }

    /* See if we can decompress */
    if(version_needed_to_extract==20 && uncompr_size>=zip_min_uncompr_size){ 
        if(uncompr_size > zip_max_uncompr_size){
            uncompr_size = zip_max_uncompr_size; // don't uncompress bigger than 16MB
        }

        // don't decompress beyond end of buffer
        if((u_int)compr_size > sbuf.bufsize - (data_buf-sbuf.buf)){ 
            compr_size = sbuf.bufsize - (data_buf-sbuf.buf);
        }

        /* If depth is more than 0, don't decompress if we have seen this component before */
        if(sp.depth>0){
            if(sp.fs.check_previously_processed(data_buf,compr_size)){
                xmlstream << "<disposition>previously-processed</disposition></zipinfo>";
                zip_recorder->write(pos0+pos,name,xmlstream.str());
                return;
            }
        }

        managed_malloc<Bytef>dbuf(uncompr_size);

        if(!dbuf.buf){
            xmlstream << "<disposition>calloc-failed</disposition></zipinfo>";
            zip_recorder->write(pos0+pos,name,xmlstream.str());
            return;
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
            xmlstream << "<disposition bytes='" << zs.total_out << "'>decompressed</disposition></zipinfo>";
            zip_recorder->write(pos0+pos,name,xmlstream.str());

            /* Ignore the error return; process data if we got anything */
            if(zs.total_out>0){
                const pos0_t pos0_zip = (pos0 + pos) + rcb.partName;
                const sbuf_t sbuf_new(pos0_zip, dbuf.buf,zs.total_out,zs.total_out,false); // sbuf w/ decompressed data

                scanner_params spnew(sp,sbuf_new); // scanner_params that points to the sbuf
                (*rcb.callback)(spnew);            // process the sbuf

                /* If we are carving, then carve;
                 * Change any problematic characters to underbars in filename.
                 */
                if(unzip_recorder){
                    std::string carve_name("_"); // begin with a _
                    for(std::string::const_iterator it = name.begin(); it!=name.end();it++){
                        carve_name.push_back((*it=='/' || *it=='\\') ? '_' : *it);
                    }
                    std::string fn = unzip_recorder->carve(sbuf_new,0,sbuf_new.bufsize,carve_name);
                    unzip_recorder->set_carve_mtime(fn,mtime);
                }
            }
            r = inflateEnd(&zs);
        } else {
            xmlstream << "<disposition>decompress-failed</disposition></zipinfo>";
            zip_recorder->write(pos0+pos,name,xmlstream.str());
        }
    }
}

extern "C"
void scan_zip(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);

    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name  = "zip";
        sp.info->flags = scanner_info::SCANNER_RECURSE | scanner_info::SCANNER_RECURSE_EXPAND;
        sp.info->get_config("zip_min_uncompr_size",&zip_min_uncompr_size,"Minimum size of a ZIP uncompressed object");
        sp.info->get_config("zip_max_uncompr_size",&zip_max_uncompr_size,"Maximum size of a ZIP uncompressed object");
        sp.info->get_config("zip_name_len_max",&zip_name_len_max,"Maximum name of a ZIP component filename");
        sp.info->get_config("unzip_carve_mode",&unzip_carve_mode,CARVE_MODE_DESCRIPTION);
	sp.info->feature_names.insert(ZIP_RECORDER_NAME);
        if(unzip_carve_mode){
            sp.info->feature_names.insert(UNZIP_RECORDER_NAME);
        }
	return;
    }
    feature_recorder *zip_recorder = sp.fs.get_name(ZIP_RECORDER_NAME);
    feature_recorder *unzip_recorder = unzip_carve_mode ? sp.fs.get_name(UNZIP_RECORDER_NAME) : 0;
    if(sp.phase==scanner_params::PHASE_INIT && unzip_carve_mode!=feature_recorder::CARVE_NONE){
        unzip_recorder->set_carve_mode(static_cast<feature_recorder::carve_mode_t>(unzip_carve_mode));
        unzip_recorder->set_carve_ignore_encoding("ZIP");
	zip_recorder->set_flag(feature_recorder::FLAG_XML); // because we are sending through XML
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = sp.sbuf;

        if(sbuf.bufsize < MIN_ZIP_SIZE) return;

	for(size_t i=0 ; i < sbuf.pagesize && i < sbuf.bufsize-MIN_ZIP_SIZE; i++){
	    /** Look for signature for beginning of a ZIP component. */
	    if(sbuf[i]==0x50 && sbuf[i+1]==0x4B && sbuf[i+2]==0x03 && sbuf[i+3]==0x04){
                scan_zip_component(sp,rcb,zip_recorder,unzip_recorder,i);
	    }
	}
    }
}
