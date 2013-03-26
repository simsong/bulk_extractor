/**
 * scan_exif: - custom exif scanner for identifying specific features for bulk_extractor.
 *
 * We do not require EXIF data to begin on a 512-byte offset boundary.
 *
 * Revision history:
 * 2011-dec-12 bda - Ported from file scan_exif.cpp.
 */

#include "bulk_extractor.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>

#include "xml.h"
#include "md5.h"
#include "exif_reader.h"
#include "unicode_escape.h"

// #define DEBUG 1

static const uint32_t MAX_ENTRY_SIZE = 1000000;

/****************************************************************
 *** formatting code
 ****************************************************************/

/**
 * Used for helping to convert TIFF's GPS format to decimal lat/long
 */

static double stod(string s)
{
    double d=0;
    sscanf(s.c_str(),"%lf",&d);
    return d;
}

static double rational(string s)
{
    std::vector<std::string> parts = split(s,'/');
    if(parts.size()!=2) return stod(s);	// no slash, so return without
    double top = stod(parts[0]);
    double bot = stod(parts[1]);
    return bot>0 ? top / bot : top;
}

static string fix_gps(string s)
{
    std::vector<std::string> parts = split(s,' ');
    if(parts.size()!=3) return s;	// return the original
    double res = rational(parts[0]) + rational(parts[1])/60.0 + rational(parts[2])/3600.0;
    s = dtos(res);
    return s;
}

static string fix_gps_ref(string s)
{
    if(s=="W" || s=="S") return "-";
    return "";
}


/************************************************************
 * exif_reader
 *************************************************************
 */
namespace psd_reader {
    static uint32_t get_uint8_psd(const sbuf_t &exif_sbuf, uint32_t offset);
    static uint32_t get_uint16_psd(const sbuf_t &exif_sbuf, uint32_t offset);
    static uint32_t get_uint32_psd(const sbuf_t &exif_sbuf, uint32_t offset);

    /** 
     * finds the TIFF header within the PSD region, or 0 if invalid
     */
    size_t get_tiff_offset_from_psd(const sbuf_t &exif_sbuf) {
        // validate that the PSD header is "8BPS" ver. 01
        // Ref. Photoshop header, e.g. http://www.fileformat.info/format/psd/egff.htm
        // or exiv2-0.22/src/psdimage.cpp
        if (exif_sbuf.pagesize < 26
         || exif_sbuf[0]!='8'
         || exif_sbuf[1]!='B'
         || exif_sbuf[2]!='P'
         || exif_sbuf[3]!='S'
         || exif_sbuf[4]!= 0
         || exif_sbuf[5]!= 1) {
#ifdef DEBUG
            cout << "scan_exif.get_tiff_offset_from_psd header rejected\n";
#endif
            return 0;
        }

        // validate that the 6 reserved bytes are 0
        if (exif_sbuf[6]!=0 || exif_sbuf[7]!=0 || exif_sbuf[8]!=0 || exif_sbuf[9]!=0
         || exif_sbuf[10]!=0 || exif_sbuf[11]!=0) {
#ifdef DEBUG
            cout << "scan_exif.get_tiff_offset_from_psd reserved bytes rejected\n";
#endif
            return 0;
        }

        // get the size of the color mode data section that is skipped over
        uint32_t color_mode_data_section_length = get_uint32_psd(exif_sbuf, 26);

        // get the size of the list of resource blocks
        uint32_t resource_length = get_uint32_psd(exif_sbuf, 26 + 4 + color_mode_data_section_length);

        // define the offset to the list of resource blocks
        size_t resource_offset_start = 26 + 4 + color_mode_data_section_length + 4;

        // identify the offset to the end of the list of resource blocks
        size_t resource_offset_end = resource_offset_start + resource_length;

        // loop through resource blocks looking for PhotoShop 7.0 Resource ID ExifInfo, 0x0422
        size_t resource_offset = resource_offset_start;
        while (resource_offset < resource_offset_end) {
	      //uint32_t resource_type = get_uint32_psd(exif_sbuf, resource_offset + 0);
            uint16_t resource_id = get_uint16_psd(exif_sbuf, resource_offset + 4);
            uint8_t resource_name_length = get_uint8_psd(exif_sbuf, resource_offset + 6) & 0xfe;
            uint32_t resource_size = get_uint32_psd(exif_sbuf, resource_offset + 8 + resource_name_length);

            // align resource size to word boundary
            resource_size = (resource_size + 1) & 0xfffe;

            // check to see if this resource is ExifInfo
            if (resource_id == 0x0422) {
                size_t tiff_start = resource_offset + 8 + resource_name_length + 4;
#ifdef DEBUG
                cout << "scan_exif.get_tiff_offset_from_psd accepted at tiff_start " << tiff_start << "\n";
#endif
                return tiff_start;
            }
            resource_offset += 8 + resource_name_length + 4 + resource_size;
        }
#ifdef DEBUG
        cout << "scan_exif.get_tiff_offset_from_psd ExifInfo resource was not found\n";
#endif
        return 0;
    }

