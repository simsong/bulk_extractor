/**
 * exif_entry: - reads an entry of an IFD type to provide a name and value pair
 *               as strings for use by bulk_extractor
 * Revision history:
 * 2012-jan-05 bda - Created.
 *
 * Reading approach: read every entry available,
 * but disregard entries whose size conflicts with the tag.
 * Abort reading when input is invalid, but report valid entries read so far.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>
#include "config.h"

#include "utf8.h" // for reading UTF16 and UTF32
#include "unicode_escape.h" // for validating or escaping UTF8
#include "exif_reader.h"

// #define DEBUG 1

static const uint32_t OPT_MAX_IFD_ENTRIES = 1000;	// don't parse more than this
static const uint32_t OPT_MAX_EXIF_BYTES = 64 * 1024; // rough sanity limit
static const bool opt_exif_suppress_unknown_entry_types = true; // suppress undefined entry types

// entry types
static const uint16_t EXIF_BYTE = 1;		// uint8
static const uint16_t EXIF_ASCII = 2;		// byte with 7-bit ascii
static const uint16_t EXIF_SHORT = 3;		// uint16
static const uint16_t EXIF_LONG = 4;		// uint32
static const uint16_t EXIF_RATIONAL = 5;	// two LONGs
static const uint16_t EXIF_UNDEFINED = 7;	// byte whose value depends on the field definition
static const uint16_t EXIF_SLONG = 9;		// signed int32
static const uint16_t EXIF_SRATIONAL = 10;	// two SLONGs

// constants
static const uint32_t MAX_IMAGE_SIZE = 65535;

// private helpers
static std::string get_value(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_generic_name(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static void add_entry(ifd_type_t ifd_type, const std::string &name, const std::string &value,
                      entry_list_t &entries);
static bool chars_match(const sbuf_t &sbuf, size_t count);
static bool char_pairs_match(const sbuf_t &sbuf, size_t count);
static std::string get_possible_utf8(const sbuf_t &sbuf, size_t count);
static std::string get_possible_utf16(const sbuf_t sbuf, size_t count, sbuf_t::byte_order_t byte_order);
static std::string get_possible_utf32(const sbuf_t &sbuf, size_t count, sbuf_t::byte_order_t byte_order);

// IFD field readers for reading the 4 fields of a 12-byte IFD record
inline static uint16_t get_entry_tag(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint16_t get_entry_type(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint32_t get_entry_count(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint16_t get_data_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint32_t get_ifd_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);

// EXIF type readers to return strings customized for bulk_extractor
static std::string get_exif_byte(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_exif_ascii(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_exif_short(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_exif_long(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_exif_rational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_exif_undefined(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_exif_slong(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static std::string get_exif_srational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);

exif_entry::exif_entry(uint16_t ifd_type_, const std::string &name_, const std::string &value_)
    :ifd_type(ifd_type_), name(name_), value(value_) {
}

exif_entry::exif_entry(const exif_entry &that):ifd_type(that.ifd_type), name(that.name), value(that.value) {
}

exif_entry::~exif_entry() {
#ifdef DEBUG
    std::cout << "exif_entry.~ " << name.length() << "val " << value.length() << "\n";
#endif
}

const std::string exif_entry::get_full_name() const {
    switch(ifd_type) {
    case IFD0_TIFF:
        return "ifd0.tiff." + name;	// as labeled by Exif doc JEITA CP-3451B
    case IFD0_EXIF:
        return "ifd0.exif." + name;	// as labeled by Exif doc JEITA CP-3451B
    case IFD0_GPS:
        return "ifd0.gps." + name;	// as labeled by Exif doc JEITA CP-3451B
    case IFD0_INTEROPERABILITY:
        return "ifd0.interoperability." + name;
    case IFD1_TIFF:
        return "ifd1.tiff." + name;	// as labeled by Exif doc JEITA CP-3451B
    case IFD1_EXIF:
        return "ifd1.exif." + name;	// as labeled by Exif doc JEITA CP-3451B
    case IFD1_GPS:
        return "ifd1.gps." + name;	// as labeled by Exif doc JEITA CP-3451B
    case IFD1_INTEROPERABILITY:
        return "ifd1.interoperability." + name;
    default:
        std::cerr << "Program state errror: Invalid ifd type " << ifd_type << "\n";
        assert(0);
    }
    return "ERROR";			// required to avoid compiler warning
}
 
/** 
 * parse_ifd_entries() extracts entries from an offset given its type.
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
void entry_reader::parse_ifd_entries(ifd_type_t ifd_type, tiff_handle_t &tiff_handle,
				     size_t ifd_offset, entry_list_t &entries) {
    uint16_t num_entries = tiff_ifd_reader::get_num_entries(tiff_handle, ifd_offset);
#ifdef DEBUG
    std::cout << "exif_entry.parse_ifd_entries ifd type " << (uint32_t)ifd_type << ", num entries: " << num_entries << " from offset " << ifd_offset << "\n";
#endif
    if (num_entries == 0) {
        return;
    }

    // abort the EXIF if its number of entries is considered excessive
    // truncate excessive entries
    if (num_entries > OPT_MAX_IFD_ENTRIES) throw exif_failure_exception_t();

    // parse each entry
    for (uint16_t i=0; i<num_entries; i++) {
        // quick sanity check - have we parsed more than 64K?
        if (tiff_handle.bytes_read >= OPT_MAX_EXIF_BYTES) throw exif_failure_exception_t();

        uint32_t ifd_entry_offset = tiff_ifd_reader::get_entry_offset(ifd_offset, i);
        if (ifd_entry_offset + 12 > tiff_handle.sbuf->bufsize) throw exif_failure_exception_t();
#ifdef DEBUG
	std::cout << "exif_entry.parse_ifd_entries ifd_entry_offset: " << ifd_entry_offset << "\n";
#endif

        // parse the entry
        parse_entry(ifd_type, tiff_handle, ifd_entry_offset, entries);
    }
}

void entry_reader::parse_entry(ifd_type_t ifd_type, tiff_handle_t &tiff_handle,
			       size_t ifd_entry_offset, entry_list_t &entries) {
/** 
 * parse_ifd_entry() parses one entry and places it into entries.
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */

    // check now that the 12 entry bytes are available
    if (ifd_entry_offset + 12 > tiff_handle.sbuf->bufsize) {
	// no space for entry record
	return;
    }

    // entry tag
    const uint16_t entry_tag = get_entry_tag(tiff_handle, ifd_entry_offset);
