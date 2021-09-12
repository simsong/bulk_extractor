#include <cstdlib>
#include <string>
#include <iostream>
#include <iomanip>
#include <cassert>


#include "config.h"
#include "sbuf_decompress.h"
#include "be13_api/scanner_params.h"
#include "dfxml_cpp/src/dfxml_writer.h"
#include "utf8.h"


static const std::string  ZIP_RECORDER_NAME {"zip"};

// these are not tunable
static uint32_t  zip_max_uncompr_size = 256*1024*1024; // don't decompress objects larger than this
static uint32_t  zip_min_uncompr_size = 6;	// don't bother with objects smaller than this
static uint32_t  zip_name_len_max = 1024;

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
	if (ch<' ') return true;
    }
    return false;
}

/* We should have a threadsafe set */

/**
 * code from tsk3
 */

#include "tsk3_fatdirs.h"

const uint32_t   MIN_ZIP_SIZE = 38;     // minimum size of a zip header and file name
/**
 * given a location in an sbuf, determine if it contains a zip component.
 * If it does and if it passes validity tests, unzip and recurse.
 */
inline void scan_zip_component(scanner_params &sp, feature_recorder &zip_recorder, size_t pos)
{

    const sbuf_t &sbuf = (*sp.sbuf);
    const pos0_t &pos0 = sbuf.pos0;

    /* Local file header */
    uint16_t version_needed_to_extract= sbuf.get16u(pos+4);
    uint16_t general_purpose_bit_flag = sbuf.get16u(pos+6);
    uint16_t compression_method=sbuf.get16u(pos+8);
    uint16_t lastmodtime=sbuf.get16u(pos+10);
    uint16_t lastmoddate=sbuf.get16u(pos+12);
    uint32_t crc32=sbuf.get32u(pos+14);	        /* not used needed */
    uint32_t compr_size=sbuf.get32u(pos+18);
    uint32_t uncompr_size=sbuf.get32u(pos+22);
    uint16_t name_len=sbuf.get16u(pos+26);
    uint16_t extra_field_len=sbuf.get16u(pos+28);

    if ((name_len<=0) || (name_len > zip_name_len_max)) return;	 // unreasonable name length
    if (pos+30+name_len > sbuf.bufsize) return;                  // name is bigger than what's left

    std::string name = sbuf.substr(pos+30,name_len);
    /* scan for unprintable characters, which means this isn't a validate zip header
     * Name may contain UTF-8
     */
    if (utf8::find_invalid(name.begin(),name.end()) != name.end()) return; // invalid utf8 in name; not valid zip header
    if (has_control_characters(name)) return; // no control characters allowed in name.
    name=dfxml_writer::xmlescape(name);     // make sure it is escaped

    if (name.size()==0) name="<NONAME>";    // If no name is provided, use this

    /* Save details of the zip header */
    std::string mtime = fatDateToISODate(lastmoddate,lastmodtime);
    char b2[1024];
    snprintf(b2, sizeof(b2),
             "<zipinfo><name>%s</name>"
             "<version>%d</version><general>%d</general><compression_method>%d</compression_method>"
             "<uncompr_size>%d</uncompr_size><compr_size>%d</compr_size>"
             "<mtime>%s</mtime><crc32>%u</crc32>"
             "<extra_field_len>%d</extra_field_len>",
             name.c_str(), version_needed_to_extract,
             general_purpose_bit_flag, compression_method,uncompr_size, compr_size,
             mtime.c_str(),
             crc32, extra_field_len);
    std::stringstream xmlstream;
    xmlstream << b2;

    /* OpenOffice makes invalid ZIP files with compr_size=0 and uncompr_size=0.
     * If compr_size==uncompr_size==0, then assume it may go to the end of the sbuf.
     */
    if (uncompr_size==0 && compr_size==0){
        uncompr_size = zip_max_uncompr_size;
        compr_size   = zip_max_uncompr_size;
    }

    if (uncompr_size > zip_max_uncompr_size){
        uncompr_size = zip_max_uncompr_size; // don't uncompress bigger than 16MB
    }

    /* See if we can decompress */
    if (version_needed_to_extract==20 && uncompr_size>=zip_min_uncompr_size){
        // Create an sbuf that contains the source data pointed to by the header that is to be decompressed
        size_t header_size = 30+name_len+extra_field_len; // how far past 'pos' that the decompress starts
        const sbuf_t sbuf_src(sbuf, pos+header_size);

        // If there is no data, then just indicate this and return.
        if (sbuf_src.pagesize==0){
            xmlstream << "<disposition>end-of-buffer</disposition></zipinfo>";
            zip_recorder.write(pos0+pos,name,xmlstream.str());
            return;
        }

        /* If depth is more than 0, don't decompress if we have seen this component before */
        if (sbuf_src.depth() > 0){
            if (sp.check_previously_processed(sbuf_src)){
                xmlstream << "<disposition>previously-processed</disposition></zipinfo>";
                zip_recorder.write(pos0+pos,name,xmlstream.str());
                return;
            }
        }

        auto *decomp = sbuf_decompress::sbuf_new_decompress(sbuf_src, uncompr_size, "ZIP", sbuf_decompress::mode_t::ZIP, header_size);
        if (decomp!=nullptr) {
            xmlstream << "<disposition bytes='" << decomp->bufsize << "'>decompressed</disposition></zipinfo>";
            zip_recorder.write(pos0+pos,name,xmlstream.str());

            std::string carve_name("_"); // begin with a _
            for(auto const &it : name ){
                carve_name.push_back((it=='/' || it=='\\') ? '_' : it);
            }
            zip_recorder.carve(*decomp, carve_name, mtime);

            // recurse. Remember that recurse will free the sbuf
            sp.recurse( decomp );
        } else {
            xmlstream << "<disposition>decompress-failed</disposition></zipinfo>";
            zip_recorder.write(pos0+pos,name,xmlstream.str());
        }
    }
}

extern "C"
void scan_zip(scanner_params &sp)
{
    sp.check_version();

    if (sp.phase==scanner_params::PHASE_INIT){
        feature_recorder_def::flags_t flags;
        flags.xml = true;
        flags.carve = true;
        sp.info->set_name("zip" );
        sp.info->scanner_flags.recurse = true;
	sp.info->feature_defs.push_back( feature_recorder_def(ZIP_RECORDER_NAME, flags ));
        sp.get_scanner_config("zip_min_uncompr_size",&zip_min_uncompr_size,"Minimum size of a ZIP uncompressed object");
        sp.get_scanner_config("zip_max_uncompr_size",&zip_max_uncompr_size,"Maximum size of a ZIP uncompressed object");
        sp.get_scanner_config("zip_name_len_max",&zip_name_len_max,"Maximum name of a ZIP component filename");
	return;
    }

    if (sp.phase==scanner_params::PHASE_SCAN){
	const sbuf_t &sbuf = (*sp.sbuf);

        if (sbuf.bufsize < MIN_ZIP_SIZE) return;

        feature_recorder &zip_recorder   = sp.named_feature_recorder(ZIP_RECORDER_NAME);

	for(size_t i=0 ; i < sbuf.pagesize && i < sbuf.bufsize-MIN_ZIP_SIZE; i++){
	    /** Look for signature for beginning of a ZIP component. */
	    if (sbuf[i]==0x50 && sbuf[i+1]==0x4B && sbuf[i+2]==0x03 && sbuf[i+3]==0x04){
                scan_zip_component(sp, zip_recorder, i);
	    }
	}
    }
}