    static uint32_t get_uint8_psd(const sbuf_t &exif_sbuf, uint32_t offset) {
        // check for EOF
        if (offset + 1 > exif_sbuf.bufsize) return 0;

        // return uint32 in big-endian Motorola byte order
        return static_cast<uint32_t>(exif_sbuf[offset + 1]) << 0;
    }

    static uint32_t get_uint16_psd(const sbuf_t &exif_sbuf, uint32_t offset) {
        // check for EOF
        if (offset + 2 > exif_sbuf.bufsize) return 0;

        // return uint32 in big-endian Motorola byte order
        return static_cast<uint32_t>(exif_sbuf[offset + 0]) << 8
             | static_cast<uint32_t>(exif_sbuf[offset + 1]) << 0;
    }

    static uint32_t get_uint32_psd(const sbuf_t &exif_sbuf, uint32_t offset) {
        // check for EOF
        if (offset + 4 > exif_sbuf.bufsize) return 0;

        // return uint32 in big-endian Motorola byte order
        return static_cast<uint32_t>(exif_sbuf[offset + 0]) << 24
             | static_cast<uint32_t>(exif_sbuf[offset + 1]) << 16
             | static_cast<uint32_t>(exif_sbuf[offset + 2]) << 8
             | static_cast<uint32_t>(exif_sbuf[offset + 3]) << 0;
    }
}


/****************************************************************/
/* C++ string splitting code from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */

using namespace std;

inline size_t min(size_t a,size_t b){
    return a<b ? a : b;
}

/**
 * record exif data in well-formatted XML.
 */
static void record_exif_data(feature_recorder *exif_recorder, const pos0_t &pos0, const string &md5_hex, const entry_list_t &entries)
{
#ifdef DEBUG
    cout << "scan_exif recording data for entry" << "\n";
#endif

    // do not record the exif feature if there are no entries
    if (entries.size() == 0) {
        return;
    }

    // compose xml from all entries
    stringstream ss;
    ss << "<exif>";
    for (entry_list_t::const_iterator it = entries.begin(); it!=entries.end(); it++) {

        // prepare by escaping XML codes.
        string prepared_value = xml::xmlescape((*it)->value);

        // do not report entries that have empty values
        if (prepared_value.length() == 0) {
            continue;
        }

        // validate against maximum entry size
        if (debug & DEBUG_PEDANTIC) {
            if (prepared_value.size() > MAX_ENTRY_SIZE) {
                std::cerr << "ERROR exif_entry: prepared_value.size()==" << prepared_value.size() << "\n" ;
                assert(0);
            }
        }

#ifdef DEBUG
        cout << "scan_exif fed before xmlescape: " << (*it)->value << "\n";
        cout << "scan_exif fed after xmlescape: " << prepared_value << "\n";
#endif
        ss << "<" << (*it)->get_full_name() << ">" << prepared_value << "</" << (*it)->get_full_name() << ">";
    }
    ss << "</exif>";

    // record the formatted exif entries
    exif_recorder->write(pos0, md5_hex, ss.str());
}

/**
 * Record GPS data as comma separated values.
 * Note that GPS data is considered to be present when a GPS IFD entry is present
 * that is not just a time or date entry.
 */