#ifdef DEBUG
    const uint16_t zentry_type = get_entry_type(tiff_handle, ifd_entry_offset);
    uint32_t zcount = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t zoffset = get_data_offset(tiff_handle, ifd_entry_offset);
    std::cout << "exif_entry.parse_entry entry tag: " << entry_tag << ", entry type: " << zentry_type << " entry count: " << zcount << ", entry offset: " << zoffset << "\n";
#endif

    // add entry or parse entry's nested IFD, based on entry_tag
#ifdef DEBUG
    std::cout << "exif_entry.parse_entry IFD type: " << (int)ifd_type << "\n";
#endif
    std::string generic_name;
    std::string value_string;
    switch (ifd_type) {
    case IFD0_TIFF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
    case IFD1_TIFF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
        uint32_t forwarded_ifd_offset;
        switch (entry_tag) {
	case 0x0100: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ImageWidth", value_string, entries);
            break;
        }
	case 0x0102: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "BitsPerSample", value_string, entries);
            break;
        }
	case 0x0103: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Compression", value_string, entries);
            break;
        }
	case 0x0106: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "PhotometricInterpreation", value_string, entries);
            break;
        }
	case 0x010e: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ImageDescription", value_string, entries);
            break;
        }
	case 0x010f: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Make", value_string, entries);
            break;
        }
	case 0x0110: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Model", value_string, entries);
            break;
        }
	case 0x0111: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "StripOffsets", value_string, entries);
            break;
        }
	case 0x0112: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Orientation", value_string, entries);
            break;
        }
	case 0x0115: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SamplesPerPixel", value_string, entries);
            break;
        }
	case 0x0116: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "RowsPerStrip", value_string, entries);
            break;
        }
	case 0x0117: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "StripByteCounts", value_string, entries);
            break;
        }
	case 0x011a: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "XResolution", value_string, entries);
            break;
        }
	case 0x011b: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "YResolution", value_string, entries);
            break;
        }
	case 0x011c: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "PlanarConfiguration", value_string, entries);
            break;
        }
	case 0x0128: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ResolutionUnit", value_string, entries);
            break;
        }
	case 0x012d: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "TransferFunction", value_string, entries);
            break;
        }
	case 0x0131: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Software", value_string, entries);
            break;
        }
	case 0x0132: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "DateTime", value_string, entries);
            break;
        }
	case 0x013b: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Artist", value_string, entries);
            break;
        }
	case 0x013e: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "WhitePoint", value_string, entries);
            break;
        }
	case 0x013f: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "PrimaryChromaticities", value_string, entries);
            break;
        }
	case 0x0201: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "JPEGInterchangeFormat", value_string, entries);
            break;
        }
	case 0x0202: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "JPEGInterchangeFormatLength", value_string, entries);
            break;
        }
	case 0x0211: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "YCbCrCoefficients", value_string, entries);
            break;
        }
	case 0x0212: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "YCbCrSubSampling", value_string, entries);
            break;
        }
	case 0x0213: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "YCbCrPositioning", value_string, entries);
            break;
        }
	case 0x0214: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ReferenceBlackWhite", value_string, entries);
            break;
        }
	case 0x8298: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "", value_string, entries);
            break;
        }
	case 0x8769: // EXIF tag
            forwarded_ifd_offset = get_ifd_offset(tiff_handle, ifd_entry_offset);
            if (ifd_type == IFD0_TIFF) {
#ifdef DEBUG
                std::cout << "exif_entry.parse_ifd_entries case8769 IFD0_tiff to IFD0_EXIF: ifd_entry_offset: " << ifd_entry_offset << " forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD0_EXIF, tiff_handle, forwarded_ifd_offset, entries);
            } else if (ifd_type == IFD1_TIFF) {
#ifdef DEBUG
                std::cout << "exif_entry.parse_ifd_entries case8769 IFD1_tiff to IFD1_EXIF: ifd_entry_offset:" << ifd_entry_offset << " forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD1_EXIF, tiff_handle, forwarded_ifd_offset, entries);
            } else {
		assert(0);
            }
            break;
	case 0x8825: // GPS tag
            forwarded_ifd_offset = get_ifd_offset(tiff_handle, ifd_entry_offset);
            if (ifd_type == IFD0_TIFF) {
#ifdef DEBUG
                std::cout << "exif_entry.parse_ifd_entries case8825 IFD0_tiff to IFD0_GPS: forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD0_GPS, tiff_handle, forwarded_ifd_offset, entries);
            } else if (ifd_type == IFD1_TIFF) {
#ifdef DEBUG
                std::cout << "exif_entry.parse_ifd_entries case8825 IFD1_tiff to IFD1_GPS: forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD1_GPS, tiff_handle, forwarded_ifd_offset, entries);
            } else {
		assert(0);
            }
            break;
	default: {
            if (opt_exif_suppress_unknown_entry_types) {
                // suppress the unrecognized entry
                return;
            }

            generic_name = get_generic_name(tiff_handle, ifd_entry_offset);
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, generic_name, value_string, entries);
            break;
        } // end default
        } // end switch entry_tag
        break;
    case IFD0_EXIF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
    case IFD1_EXIF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
        switch (entry_tag) {
	case 0x829a: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ExposureTime", value_string, entries);
            break;
        }
	case 0x829d: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FNumber", value_string, entries);
            break;
        }
	case 0x8822: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ExposureProgram", value_string, entries);
            break;
        }
	case 0x8824: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SpectralSensitivity", value_string, entries);
            break;
        }
	case 0x8827: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "PhotographicSensitivity", value_string, entries);
            break;
        }
	case 0x8828: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "OECF", value_string, entries);
            break;
        }
	case 0x8830: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SensitivityType", value_string, entries);
            break;
        }
	case 0x8831: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "StandardOutputSensitivity", value_string, entries);
            break;
        }
	case 0x8832: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "RecommendedExposureIndex", value_string, entries);
            break;
        }
	case 0x8833: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ISOSpeed", value_string, entries);
            break;
        }
	case 0x8834: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ISOSpeedLatitudeyyy", value_string, entries);
            break;
        }
	case 0x8835: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "IOSpeedLatitudezzz", value_string, entries);
            break;
        }
	case 0x9000: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ExifVersion", value_string, entries);
            break;
        }
	case 0x9003: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "DateTimeOriginal", value_string, entries);
            break;
        }
	case 0x9004: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "DateTimeDigitized", value_string, entries);
            break;
        }
	case 0x9101: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ComponentsConfiguration", value_string, entries);
            break;
        }
	case 0x9102: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "CompressedBitsPerPixel", value_string, entries);
            break;
        }
	case 0x9201: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ShutterSpeedValue", value_string, entries);
            break;
        }
	case 0x9202: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ApertureValue", value_string, entries);
            break;
        }
	case 0x9203: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "BrightnessValue", value_string, entries);
            break;
        }
	case 0x9204: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ExposureBiasValue", value_string, entries);
            break;
        }
	case 0x9205: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "MaxApertureValue", value_string, entries);
            break;
        }
	case 0x9206: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SubjectDistance", value_string, entries);
            break;
        }
	case 0x9207: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "MeteringMode", value_string, entries);
            break;
        }
	case 0x9208: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "LightSource", value_string, entries);
            break;
        }
	case 0x9209: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Flash", value_string, entries);
            break;
        }
	case 0x920a: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FocalLength", value_string, entries);
            break;
        }
	case 0x9214: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SubjectArea", value_string, entries);
            break;
        }
	case 0x927c: {
            // currently, we skip the MakerNote entry rather than adding
            // it to entries, so no action.
#ifdef DEBUG
            value_string = get_value(tiff_handle, ifd_entry_offset);
            std::cout << "exif_entry.add_entry ifd type: '" << (int)ifd_type << "' (skipped)\n";
            std::cout << "exif_entry.add_entry name: '" << "MakerNote" << "' (skipped)\n";
            std::cout << "exif_entry.add_entry value: '" << value_string << "' (skipped)\n";
#endif
            //value_string = get_value(tiff_handle, ifd_entry_offset);
            //add_entry(ifd_type, "MakerNote", value_string, entries);
            break;
        }
	case 0x9286: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "UserComment", value_string, entries);
            break;
        }
	case 0x9290: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SubSecTime", value_string, entries);
            break;
        }
	case 0x9291: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SubSecTimeOriginal", value_string, entries);
            break;
        }
	case 0x9292: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SubSecTimeDigitized", value_string, entries);
            break;
        }
	case 0xa000: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FlashpixVersion", value_string, entries);
            break;
        }
	case 0xa001: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ColorSpace", value_string, entries);
            break;
        }
	case 0xa002: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            uint32_t lx = atol(value_string.c_str());
            if (lx > MAX_IMAGE_SIZE) {
              throw exif_failure_exception_t();
            }
            add_entry(ifd_type, "PixelXDimension", value_string, entries);
            break;
        }
	case 0xa003: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            uint32_t ly = atol(value_string.c_str());
            if (ly > MAX_IMAGE_SIZE) {
              throw exif_failure_exception_t();
            }
            add_entry(ifd_type, "PixelYDimension", value_string, entries);
            break;
        }
	case 0xa004: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "RelatedSoundFile", value_string, entries);
            break;
        }
	case 0xa005: // Interoperability tag
            forwarded_ifd_offset = get_ifd_offset(tiff_handle, ifd_entry_offset);
            if (ifd_type == IFD0_EXIF) {
#ifdef DEBUG
                std::cout << "exif_entry.parse_ifd_entries casea005 IFD0_EXIF to IFD0_INTEROP: ifd_entry_offset: " << ifd_entry_offset << ", forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD0_INTEROPERABILITY, tiff_handle, forwarded_ifd_offset, entries);
            } else if (ifd_type == IFD1_EXIF) {
#ifdef DEBUG
                std::cout << "exif_entry.parse_ifd_entries casea005 IFD1_EXIF to IFD1_INTEROP: forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD1_INTEROPERABILITY, tiff_handle, forwarded_ifd_offset, entries);
            } else {
		assert(0);
            }
            break;
	case 0xa20b: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FlashEnergy", value_string, entries);
            break;
        }
	case 0xa20c: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SpatialFrequencyResponse", value_string, entries);
            break;
        }
	case 0xa20e: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FocalPlaneXResolution", value_string, entries);
            break;
        }
	case 0xa20f: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FocalPlaneYResolution", value_string, entries);
            break;
        }
	case 0xa210: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FocalPlaneResolutionUnit", value_string, entries);
            break;
        }
	case 0xa214: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SubjectLocation", value_string, entries);
            break;
        }
	case 0xa215: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ExposureIndex", value_string, entries);
            break;
        }
	case 0xa217: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SensingMethod", value_string, entries);
            break;
        }
	case 0xa300: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FileSource", value_string, entries);
            break;
        }
	case 0xa301: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SceneType", value_string, entries);
            break;
        }
	case 0xa302: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "CFAPattern", value_string, entries);
            break;
        }
	case 0xa401: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "CustomRendered", value_string, entries);
            break;
        }
	case 0xa402: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ExposureMode", value_string, entries);
            break;
        }
	case 0xa403: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "WhiteBalance", value_string, entries);
            break;
        }
	case 0xa404: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "DigitalZoomRatio", value_string, entries);
            break;
        }
	case 0xa405: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "FocalLengthIn35mmFilm", value_string, entries);
            break;
        }
	case 0xa406: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SceneCaptureType", value_string, entries);
            break;
        }
	case 0xa407: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GainControl", value_string, entries);
            break;
        }
	case 0xa408: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Contrast", value_string, entries);
            break;
        }
	case 0xa409: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Saturation", value_string, entries);
            break;
        }
	case 0xa40a: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Sharpness", value_string, entries);
            break;
        }
	case 0xa40b: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "DeviceSettingDescription", value_string, entries);
            break;
        }
	case 0xa40c: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "SubjectDistanceRange", value_string, entries);
            break;
        }
	case 0xa420: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "ImageUniqueID", value_string, entries);
            break;
        }
	case 0xa430: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "CameraOwnerName", value_string, entries);
            break;
        }
	case 0xa431: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "BodySerialNumber", value_string, entries);
            break;
        }
	case 0xa432: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "LensSpecification", value_string, entries);
            break;
        }
	case 0xa433: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "LensMake", value_string, entries);
            break;
        }
	case 0xa434: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "LensModel", value_string, entries);
            break;
        }
	case 0xa435: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "LensSerialNumber", value_string, entries);
            break;
        }
	case 0xa500: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "Gamma", value_string, entries);
            break;
        }
	default: {
            if (opt_exif_suppress_unknown_entry_types) {
                // suppress the unrecognized entry
                return;
            }

            generic_name = get_generic_name(tiff_handle, ifd_entry_offset);
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, generic_name, value_string, entries);
            break;
        } // end default
        } // end switch entry_tag
        break;
    case IFD0_GPS:
    case IFD1_GPS:
        switch (entry_tag) {
	case 0x0000: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSVersionID", value_string, entries);
            break;
        }
	case 0x0001: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSLatitudeRef", value_string, entries);
            break;
        }
	case 0x0002: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSLatitude", value_string, entries);
            break;
        }
	case 0x0003: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSLongitudeRef", value_string, entries);
            break;
        }
	case 0x0004: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSLongitude", value_string, entries);
            break;
        }
	case 0x0005: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSAltitudeRef", value_string, entries);
            break;
        }
	case 0x0006: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSAltitude", value_string, entries);
            break;
        }
	case 0x0007: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSTimeStamp", value_string, entries);
            break;
        }
	case 0x0008: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSSatellites", value_string, entries);
            break;
        }
	case 0x0009: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSStatus", value_string, entries);
            break;
        }
	case 0x000a: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSMeasureMode", value_string, entries);
            break;
        }
	case 0x000b: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDOP", value_string, entries);
            break;
        }
	case 0x000c: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSSpeedRef", value_string, entries);
            break;
        }
	case 0x000d: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSSpeed", value_string, entries);
            break;
        }
	case 0x000e: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSTrackRef", value_string, entries);
            break;
        }
	case 0x000f: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSTrack", value_string, entries);
            break;
        }
	case 0x0010: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSImgDirectionRef", value_string, entries);
            break;
        }
	case 0x0011: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSImgDirection", value_string, entries);
            break;
        }
	case 0x0012: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSMapDatum", value_string, entries);
            break;
        }
	case 0x0013: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestLatitudeRef", value_string, entries);
            break;
        }
	case 0x0014: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestLatitude", value_string, entries);
            break;
        }
	case 0x0015: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestLongitudeRef", value_string, entries);
            break;
        }
	case 0x0016: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestLongitude", value_string, entries);
            break;
        }
	case 0x0017: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestBearingRef", value_string, entries);
            break;
        }
	case 0x0018: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestBearing", value_string, entries);
            break;
        }
	case 0x0019: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestDistanceRef", value_string, entries);
            break;
        }
	case 0x001a: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDestDistance", value_string, entries);
            break;
        }
	case 0x001b: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSProcessingMethod", value_string, entries);
            break;
        }
	case 0x001c: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSAreaInformation", value_string, entries);
            break;
        }
	case 0x001d: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDateStamp", value_string, entries);
            break;
        }
	case 0x001e: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSDifferential", value_string, entries);
            break;
        }
	case 0x001f: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "GPSHPositioningError", value_string, entries);
            break;
        }
	default: {
            if (opt_exif_suppress_unknown_entry_types) {
                // suppress the unrecognized entry
                return;
            }

            generic_name = get_generic_name(tiff_handle, ifd_entry_offset);
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, generic_name, value_string, entries);
            break;
        } // end default
        } // end switch entry_tag
        break;
    case IFD0_INTEROPERABILITY:
    case IFD1_INTEROPERABILITY:
        switch (entry_tag) {
	case 0x0001: {
            value_string = get_value(tiff_handle, ifd_entry_offset);
            add_entry(ifd_type, "InteroperabilityIndex", value_string, entries);
            break;
        }
	default: {
            // do nothing by default; the following catch-all was recording huge amounts of redundant data
            //if (opt_exif_suppress_unknown_entry_types) {
            //    // suppress the unrecognized entry
            //    return;
            //}

            //generic_name = get_generic_name(tiff_handle, ifd_entry_offset);
            //value_string = get_value(tiff_handle, ifd_entry_offset);
            //add_entry(ifd_type, generic_name, value_string, entries);
            //break;
        } // end default
        } // end switch (entry_tag)
        break;

    default:
        // not a valid type
        std::cerr << "exif_entry.parse_entry invalid entry tag: " << entry_tag << "\n";
        assert(0);
    } // end switch ifd_type
}

