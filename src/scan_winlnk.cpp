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
 * - 2015-apr-08 bda - scan nested data structures per MS-SHLLINK Doc
 *                     in particualr to capture remote links
 *
 * Resources:
 * - http://msdn.microsoft.com/en-us/library/dd871305.aspx
 * - https://forensicswiki.xyz/wiki/index.php?title=LNK
 */

#include "config.h"

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <sstream>
#include <vector>

#include "utf8.h"
#include "be13_api/utils.h"              // needs config.h
#include "be13_api/scanner_params.h"
#include "be13_api/unicode_escape.h"
#include "dfxml_cpp/src/dfxml_writer.h"

static const size_t SMALLEST_LNK_FILE = 150;  // did you see smaller LNK file?

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

/*
 * This is called a lot, so we take a const ref to the sbuf and an offset.
 */
size_t read_StringData(const std::string& tagname, const sbuf_t &sbuf, size_t offset,
                       const bool is_unicode, dfxml_writer::strstrmap_t& lnkmap)
{
    // get count to read
    const uint16_t count = sbuf.get16u(offset+0);
    const size_t size = 2 + count * (is_unicode ? 2 : 1);

    if (count >= 4000) {
        // skip unreasonably long count
        return size;
    }

    if (is_unicode) {
        // get string data from UTF16 input
        std::wstring utf16_data = sbuf.getUTF16(offset+2, count);
        std::string data = safe_utf16to8(utf16_data);
        if (data.size()==0) data="INVALID_DATA";
        lnkmap[tagname] = data;
    } else {
        // get string data from UTF8 input
        std::string utf8_string = sbuf.getUTF8(offset+2, count);
        validateOrEscapeUTF8(utf8_string,true,false,false);
        if (utf8_string.size()==0) utf8_string="INVALID_DATA";
        lnkmap[tagname] = utf8_string;
    }
    return size;
}

void read_utf8(const std::string& tagname, const sbuf_t &sbuf, size_t offset, dfxml_writer::strstrmap_t& lnkmap) {
    std::string utf8_string = sbuf.getUTF8(offset);
    validateOrEscapeUTF8(utf8_string,true,false,false);
    // there can be fields with size 0 so skip them
    if (utf8_string.size()>0) {
        lnkmap[tagname] = utf8_string;
    }
}

void read_utf16(const std::string& tagname, const sbuf_t &sbuf, size_t offset, dfxml_writer::strstrmap_t& lnkmap) {
    std::wstring utf16_string = sbuf.getUTF16(offset);
    std::string utf8_string = safe_utf16to8(utf16_string);
    if (utf8_string.size()>0) {
        lnkmap[tagname] = utf8_string;
    }
}

size_t read_LinkTargetIDList(const sbuf_t &sbuf, size_t offset, dfxml_writer::strstrmap_t& lnkmap) {
    // there are no fields to record in this structure so just advance by size
    const uint16_t size = sbuf.get16u(offset+0);
    return (size_t)size;
}

void read_VolumeID(const sbuf_t &sbuf, size_t offset, dfxml_writer::strstrmap_t& lnkmap) {
    const uint32_t VolumeLabelOffset = sbuf.get32u(offset+12);
    if (VolumeLabelOffset == 0x14) {
        const uint32_t VolumeLabelOffsetUnicode = sbuf.get32u(offset+16);
        read_utf16("volume_label", sbuf, offset+VolumeLabelOffsetUnicode, lnkmap);
    } else {
        read_utf8("volume_label", sbuf, offset+VolumeLabelOffset, lnkmap);
    }
}

void read_CommonNetworkRelativeLink(const sbuf_t &sbuf, size_t offset, dfxml_writer::strstrmap_t& lnkmap) {

    // data state
    //const uint32_t CommonNetworkRelativeLinkSize = sbuf.get32u(0);
    const uint32_t CommonNetworkRelativeLinkFlags = sbuf.get32u(offset+4);

    // NetName
    const uint32_t NetNameOffset = sbuf.get32u(offset+8);
    read_utf8("net_name", sbuf, offset+NetNameOffset, lnkmap);

    // device name
    if (CommonNetworkRelativeLinkFlags & (1 << 0)) {    // A ValidDevice
        const uint32_t DeviceNameOffset = sbuf.get32u(offset+12);
        read_utf8("device_name", sbuf, offset+DeviceNameOffset, lnkmap);
    }

    // network provider type
    // this field is currently not reported.

    // NetNameOffsetUnicode
    if (NetNameOffset > 0x14) {
        const uint32_t NetNameOffsetUnicode = sbuf.get32u(offset+20);
        read_utf16("net_name_unicode", sbuf, offset+NetNameOffsetUnicode, lnkmap);
    }

    // DeviceNameOffsetUnicode
    if (NetNameOffset > 0x14) {
        const uint32_t DeviceNameOffsetUnicode = sbuf.get32u(offset+24);
        read_utf16("device_name_unicode", sbuf, offset+DeviceNameOffsetUnicode, lnkmap);
    }
}