static void record_gps_data(feature_recorder *gps_recorder, const pos0_t &pos0, const string &md5_hex, const entry_list_t &entries)
{
    // desired GPS strings
    string gps_time;
    string gps_date;
    string gps_lon_ref;
    string gps_lon;
    string gps_lat_ref;
    string gps_lat;
    string gps_ele;
    string gps_speed;
    string gps_course;

    // date if GPS date is not available
    string exif_time;
    string exif_date;


    bool has_gps = false;
    bool has_gps_date = false;

    // get the desired GPS strings from the entries
    for (entry_list_t::const_iterator it = entries.begin(); it!=entries.end(); it++) {

        // get timestamp from EXIF IFD just in case it is not available from GPS IFD
        if ((*it)->name.compare("DateTimeOriginal") == 0) {
            exif_time = (*it)->value;

#ifdef DEBUG
            cout << "scan_exif.format_gps_data exif_time: " << exif_time << "\n";
#endif

            if (exif_time.length() == 19) {
                // reformat timestamp to standard ISO8601

                /* change "2011/06/25 12:20:11" to "2011-06-25 12:20:11" */
                /*             ^  ^                     ^  ^             */
                if (exif_time[4] == '/') {
                    exif_time[4] = '-';
                }
                if (exif_time[7] == '/') {
                    exif_time[7] = '-';
                }

                /* change "2011:06:25 12:20:11" to "2011-06-25 12:20:11" */
                /*             ^  ^                     ^  ^             */
                if (exif_time[4] == ':') {
                    exif_time[4] = '-';
                }
                if (exif_time[7] == ':') {
                    exif_time[7] = '-';
                }

                /* Change "2011-06-25 12:20:11" to "2011-06-25T12:20:11" */
                /*                   ^                        ^          */
                if (exif_time[10] == ' ') {
                    exif_time[10] = 'T';
                }
            }
        }

        if ((*it)->ifd_type == IFD0_GPS) {

            // get GPS values from IFD0's GPS IFD
            if ((*it)->name.compare("GPSTimeStamp") == 0) {
                has_gps_date = true;
                gps_time = (*it)->value;
                // reformat timestamp to standard ISO8601
                // change "12 20 11" to "12:20:11"
                if (gps_time.length() == 8) {
                    if (gps_time[4] == ' ') {
                        gps_time[4] = ':';
                    }
                    if (gps_time[7] == ' ') {
                        gps_time[7] = ':';
                    }
                }
            } else if ((*it)->name.compare("GPSDateStamp") == 0) {
                has_gps_date = true;
                gps_date = (*it)->value;
                // reformat timestamp to standard ISO8601
                // change "2011:06:25" to "2011-06-25"
                if (gps_date.length() == 10) {
                    if (gps_date[4] == ':') {
                        gps_date[4] = '-';
                    }
                    if (gps_date[7] == ':') {
                        gps_date[7] = '-';
                    }
                }
            } else if ((*it)->name.compare("GPSLongitudeRef") == 0) {
                has_gps = true;
                gps_lon_ref = fix_gps_ref((*it)->value);
            } else if ((*it)->name.compare("GPSLongitude") == 0) {
                has_gps = true;
                gps_lon = fix_gps((*it)->value);
            } else if ((*it)->name.compare("GPSLatitudeRef") == 0) {
                has_gps = true;
                gps_lat_ref = fix_gps_ref((*it)->value);
            } else if ((*it)->name.compare("GPSLatitude") == 0) {
                has_gps = true;
                gps_lat = fix_gps((*it)->value);
            } else if ((*it)->name.compare("GPSAltitude") == 0) {
                has_gps = true;
                gps_ele = dtos(rational((*it)->value));
            } else if ((*it)->name.compare("GPSSpeed") == 0) {
                has_gps = true;
                gps_speed = dtos(rational((*it)->value));
            } else if ((*it)->name.compare("GPSTrack") == 0) {
                has_gps = true;
                gps_course = (*it)->value;
            }
        }
    }

    // compose the formatted gps value from the desired GPS strings
    // NOTE: desired date format is "2011-06-25T12:20:11" made from "2011:06:25" and "12 20 11"
    if (has_gps) {
        // report GPS
        stringstream ss;
        if (has_gps_date) {
            // use GPS data with GPS date
            ss << gps_date << "T" << gps_time << ",";
        } else {
            // use GPS data with date from EXIF
            ss << exif_time << ",";
        }
        ss << gps_lat_ref << gps_lat << "," << gps_lon_ref << gps_lon << ",";
        ss << gps_ele << "," << gps_speed << "," << gps_course;

        // record the formatted GPS entries
        gps_recorder->write(pos0, md5_hex, ss.str());

    } else {
        // no GPS to report
    }
}