// entry value
static std::string get_value(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // entry type and entry count
    const uint16_t entry_type = get_entry_type(tiff_handle, ifd_entry_offset);

    // value
    switch(entry_type) {
    case EXIF_BYTE:
        return get_exif_byte(tiff_handle, ifd_entry_offset);
    case EXIF_ASCII:
        return get_exif_ascii(tiff_handle, ifd_entry_offset);
    case EXIF_SHORT:
        return get_exif_short(tiff_handle, ifd_entry_offset);
    case EXIF_LONG:
        return get_exif_long(tiff_handle, ifd_entry_offset);
    case EXIF_RATIONAL:
        return get_exif_rational(tiff_handle, ifd_entry_offset);
    case EXIF_UNDEFINED:
        return get_exif_undefined(tiff_handle, ifd_entry_offset);
    case EXIF_SLONG:
        return get_exif_slong(tiff_handle, ifd_entry_offset);
    case EXIF_SRATIONAL:
        return get_exif_srational(tiff_handle, ifd_entry_offset);
    default:
        // NOTE: in future, we may wish to log the invalid entry_type to diagnostics
        //       when in full diagnostics mode.

        // the type is not valid, so use best-effort and return value for UNDEFINED type
        return get_exif_undefined(tiff_handle, ifd_entry_offset);
    }
}