size_t read_LinkInfo(const sbuf_t &sbuf, size_t offset, dfxml_writer::strstrmap_t& lnkmap) {

    // data state
    const uint32_t LinkInfoSize = sbuf.get32u(offset+0);

    const uint32_t LinkInfoHeaderSize = sbuf.get32u(offset+4);
    const uint32_t LinkInfoFlags = sbuf.get32u(offset+8);
    const uint32_t VolumeIDOffset = sbuf.get32u(offset+12);

    // volume ID
    if (LinkInfoFlags & (1 << 0)) {    // A VolumeIDAndLocalBasePath
        read_VolumeID(sbuf, offset+VolumeIDOffset, lnkmap);
    }

    // local base path
    if (LinkInfoFlags & (1 << 0)) {    // A VolumeIDAndLocalBasePath
        const uint32_t LocalBasePathOffset = sbuf.get32u(offset+16);
        read_utf8("local_base_path", sbuf, offset+LocalBasePathOffset, lnkmap);
    }

    // common network relative link
    if (LinkInfoFlags & (1 << 1)) {    // B CommonNetworkRelativeLinkAndPathSuffix
        const uint32_t CommonNetworkRelativeLinkOffset = sbuf.get32u(offset+20);
        read_CommonNetworkRelativeLink(sbuf, offset+CommonNetworkRelativeLinkOffset, lnkmap);
    }

    // common path suffix
    const uint32_t CommonPathSuffixOffset = sbuf.get32u(offset+24);
    read_utf8("common_path_suffix", sbuf, offset+CommonPathSuffixOffset, lnkmap);

    // local base path unicode
    if (LinkInfoFlags & (1 << 0)) {    // A VolumeIDAndLocalBasePath
        if (LinkInfoHeaderSize >=0x24) {
            const uint32_t LocalBasePathOffsetUnicode = sbuf.get32u(offset+28);
            read_utf16("local_base_path_unicode", sbuf, offset+LocalBasePathOffsetUnicode, lnkmap);
        }
    }

    // common path suffix unicode
    if (LinkInfoHeaderSize >=0x24) {
        const uint32_t CommonPathSuffixOffsetUnicode = sbuf.get32u(offset+32);
        read_utf16("common_path_suffix_unicode", sbuf, offset+CommonPathSuffixOffsetUnicode, lnkmap);
    }

    return LinkInfoSize;
}

// top level data structrue, true if has data
// This one doesn't take an offset; we make a child sbuf because we use it a lot
bool read_ShellLinkHeader(const sbuf_t &sbuf, size_t pos, dfxml_writer::strstrmap_t& lnkmap)
{
    sbuf_t sb2 = sbuf.slice(pos);
    // record fields in this header
    const uint64_t CreationTime   = sb2.get64u(0x001c);
    const uint64_t AccessTime     = sb2.get64u(0x0024);
    const uint64_t WriteTime      = sb2.get64u(0x002c);
    lnkmap["ctime"] = microsoftDateToISODate(CreationTime);
    lnkmap["atime"] = microsoftDateToISODate(AccessTime);
    lnkmap["wtime"] = microsoftDateToISODate(WriteTime);

    // flags dictating how the structure will be parsed
    const uint32_t LinkFlags      = sb2.get32u(0x0014);
    const bool is_unicode         = LinkFlags & (1 << 7);

    // read the optional fields
    size_t offset = 0x004c;
    if (LinkFlags & (1 << 0)) {    // LinkFlags A HasLinkTargetIDList
        offset += read_LinkTargetIDList(sb2, offset, lnkmap) + 2;
    }

    // If I return here, it doesn't crash

    if (LinkFlags & (1 << 1)) {    // LinkFlags B HasLinkInfo
        offset += read_LinkInfo(sb2, offset, lnkmap); // causing crash
    }

    // what if we return here?
    return false;                       // remove me


    if (LinkFlags & (1 << 2)) {    // LinkFlags C HasName
        offset += read_StringData("name_string", sb2, offset, is_unicode, lnkmap);
    }
    if (LinkFlags & (1 << 3)) {    // LinkFlags D HasRelativePath
        offset += read_StringData("relative_path", sb2, offset, is_unicode, lnkmap);
    }
    if (LinkFlags & (1 << 4)) {    // LinkFlags E HasWorkingDir
        offset += read_StringData("working_dir", sb2, offset, is_unicode, lnkmap);
    }
    if (LinkFlags & (1 << 5)) {    // LinkFlags F HasArguments
        offset += read_StringData("command_line_arguments", sb2, offset, is_unicode, lnkmap);
    }
    if (LinkFlags & (1 << 6)) {    // LinkFlags G HasIconLocation
        offset += read_StringData("icon_location", sb2, offset, is_unicode, lnkmap);
    }

    int lkcount = 0;
    while (lkcount < 20) { // there should be at most 11 of these

        // read any extra data blocks
        const uint32_t BlockSize = sb2.get32u(offset+0);
        if (BlockSize < 4) {
            // the end block is defined as having BlockSize<4
            break;
        }

        // only some block types are interesting
        const uint32_t BlockSignature = sb2.get32u(offset+4);
        if (BlockSize == 0x60 && BlockSignature == 0xa0000003) {

            // Tracker Data Block
            std::string dvolid = get_guid(sb2, offset+32);
            lnkmap["droid_volumeid"] = dvolid;
            lnkmap["droid_fileid"] = get_guid(sb2, offset+48);
            lnkmap["birth_volumeid"] = get_guid(sb2, offset+64);
            lnkmap["birth_fileid"] = get_guid(sb2, offset+80);
        }

        offset += BlockSize;
        lkcount += 1;
    }

    // sometimes blank featues exist
    if (LinkFlags == 0 && lkcount == 0) {
        return false;        // blank
    } else {

        return true;        // has data
    }
}

