/**
 * \addtogroup winlnk
 * @{
 */
/**
 * \file
 *
 * scan_winlnk finds windows LNK files
 *
 * Revision history:
 * - 2014-apr-30 slg - created 
 *
 * Resources:
 * - http://msdn.microsoft.com/en-us/library/dd871305.aspx
 * - http://www.forensicswiki.org/wiki/LNK
 */

#include "config.h"
#include "be13_api/bulk_extractor_i.h"

#if defined(HAVE_LIBLNK_H) && defined(HAVE_LIBBFIO_H) && defined(HAVE_LIBLNK) && defined(HAVE_LIBBFIO)
#include "liblnk.h"
#include "libbfio.h"
#define USE_LIBLNK
#else
#undef USE_LIBLNK
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sstream>
#include <vector>

#include "utf8.h"
#include "dfxml/src/dfxml_writer.h"

// sbuf_stream.h needs integrated into another include file as is done with sbuf_h?
#include "sbuf_stream.h"

static int debug=0;
const size_t SMALLEST_LNK_FILE = 150;  // did you see smaller LNK file?

/* Extract and form GUID. Needs 16 bytes */
std::string get_guid(const sbuf_t &buf, const size_t offset)
{
    char str[37];
    snprintf(str, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", \
        buf[offset+3], buf[offset+2], buf[offset+1], buf[offset+0],    \
        buf[offset+5], buf[offset+4], buf[offset+7], buf[offset+6],    \
        buf[offset+8], buf[offset+9], buf[offset+10], buf[offset+11],    \
        buf[offset+12], buf[offset+13], buf[offset+14], buf[offset+15]);
    return(std::string(str));
}

/**
 * Scanner scan_winlnk scans and extracts windows lnk records.
 * It takes scanner_params and recursion_control_block as input.
 * The scanner_params structure includes the following:
 * \li scanner_params::phase identifying the scanner phase.
 * \li scanner_params.sbuf containing the buffer being scanned.
 * \li scanner_params.fs, which provides feature recorder feature_recorder
 * that scan_winlnk will write to.
 *
 * scan_winlnk iterates through each byte of sbuf
 */
extern "C"
void scan_winlnk(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name		= "winlnk";
        sp.info->author		= "Simson Garfinkel";
        sp.info->description	= "Search for Windows LNK files";
        sp.info->feature_names.insert("winlnk");
        debug = sp.info->config->debug;
        return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	
	// phase 1: set up the feature recorder and search for winlnk features
	const sbuf_t &sbuf = sp.sbuf;
	feature_recorder *winlnk_recorder = sp.fs.get_name("winlnk");

	// make sure that potential LNK file is large enough and has the correct signature
	if (sbuf.pagesize <= SMALLEST_LNK_FILE){
            return;
        }

        for (size_t p=0;(p < sbuf.pagesize) &&  (p + SMALLEST_LNK_FILE < sbuf.bufsize ); p++){
            if ( sbuf.get32u(p+0x00) == 0x0000004c &&
                 sbuf.get32u(p+0x04) == 0x00021401 &&
                 sbuf.get32u(p+0x08) == 0x00000000 &&
                 sbuf.get32u(p+0x0c) == 0x000000c0 &&
                 sbuf.get32u(p+0x10) == 0x46000000 ){

		dfxml_writer::strstrmap_t lnkmap;
                
                uint32_t LinkFlags      = sbuf.get32u(p+0x0014);
                bool     HasLinkTargetIDList = LinkFlags & (1 << 0);
                bool     HasLinkInfo        = LinkFlags  & (1 << 1);
                //bool     HasName            = LinkFlags  & (1 << 2);
                //bool     HasRelativePath    = LinkFlags  & (1 << 3);
                //bool     HasWorkingDir      = LinkFlags  & (1 << 4);
                //uint32_t FileAttributes = sbuf.get32u(p+0x0018);
                uint64_t CreationTime   = sbuf.get64u(p+0x001c);
                uint64_t AccessTime     = sbuf.get64u(p+0x0024);
                uint64_t WriteTime      = sbuf.get64u(p+0x002c);

                size_t loc = 0x004c;    // location of next section
                
                lnkmap["ctime"] = microsoftDateToISODate(CreationTime);
                lnkmap["atime"] = microsoftDateToISODate(AccessTime);
                lnkmap["wtime"] = microsoftDateToISODate(WriteTime);

                if (HasLinkTargetIDList ){
                    uint16_t IDListSize = sbuf.get16u(p+loc);
                    loc += IDListSize + 2;
                }

                std::string path("NO_LINKINFO");

                if (HasLinkInfo && p+loc+16+4 < sbuf.bufsize){
                    uint32_t LinkInfoSize       = sbuf.get32u(p+loc);
                    //uint32_t LinkInfoHeaderSize = sbuf.get32u(p+loc+4);
                    //uint32_t LinkInfoFlags      = sbuf.get32u(p+loc+8);
                    //uint32_t VolumeIDOffset      = sbuf.get32u(p+loc+12);
                    uint32_t LocalBasePathOffset = sbuf.get32u(p+loc+16);
                    uint32_t CommonNetworkRelativeLinkOffset = sbuf.get32u(p+loc+20);
                    uint32_t CommonNetworkRelativeLinkPos = p+loc+CommonNetworkRelativeLinkOffset;
                    //uint32_t CommonPathSuffixOffset = sbuf.get32u(p+loc+20);


                    /* copy out the pathname */
                    path = "";
                    for(size_t i = p+loc+LocalBasePathOffset; i<sbuf.bufsize && sbuf[i]; i++){
                        path += sbuf[i];
                    }
                    if(path.size()==0) path="LINKINFO_PATH_EMPTY";
                    lnkmap["path"] = path;
                    loc += LinkInfoSize;


                    // Parsing of network links does not work at the present time
                    (void)CommonNetworkRelativeLinkPos; 
#if 0
                    for(int i=CommonNetworkRelativeLinkPos;i<CommonNetworkRelativeLinkPos+256;i++){
                        fprintf(stderr,"sbuf[%d]=%x\n",i,sbuf[i]);
                    }
                    std::cerr << "CommonNetworkRelativeLinkOffset=" << CommonNetworkRelativeLinkOffset << "\n";
                    

                    uint32_t CommonNetworkRelativeLinkSize = sbuf.get32u(CommonNetworkRelativeLinkPos);
                    uint32_t CommonNetworkRelativeLinkFlags = sbuf.get32u(CommonNetworkRelativeLinkPos+4);
                    uint32_t NetNameOffset = sbuf.get32u(CommonNetworkRelativeLinkPos+8);
                    uint32_t DeviceNameOffset = sbuf.get32u(CommonNetworkRelativeLinkPos+12);
                    uint32_t NetworkProviderType = sbuf.get32u(CommonNetworkRelativeLinkPos+16);
                    uint32_t NetNameOffsetUnicode = sbuf.get32u(CommonNetworkRelativeLinkPos+20);

                    std::cerr << "CommonNetworkRelativeLinkSize="<<CommonNetworkRelativeLinkSize<<"\n";
                    std::cerr << "NetNameOffset=" << NetNameOffset<<"\n";
                    std::cerr << "DeviceNameOffset=" << DeviceNameOffset << "\n";
                    std::cerr << "NetworkProviderType=" << NetworkProviderType << "\n";
                    std::cerr << "NetNameOffsetUnicode=" << NetNameOffsetUnicode << "\n";

 
                    for(size_t i=p+loc+CommonNetworkRelativeLinkSize+NetNameOffset-8;i<sbuf.bufsize;i++){
                        fprintf(stderr,"sbuf[%d]=%x %c\n",i,sbuf[i],sbuf[i]);
                    }
#endif


                }
                winlnk_recorder->write(sbuf.pos0+p,path,dfxml_writer::xmlmap(lnkmap,"lnk",""));
                p += loc;
                continue;
            } 
            /**
             * At present we don't entirely support LNK parsing, and
             * some blocks need to be carved because of this.
             */
            if ( sbuf.get32u(p+0x00) == 0x00000060 &&
                 sbuf.get32u(p+0x04) == 0xa0000003 &&
                 sbuf.get32u(p+0x08) == 0x00000058 &&
                 sbuf.get32u(p+0x0c) == 0x00000000 &&
		 p+80+16 < sbuf.bufsize ){
                
		dfxml_writer::strstrmap_t lnkguid;
                std::string dvolid = get_guid(sbuf, p+32);
                lnkguid["droid_volumeid"] = dvolid;
                lnkguid["droid_fileid"] = get_guid(sbuf, p+48);
                lnkguid["birth_volumeid"] = get_guid(sbuf, p+64);
                lnkguid["birth_fileid"] = get_guid(sbuf, p+80);
                winlnk_recorder->write(sbuf.pos0+p,dvolid,dfxml_writer::xmlmap(lnkguid,"lnk-guid",""));
                p += 96;
            }
        }
    }
}