// generic entry value
static std::string get_generic_name(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // create a generic name as "entry(N)"
    uint16_t entry_tag;
    try {
        entry_tag = tiff_handle.sbuf->get16u(ifd_entry_offset + 0, tiff_handle.byte_order);
    } catch (const sbuf_t::range_exception_t &e) {
        entry_tag = 0;
    }
    std::stringstream ss;
    ss << "entry_0x";
    ss.width(4);
    ss.fill('0');
    ss << std::hex << entry_tag;
    std::string name = ss.str();

    return name;
}

static void add_entry(ifd_type_t ifd_type, const std::string &name, const std::string &value,
                      entry_list_t &entries) {

    // push the name and value onto entries
#ifdef DEBUG
    std::cout << "exif_entry.add_entry ifd type: '" << (int)ifd_type << "'\n";
    std::cout << "exif_entry.add_entry name: '" << name << "'\n";
    std::cout << "exif_entry.add_entry value: '" << value << "'\n";
#endif

    entries.push_back(new exif_entry(ifd_type, name, value));
}

inline static uint16_t get_entry_tag(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    try {
        return tiff_handle.sbuf->get16u(ifd_entry_offset + 0, tiff_handle.byte_order);
    } catch (const sbuf_t::range_exception_t &e) {
        return 0;
    }
}