// return existing field else ""
// make this more efficient? It's pretty lame.
// On the other hand, it works and it should only be envoked for links when we find them.
std::string get_field(const dfxml_writer::strstrmap_t& lnkmap, const std::string& name) {
    return lnkmap.find(name) == lnkmap.end() ? "" : lnkmap.find(name)->second;
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
static feature_recorder *winlnk_recorder = nullptr;

extern "C"
void scan_winlnk(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT){
        sp.info = std::make_unique<scanner_params::scanner_info>(scan_winlnk,"winlnk");
        sp.info->author		= "Simson Garfinkel";
        sp.info->description	= "Search for Windows LNK files";
        sp.info->feature_defs.push_back( feature_recorder_def("winlnk"));
        sp.info->min_sbuf_size = SMALLEST_LNK_FILE;
        return;
    }
    if (sp.phase==scanner_params::PHASE_INIT2){
        winlnk_recorder = &sp.named_feature_recorder("winlnk");
    }

    if (sp.phase==scanner_params::PHASE_SCAN){

	// phase 1: set up the feature recorder and search for winlnk features
	const sbuf_t &sbuf = *(sp.sbuf);

        for (size_t pos=0;(pos < sbuf.pagesize) &&  (pos + SMALLEST_LNK_FILE < sbuf.bufsize ); pos++){

            // look for Shell Link (.LNK) binary file format magic number
            // Move this to a b-m search.
            if ( sbuf.get32u(pos+0x00) == 0x0000004c &&      // header size
                 sbuf.get32u(pos+0x04) == 0x00021401 &&      // LinkCLSID 1
                 sbuf.get32u(pos+0x08) == 0x00000000 &&      // LinkCLSID 2
                 sbuf.get32u(pos+0x0c) == 0x000000c0 &&      // LinkCLSID 3
                 sbuf.get32u(pos+0x10) == 0x46000000 ){      // LinkCLSID 4

                // container for reported fields
		dfxml_writer::strstrmap_t lnkmap;

                // read
                bool has_data = true;
                try {
                    has_data = read_ShellLinkHeader(sbuf, pos, lnkmap);
                } catch (sbuf_t::range_exception_t &e) {
                    // add error field to indicate that the read was not complete
                    lnkmap["error"] = "LINKINFO_DATA_ERROR";
                }

                // set path when no data
                std::string path = "";
                if (!has_data) {
                    path = "NO_LINKINFO";
                }

                // pick a path value from one of the lnkmap fields
                if (path == "") path = get_field(lnkmap, "local_base_path");
                if (path == "") path = get_field(lnkmap, "local_base_path_unicode");
                if (path == "") path = get_field(lnkmap, "common_path_suffix");
                if (path == "") path = get_field(lnkmap, "common_path_suffix_unicode");
                if (path == "") path = get_field(lnkmap, "name_string");
                if (path == "") path = get_field(lnkmap, "relative_path");
                if (path == "") path = get_field(lnkmap, "net_name");
                if (path == "") path = get_field(lnkmap, "net_name_unicode");
                if (path == "") path = get_field(lnkmap, "device_name");
                if (path == "") path = get_field(lnkmap, "device_name_unicode");
                if (path == "") path = get_field(lnkmap, "working_dir");
                if (path == "") path = get_field(lnkmap, "command_line_arguments");
                if (path == "") path = get_field(lnkmap, "volume_label");
                if (path == "") path = get_field(lnkmap, "droid_volumeid");
                if (path == "") path = "LINKINFO_PATH_EMPTY"; // nothing to assign to path

                // record
                winlnk_recorder->write(sbuf.pos0+pos, path, dfxml_writer::xmlmap(lnkmap,"lnk",""));
            }
        }
    }
}
