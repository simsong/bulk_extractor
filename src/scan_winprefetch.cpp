/**
 * \addtogroup winprefetch_scanner
 * @{
 */
/**
 * \file
 *
 * scan_winprefetch finds windows prefetch entries.
 *
 * Revision history:
 * - Created by Luis E. Garcia II
 * - 2012-apr-27 bda - Optimized for performance and to fix prefetch entries
 *                   that were missed when the scanner ran on Windows.
 *
 * Resources:
 * - http://forensicswiki.org/wiki/Prefetch
 * - http://www.forensicswiki.org/wiki/Windows_Prefetch_File_Format
 * - prefetch research discussion: http://42llc.net/?p=375
 * - a viewer: http://www.nirsoft.net/utils/win_prefetch_view.html
 * - Use of UTF16 in Windows OS: http://en.wikipedia.org/wiki/UTF-16
 */

#include "bulk_extractor.h"

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
#include "xml.h"

// sbuf_stream.h needs integrated into another include file as is done with sbuf_h?
#include "sbuf_stream.h"

/**
 * Instantiates a populated prefetch record from the buffer provided.
 */
class prefetch_record_t {

public:
    std::string prefetch_version;
    uint32_t header_size;
    string   execution_filename;
    uint32_t execution_counter;
    int64_t  execution_time;
    string   volume_path_name;
    uint32_t volume_serial_number;
    int64_t  volume_creation_time;
    vector<std::string> files;
    vector<std::string> directories;

    prefetch_record_t(const sbuf_t &sbuf):prefetch_version(), header_size(0), execution_filename(),
                      execution_counter(0), execution_time(0), volume_path_name(),
                      volume_serial_number(0), volume_creation_time(0),
                      files(), directories() {

        // read fields in order until done or range exception
        try {

            // get prefetch version identifier
            uint8_t prefetch_version_byte = sbuf.get8u(0);

            // set values based on prefetch version
            size_t execution_time_offset=0;
            size_t execution_counter_offset=0;
            if (prefetch_version_byte == 0x11) {
                prefetch_version = "Windows XP";
                execution_time_offset = 0x78;
                execution_counter_offset = 0x90;
            } else if (prefetch_version_byte == 0x17) {
                prefetch_version = "Windows Vista or Windows 7";
                execution_time_offset = 0x80;
                execution_counter_offset = 0x98;
            } else {
                // program error: don't create prefetch_record if this byte is invalid.
                assert(0);
            }

            // size in bytes of the whole prefetch file
            uint32_t prefetch_file_length = sbuf.get32u(0x0c);

            // get execution file filename
            wstring utf16_execution_filename;
            sbuf.getUTF16(0x10, utf16_execution_filename);
            execution_filename = safe_utf16to8(utf16_execution_filename);

            // get the offset to Section A
            uint32_t section_a_offset = sbuf.get32u(0x54);
            header_size = section_a_offset; // header is everything before section A

            // validate the offset since we know what it should be
            if ((prefetch_version_byte == 0x11 && header_size != 0x98)		// XP and 2003
             || (prefetch_version_byte == 0x17 && header_size != 0xf0)) {	// Vista and 7
                // invalid so quit trying
                return;
            }

            // get last execution time
            execution_time = sbuf.get64u(execution_time_offset);

            // get number of execution file runs
            execution_counter = sbuf.get32u(execution_counter_offset);

            // get the list of files from Section C
            uint32_t section_c_offset = sbuf.get32u(0x64);
            uint32_t section_c_length = sbuf.get32u(0x68);
            sbuf_stream filename_stream(sbuf + section_c_offset);
            while (filename_stream.tell() < section_c_length) {
                wstring utf16_filename;
                filename_stream.getUTF16(utf16_filename);
                if (utf16_filename.length() == 0) {
                    break;
                }
                string filename = safe_utf16to8(utf16_filename);
                files.push_back(filename);
            }

            // get the offset to Section D
            uint32_t section_d_offset = sbuf.get32u(0x6c);

            // get the volume name from Section D
            uint32_t volume_name_offset = sbuf.get32u(section_d_offset + 0x00);
            wstring utf16_volume_name;
            sbuf.getUTF16(section_d_offset+volume_name_offset, utf16_volume_name);
            volume_path_name = safe_utf16to8(utf16_volume_name);

            // get volume creation time from Section D
            volume_creation_time = sbuf.get64i(section_d_offset+0x08);

            // get volume serial number from Section D
            volume_serial_number = sbuf.get32u(section_d_offset+0x10);

            // get the directory offset with respect to Section D from Section D subsection 2
            uint32_t section_d_2_offset = sbuf.get32u(section_d_offset + 0x1c);

            // establish the directory offset
            size_t directory_offset = section_d_offset + section_d_2_offset;

            // get the number of directory entries in Section D subsection 2
            uint32_t num_directory_entries = sbuf.get32u(section_d_offset + 0x20);

            // get each of the directory entries from Section D subsection 2
            if (directory_offset > prefetch_file_length) {
                // the offset is out of range so don't get the list of directories
            } else {
                // calculate a rough maximum number of bytes for directory entries
                size_t upper_max = prefetch_file_length - directory_offset;

                sbuf_stream directory_stream = sbuf_stream(sbuf + directory_offset);

                for (uint32_t i=0; i<num_directory_entries; i++) {
                    // break if obviously out of range
                    if (directory_stream.tell() > upper_max) {
                        break;
                    }

                    // for directories, the first int16 is the directory name length.
                    // We read to \U0000 instead so we throw away the directory name length.
                    directory_stream.get16u();

                    // read the directory name
                    wstring utf16_directory_name;
                    directory_stream.getUTF16(utf16_directory_name);
                    if (utf16_directory_name.length() == 0) {
                        break;
                    }
                    string directory_name = safe_utf16to8(utf16_directory_name);
                    directories.push_back(directory_name);
                }
            }
        } catch (sbuf_t::range_exception_t &e) {
            // no action, just return what was gleaned before range exception
        }
    }