inline static uint16_t get_entry_type(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    try {
        return tiff_handle.sbuf->get16u(ifd_entry_offset + 2, tiff_handle.byte_order);
    } catch (const sbuf_t::range_exception_t &e) {
        return 0;
    }
}

// Note: protocol requires that entry count < 64K
inline static uint32_t get_entry_count(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // get requested count
    uint32_t requested_count;
    try {
        requested_count = tiff_handle.sbuf->get32u(ifd_entry_offset + 4, tiff_handle.byte_order);
    } catch (const sbuf_t::range_exception_t &e) {
        requested_count = 0;
    }
    return requested_count;
}

inline static uint16_t get_data_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // get entry type
    uint16_t entry_type;
    try {
        entry_type = tiff_handle.sbuf->get16u(ifd_entry_offset + 2, tiff_handle.byte_order);
    } catch (const sbuf_t::range_exception_t &e) {
        return 0;
    }

    // determine byte width
    uint32_t byte_width;
    switch(entry_type) {
    case EXIF_BYTE: byte_width = 1; break;
    case EXIF_ASCII: byte_width = 1; break;
    case EXIF_SHORT: byte_width = 2; break;
    case EXIF_LONG: byte_width = 4; break;
    case EXIF_RATIONAL: byte_width = 8; break;
    case EXIF_UNDEFINED: byte_width = 1; break;
    case EXIF_SLONG: byte_width = 4; break;
    case EXIF_SRATIONAL: byte_width = 8; break;
    default: byte_width = 0; break;	// not valid input
    }

    // no offset if byte width is invalid
    if (byte_width == 0) {
        // no offset
        return 0;
    }

    // get requested count
    uint32_t requested_count;
    try {
        requested_count = tiff_handle.sbuf->get32u(ifd_entry_offset + 4, tiff_handle.byte_order);
    } catch (const sbuf_t::range_exception_t &e) {
        requested_count = 0;
    }

    // calculate requested byte size
    uint32_t requested_byte_size = requested_count * byte_width;

    // get the data offset, which is either at entry offset + 8 or is pointed to by offset
    if (requested_byte_size <= 4) {
        // the data is in the value offset field
	return ifd_entry_offset + 8;
    } else {
        // look up the value offset
        try {
            return tiff_handle.sbuf->get32u(ifd_entry_offset + 8, tiff_handle.byte_order);
        } catch (const sbuf_t::range_exception_t &e) {
            // return an out-of-range value
            return ifd_entry_offset + 12;
        }
    }
}