static void clear_entries(entry_list_t &entries) {
    for (entry_list_t::const_iterator it = entries.begin(); it!=entries.end(); it++) {
        delete *it;
    }
    entries.clear();
}

extern "C"
void scan_exif(const class scanner_params &sp,const recursion_control_block &rcb)
{
#ifdef DEBUG
    cout << "scan_exif start phase " << (uint32_t)sp.phase << "\n";
#endif
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);
    if(sp.phase==scanner_params::PHASE_STARTUP){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	sp.info->name		= "exif";
	sp.info->author         = "Bruce Allen";
        sp.info->description    = "Search for EXIF sections in JPEG files";
	sp.info->feature_names.insert("exif");
	sp.info->feature_names.insert("gps");
	return;
    }
    if(sp.phase==scanner_params::PHASE_SHUTDOWN) return;
    if(sp.phase==scanner_params::PHASE_SCAN){

	// phase 1: set up feature recorders and search sbuf for features
	const sbuf_t &sbuf = sp.sbuf;
	feature_recorder *exif_recorder = sp.fs.get_name("exif");

	exif_recorder->set_flag(feature_recorder::FLAG_XML); // to escape all but backslashes

	feature_recorder *gps_recorder = sp.fs.get_name("gps");

	entry_list_t entries;

	// search through sbuf for potential exif content

	// optimization: stop early rather than check for overflow.
	if (sbuf.pagesize < 6) {
	    return;
	}
	size_t stop = sbuf.pagesize - 6; // optimization: stop early rather than check for overflow.

	size_t tiff_offset = 0;
	for (size_t start=0; start < stop; start++) {

	    // exif
	    // check for probable TIFF in Exif
	    if (sbuf[start + 0] == 0xff
		&& sbuf[start + 1] == 0xd8
		&& sbuf[start + 2] == 0xff
		&& (sbuf[start + 3] & 0xf0) == 0xe0) {
#ifdef DEBUG
		cout << "scan_exif checking ffd8ff at start " << start << "\n";
#endif
		// perform thorough check for TIFF in Exif
		size_t possible_tiff_offset_from_exif = exif_reader::get_tiff_offset_from_exif(sbuf+start);
#ifdef DEBUG
		cout << "scan_exif.possible_tiff_offset_from_exif " << possible_tiff_offset_from_exif << "\n";
#endif
		if ((possible_tiff_offset_from_exif != 0)
		    && tiff_reader::is_maybe_valid_tiff(sbuf + start + possible_tiff_offset_from_exif)) {

		    // TIFF in Exif is valid, so process TIFF
		    tiff_offset = start + possible_tiff_offset_from_exif;

#ifdef DEBUG
		    cout << "scan_exif Start processing validated Exif ffd8ff at start " << start << "\n";
#endif
		    // get md5 for this exif
		    string md5_hex = sbuf.md5(start,4096).hexdigest();

		    // get entries for this exif
                    try {
		        tiff_reader::read_tiff_data(sbuf + tiff_offset, entries);
                    } catch (exif_failure_exception_t &e) {
                        // accept whatever entries were gleaned before the exif failure
                    }

#ifdef DEBUG
		    cout << "scan_exif.tiff_offset in ffd8 " << tiff_offset;
#endif

		    // record to exif feature recorder
		    record_exif_data(exif_recorder, sp.sbuf.pos0+start, md5_hex, entries);

		    // record to gps feature recorder
		    record_gps_data(gps_recorder, sp.sbuf.pos0+start, md5_hex, entries);

		    // clear entries for next round
		    clear_entries(entries);
#ifdef DEBUG
		    cout << "scan_exif Done processing validated Exif ffd8ff at start " << start << "\n";
#endif

		}

		// check for probable TIFF in photoshop PSD header
	    } else if (sbuf[start + 0] == '8'
		       && sbuf[start + 1] == 'B'
		       && sbuf[start + 2] == 'P'
		       && sbuf[start + 3] == 'S'
		       && sbuf[start + 4] == 0
		       && sbuf[start + 5] == 1) {

#ifdef DEBUG
	        cout << "scan_exif checking 8BPS at start " << start << "\n";
#endif
		// perform thorough check for TIFF in photoshop PSD
		size_t possible_tiff_offset_from_psd = psd_reader::get_tiff_offset_from_psd(sbuf+start);
#ifdef DEBUG
		cout << "scan_exif.psd possible_tiff_offset_from_psd " << possible_tiff_offset_from_psd << "\n";
#endif
		if ((possible_tiff_offset_from_psd != 0)
		    && tiff_reader::is_maybe_valid_tiff(sbuf + start + possible_tiff_offset_from_psd)) {
		    // TIFF in PSD is valid, so process TIFF
		    tiff_offset = start + possible_tiff_offset_from_psd;
#ifdef DEBUG
		    cout << "scan_exif Start processing validated Photoshop 8BPS at start " << start << " tiff_offset " << tiff_offset << "\n";
#endif

		    // get md5 for this exif
		    string md5_hex = sbuf.md5(start,4096).hexdigest();

		    // get entries for this exif
                    try {
		        tiff_reader::read_tiff_data(sbuf + tiff_offset, entries);
                    } catch (exif_failure_exception_t &e) {
                        // accept whatever entries were gleaned before the exif failure
                    }

		    // record to exif feature recorder
		    record_exif_data(exif_recorder, sp.sbuf.pos0+start, md5_hex, entries);

		    // record to gps feature recorder
		    record_gps_data(gps_recorder, sp.sbuf.pos0+start, md5_hex, entries);

		    // clear entries for next round
		    clear_entries(entries);
#ifdef DEBUG
		    cout << "scan_exif Done processing validated Photoshop 8BPS at start " << start << "\n";
#endif

		}

		// check for probable TIFF not in embedded header found above
	    } else if (
		// don't rescan a TIFF from an embedded format above
		(start != tiff_offset)
		// Intel
		&& ((sbuf[start + 0] == 'I'
		     && sbuf[start + 1] == 'I'
		     && sbuf[start + 2] == 42
		     && sbuf[start + 3] == 0)

		    // Motorola
		    || (sbuf[start + 0] == 'M'
			&& sbuf[start + 1] == 'M'
			&& sbuf[start + 2] == 0
			&& sbuf[start + 3] == 42))) {

		// probably a match so check further
		if (tiff_reader::is_maybe_valid_tiff(sbuf+start)) {
		    // treat this as a valid match
#ifdef DEBUG
		    cout << "scan_exif Start processing validated TIFF II42 or MM42 at start " << start << ", last tiff_offset: " << tiff_offset << "\n";
#endif

		    // get entries for this exif
                    try {
		        tiff_reader::read_tiff_data(sbuf + start, entries);
                    } catch (exif_failure_exception_t &e) {
                        // accept whatever entries were gleaned before the exif failure
                    }

                    // there is no MD5 because there is no associated file for this TIFF marker
		    string feature_text = "00000000000000000000000000000000";

		    // record to exif feature recorder
		    record_exif_data(exif_recorder, sp.sbuf.pos0+start, feature_text, entries);

		    // record to gps feature recorder
		    record_gps_data(gps_recorder, sp.sbuf.pos0+start, feature_text, entries);

		    // clear entries for next round
		    clear_entries(entries);
#ifdef DEBUG
		    cout << "scan_exif Done processing validated TIFF II42 or MM42 at start " << start << "\n";
#endif
		}
	    }
	}
    }
}