    ~prefetch_record_t() {
    }
};

/**
 * Returns an XML string from the prefetch record provided.
 */
static string get_prefetch_xml_string(const prefetch_record_t &prefetch_record);

/**
 * Scanner scan_winprefetch scans and extracts windows prefetch records.
 * It takes scanner_params and recursion_control_block as input.
 * The scanner_params structure includes the following:
 * \li scanner_params::phase identifying the scanner phase.
 * \li scanner_params.sbuf containing the buffer being scanned.
 * \li scanner_params.fs, which provides feature recorder feature_recorder
 * that scan_winprefetch will write to.
 *
 * scan_winprefetch iterates through each byte of sbuf
 * searching for a valid winprefetch match.
 * When a match is found, the prefetch content is extracted, formatted for XML output,
 * and written to the windows prefetch feature recorder feature_recorder
 * using prefetch_record.write(sbuf_t, string, string).
 * Value sbuf_t is used to write the feature's path.
 * The first string is the feature found, in this case the windows prefetch filename is used.
 * The second string is the full feature content, in this case, packed in XML.
 * Method xml::xml_escape() is used to help format XML output.
 */
extern "C"
void scan_winprefetch(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
        sp.info->name		= "winprefetch";
        sp.info->author		= "Bruce Allen";
        sp.info->description	= "Search for Windows Prefetch files";
        sp.info->feature_names.insert("winprefetch");
        return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN){
	
	// phase 1: set up the feature recorder and search for winprefetch features
	const sbuf_t &sbuf = sp.sbuf;
	feature_recorder *winprefetch_recorder = sp.fs.get_name("winprefetch");

	// optimization: first useful data starts after byte 0x10.
	if (sbuf.pagesize <= 0x10) {
	    return;
	}
	size_t stop = sbuf.pagesize - 8;

	// iterate through sbuf searching for winprefetch features
	for (size_t start=0; start < stop; start++) {

	    // check for probable WindowsXP or Windows7 header
	    if ((sbuf[start + 0] == 0x11 || sbuf[start + 0] == 0x17)
		&& sbuf[start + 1] == 0x00
		&& sbuf[start + 2] == 0x00
		&& sbuf[start + 3] == 0x00
		&& sbuf[start + 4] == 0x53
		&& sbuf[start + 5] == 0x43
		&& sbuf[start + 6] == 0x43
		&& sbuf[start + 7] == 0x41) {

		if(debug&DEBUG_INFO) std::cerr << "scan_winprefetch checking match at start " << start << "\n";

		// create the populated prefetch record
		prefetch_record_t prefetch_record(sbuf + start);

		// get the prefetch record
		const string prefetch_xml_string = get_prefetch_xml_string(prefetch_record);

		// record the winprefetch entry
		winprefetch_recorder->write(sp.sbuf.pos0+start,
					    prefetch_record.execution_filename,
					    prefetch_xml_string);

	    }
	}
    }
}

// Private helper functions
static string get_prefetch_xml_string(const prefetch_record_t &prefetch_record) {
    stringstream ss;

    // generate the prefetch feature in a stringstream
    ss << "<prefetch>";
        ss << "<os>" << xml::xmlescape(prefetch_record.prefetch_version) << "</os>";
        ss << "<filename>" << xml::xmlescape(prefetch_record.execution_filename) << "</filename>";
        ss << "<header_size>" << prefetch_record.header_size << "</header_size>";
        ss << "<atime>" << microsoftDateToISODate(prefetch_record.execution_time) << "</atime>";
        ss << "<runs>" << prefetch_record.execution_counter << "</runs>";
        ss << "<filenames>";
            for(vector<string>::const_iterator it = prefetch_record.files.begin();
                it != prefetch_record.files.end(); it++) {
                ss << "<file>" << xml::xmlescape(*it) << "</file>";
            }
        ss << "</filenames>";

        ss << "<volume>";
            ss << "<path>" << xml::xmlescape(prefetch_record.volume_path_name) << "</path>";
            ss << "<creation>" << microsoftDateToISODate(prefetch_record.volume_creation_time) << "</creation>";
            ss << "<serial_number>" << hex << prefetch_record.volume_serial_number << dec << "</serial_number>";

            ss << "<dirnames>";
                for(vector<string>::const_iterator it = prefetch_record.directories.begin();
                    it != prefetch_record.directories.end(); it++) {
                    ss << "<dir>" << xml::xmlescape(*it) << "</dir>";
                }
            ss << "</dirnames>";
        ss << "</volume>";
    ss << "</prefetch>";

    // return the xml as a string
    string prefetch_xml = ss.str();
    return prefetch_xml;
}