// Note: protocol requires that ifd offset < 64K
inline static uint32_t get_ifd_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    try {
        return tiff_handle.sbuf->get32u(ifd_entry_offset + 8, tiff_handle.byte_order);
    } catch (const sbuf_t::range_exception_t &e) {
        // return an out-of-range value
        return ifd_entry_offset + 12;
    }
}

// ************************************************************
// This section provides interfaces get_possible_utf16, and get_possible_utf32.
// These interfaces return every character as UTF8, along with escaped invalid characters.
// These interfaces use the byte ordering requested.
// An empty string is returned if an evaluation indicates no data.
// ************************************************************
static bool chars_match(const sbuf_t &sbuf, size_t count) {

    if (count < 2 || sbuf.pagesize < 2) {
        return false;
    }

    uint8_t b0 = sbuf[0];
    for (size_t i=1; i<count; i++) {
        if (i == sbuf.pagesize) {
            return true;
        }
        if (b0 != sbuf[i]) {
            return false;
        }
    }
    return true;
}

// check if all the character pairs are the same value
static bool char_pairs_match(const sbuf_t &sbuf, size_t count) {
    if (count < 4 || count % 2 != 0 || sbuf.pagesize < 4) {
        return false;
    }

    uint8_t b0 = sbuf[0];
    uint8_t b1 = sbuf[1];

    for (size_t i=2; i<count; i+=2) {
        // check byte0
        if (i == sbuf.pagesize) {
            return true;
        }
        if (b0 != sbuf[i]) {
            return false;
        }
        // check byte1
        if (i+1 == sbuf.pagesize) {
            return true;
        }
        if (b1 != sbuf[i+1]) {
            return false;
        }
    }
    return true;
}

static std::string get_possible_utf8(const sbuf_t &sbuf, size_t count) {
    if (chars_match(sbuf, count) || char_pairs_match(sbuf, count)) {
        // this is an uninteresting sequence, so return ""
        return "";
    } else {
        std::string s;
        sbuf.getUTF8(0, count, s);
        validateOrEscapeUTF8(s,true,false);
        return s;
    }
}

template <typename u16bit_iterator, typename octet_iterator>
octet_iterator all_utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result);
std::string all_utf16to8 (std::wstring utf16_string);

// read utf16 and return unvalidated utf8
std::string get_possible_utf16(const sbuf_t sbuf, size_t count, sbuf_t::byte_order_t byte_order) {

    // check for sequence
    if (chars_match(sbuf, count) || char_pairs_match(sbuf, count)) {
        // this is an uninteresting sequence, so return ""
        return "";
    }

    // get wstring accounting for byte order
    std::wstring wstr;
    sbuf.getUTF16(0, count, byte_order, wstr);

    // convert wstring to string
    std::string utf8_string = all_utf16to8(wstr);

#ifdef DEBUG
    std::cout << "exif_entry.get_possible_utf16 utf8_string (escaped): '" << validateOrEscapeUTF8(utf8_string, true, false) << "'\n";
#endif
    return utf8_string;
}

// read utf32 and return unvalidated utf8
std::string get_possible_utf32(const sbuf_t &sbuf, size_t count, sbuf_t::byte_order_t byte_order) {

    // check for sequence
    if (chars_match(sbuf, count) || char_pairs_match(sbuf, count)) {
        // this is an uninteresting sequence, so return ""
        return "";
    }
    std::string utf8_string;
    std::back_insert_iterator<std::basic_string<char> > result = back_inserter(utf8_string);

    for (uint32_t i=0; i<count; i++) {
        try {
            uint32_t code_point;
            code_point = sbuf.get32u(i * 4, byte_order);
            try {
                result = utf8::append(code_point, result);
            } catch (const utf8::invalid_code_point &) {

                // invalid code point so put in the byte values directly,
                // disregarding endian convention
                utf8_string += (uint8_t)code_point;
                utf8_string += (uint8_t)code_point/0x100;
                utf8_string += (uint8_t)code_point/0x10000;
                utf8_string += (uint8_t)code_point/0x1000000;
            }
        } catch (const sbuf_t::range_exception_t &e) {
            // at end of buffer
            break;
        }
    }
    return utf8_string;
}

// convert all possible utf16, along with any illegal values
std::string all_utf16to8 (std::wstring utf16_string) {
    std::string utf8_string;
    std::back_insert_iterator<std::basic_string<char> > result = back_inserter(utf8_string);

    std::wstring::const_iterator start = utf16_string.begin();
    std::wstring::const_iterator end = utf16_string.end();

//template <typename u16bit_iterator, typename octet_iterator>
//octet_iterator all_utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result)

    while (start != end) {
        uint32_t code_point = utf8::internal::mask16(*start++);
        // Take care of surrogate pairs first
        if (utf8::internal::is_lead_surrogate(code_point)) {
            if (start != end) {
                uint32_t trail_surrogate = utf8::internal::mask16(*start++);
                if (utf8::internal::is_trail_surrogate(trail_surrogate)) {
                    code_point = (code_point << 10) + trail_surrogate + utf8::internal::SURROGATE_OFFSET;
                } else {
                    // there was an error, but keep invalid code_point anyway rather than raise an exception
                }
            } else {
                // there was an error, but keep invalid code_point anyway rather than raise an exception
            }

        }
        // Lone trail surrogate
        else if (utf8::internal::is_trail_surrogate(code_point)) {
            // there was an error, but keep invalid code_point anyway rather than raise an exception
        }

        try {
            result = utf8::append(code_point, result);
        } catch (const utf8::invalid_code_point &) {
            // invalid code point so put in unescaped byte values, disregarding endian convention
            utf8_string += (uint8_t)code_point;
            utf8_string += (uint8_t)code_point/0x100;
        }
    }
    return utf8_string;
}

// ************************************************************
// END This section provides interfaces get_possible_utf16, and get_possible_utf32.
// ************************************************************
// ************************************************************
// all these ifd value readers return string
// ************************************************************
// EXIF_BYTE uint8
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static std::string get_exif_byte(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
 
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count; // exif standard: 1 exif byte is 1 byte long
    if (count >= 0x10000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    if (count == 1) {
        // count is 1 so print the byte as an integer
        std::stringstream ss;
        try {
	    ss << (int)tiff_handle.sbuf->get8u(offset);
        } catch (const sbuf_t::range_exception_t &e) {
            // add nothing to ss
        }
        return ss.str();

    } else {
        // count is not 1 so return the bytes as utf8
        return get_possible_utf8(*(tiff_handle.sbuf)+offset, count);
    }
}

// EXIF_ASCII byte with 7-bit ascii typecast from uint8_t to int8_t
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static std::string get_exif_ascii(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each byte as a character in a string
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count; // exif standard: 1 exif ascii value is 1 byte long
    if (count >= 0x10000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    // strip off "\0" allowing up to 2 \0 markers because tiff terminates with \0 using byte pairs
    try {
        if (count > 0 && tiff_handle.sbuf->get8u(offset + count - 1) == 0) {
	    count--;
        }
        if (count > 0 && tiff_handle.sbuf->get8u(offset + count - 1) == 0) {
	    count--;
        }
    } catch (const sbuf_t::range_exception_t &e) {
        // at end so there is nothing to strip
    }

    // although 7-bit ascii is expected, parse using UTF-8
    return get_possible_utf8(*(tiff_handle.sbuf)+offset, count);
}

// EXIF_SHORT uint16
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static std::string get_exif_short(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each short separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 2; // exif standard: 1 exif short is 2 bytes long
    if (count >= 0x8000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    if (count == 1) {
        // count is 1 so print the uint16 as an integer
        std::stringstream ss;
        try {
	    ss << (int)tiff_handle.sbuf->get16u(offset, tiff_handle.byte_order);
        } catch (const sbuf_t::range_exception_t &e) {
            // add nothing to ss
        }
        return ss.str();

    } else {
        // count is not 1 so print the uint16_t bytes as utf8
        std::string s = get_possible_utf16(*(tiff_handle.sbuf)+offset, count, tiff_handle.byte_order);
#ifdef DEBUG
        std::cout << "exif_entry.get_exif_short (escaped): '" << validateOrEscapeUTF8(s, true, false) << "'\n";
#endif
        return s;
    }
}

// EXIF_LONG uint32
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static std::string get_exif_long(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {

    // print each long separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 4; // exif standard: 1 exif long is 4 bytes long
    if (count >= 0x4000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    if (count == 1) {
        // count is 1 so print the long directly
        std::stringstream ss;
        try {
	    ss << (uint32_t)tiff_handle.sbuf->get32u(offset, tiff_handle.byte_order);
        } catch (const sbuf_t::range_exception_t &e) {
            // add nothing to ss
        }
        return ss.str();

    } else {
        // count is not 1 so print the uint32_t bytes as utf8
        std::string s = get_possible_utf32(*(tiff_handle.sbuf)+offset, count, tiff_handle.byte_order);
#ifdef DEBUG
        std::cout << "exif_entry.get_exif_short (escaped): '" << validateOrEscapeUTF8(s, true, false) << "'\n";
#endif
        return s;
    }
}

// EXIF_RATIONAL uint64
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static std::string get_exif_rational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each rational as 1'st uint32, "/", 2'nd uint32, separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 8; // exif standard: 1 exif rational is 8 bytes long
    if (count >= 0x2000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    std::stringstream ss;
    for (uint32_t i=0; i<count; i++) {
        try {
	    // return 1'st uint32, "/", 2'nd uint32
            ss << tiff_handle.sbuf->get32u(offset + i * 8, tiff_handle.byte_order) << "/"
               << tiff_handle.sbuf->get32u(offset + i * 8 + 4, tiff_handle.byte_order);
	    if (i + 1 < count) {
	        ss << " ";
	    }
        } catch (const sbuf_t::range_exception_t &e) {
            break;
        }
    }
    return ss.str();
}

// EXIF_UNDEFINED byte whose value depends on the field definition
static std::string get_exif_undefined(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print UNDEFINED as byte
    return get_exif_byte(tiff_handle, ifd_entry_offset);
}

// EXIF_SLONG int32
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static  std::string get_exif_slong(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each signed long
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 4; // exif standard: 1 exif slong is 4 bytes long
    if (count >= 0x4000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    std::stringstream ss;
    for (uint32_t i=0; i<count; i++) {
        try {
	    ss << tiff_handle.sbuf->get32i(offset + i * 4, tiff_handle.byte_order);
        } catch (const sbuf_t::range_exception_t &e) {
            // at end
            break;
        }
	if (i + 1 < count) {
	    ss << " ";
	}
    }

    return ss.str();
}

// EXIF_SRATIONAL int64
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static std::string get_exif_srational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each rational as 1'st uint32, "/", 2'nd uint32, separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 8; // exif standard: 1 exif srational is 8 bytes long
    if (count >= 0x2000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    std::stringstream ss;
    for (uint32_t i=0; i<count; i++) {
        try {
	    // return 1'st int32, "/", 2'nd int32
            ss << tiff_handle.sbuf->get32i(offset + i * 8, tiff_handle.byte_order) << "/"
               << tiff_handle.sbuf->get32i(offset + i * 8 + 4, tiff_handle.byte_order);
	    if (i + 1 < count) {
	        ss << " ";
	    }
        } catch (const sbuf_t::range_exception_t &e) {
            break;
        }
    }
    return ss.str();
}

