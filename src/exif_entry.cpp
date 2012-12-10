/**
 * exif_entry: - reads an entry of an IFD type to provide a name and value pair
 *               as strings for use by bulk_extractor
 * Revision history:
 * 2012-jan-05 bda - Created.
 *
 * Reading approach: read every entry available,
 * but disregard entries whose size conflicts with the tag.
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

// private helpers
static void add_entry(ifd_type_t ifd_type, const string &name,
                      tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset,
                      entry_list_t &entries);
static void add_maker_note_entry(ifd_type_t ifd_type, const string &name,
			      tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset,
			      entry_list_t &entries);
static void add_generic_entry(ifd_type_t ifd_type,
			      tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset,
			      entry_list_t &entries);
static bool chars_match(const sbuf_t &sbuf, size_t count);
static bool char_pairs_match(const sbuf_t &sbuf, size_t count);
static string get_possible_utf8(const sbuf_t &sbuf, size_t count);
static string get_possible_utf16(const sbuf_t sbuf, size_t count, sbuf_t::byte_order_t byte_order);
static string get_possible_utf32(const sbuf_t &sbuf, size_t count, sbuf_t::byte_order_t byte_order);

// IFD field readers for reading the 4 fields of a 12-byte IFD record
inline static uint16_t get_entry_tag(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint16_t get_entry_type(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint32_t get_entry_count(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint16_t get_data_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
inline static uint32_t get_ifd_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);

// EXIF type readers to return strings customized for bulk_extractor
static string get_exif_byte(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static string get_exif_ascii(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static string get_exif_short(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static string get_exif_long(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static string get_exif_rational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static string get_exif_undefined(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static string get_exif_slong(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);
static string get_exif_srational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset);

exif_entry::exif_entry(uint16_t ifd_type_, const string &name_, const string &value_)
    :ifd_type(ifd_type_), name(name_), value(value_) {
}

exif_entry::exif_entry(const exif_entry &that):ifd_type(that.ifd_type), name(that.name), value(that.value) {
}

exif_entry::~exif_entry() {
#ifdef DEBUG
    cout << "exif_entry.~ " << name.length() << "val " << value.length() << "\n";
#endif
}

const string exif_entry::get_full_name() const {
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
        cerr << "Program state errror: Invalid ifd type " << ifd_type << "\n";
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
    cout << "exif_entry.parse_ifd_entries ifd type " << (uint32_t)ifd_type << ", num entries: " << num_entries << " from offset " << ifd_offset << "\n";
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
	cout << "exif_entry.parse_ifd_entries ifd_entry_offset: " << ifd_entry_offset << "\n";
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
    cout << "exif_entry.parse_entry entry tag: " << entry_tag << ", entry type: " << zentry_type << " entry count: " << zcount << ", entry offset: " << zoffset;
#endif

    // add entry or parse entry's nested IFD, based on entry_tag
#ifdef DEBUG
    cout << "exif_entry.parse_entry IFD type: " << (int)ifd_type << "\n";
#endif
    switch (ifd_type) {
    case IFD0_TIFF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
    case IFD1_TIFF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
        uint32_t forwarded_ifd_offset;
        switch (entry_tag) {
	case 0x0100: add_entry(ifd_type, "ImageWidth", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0101: add_entry(ifd_type, "ImageLength", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0102: add_entry(ifd_type, "BitsPerSample", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0103: add_entry(ifd_type, "Compression", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0106: add_entry(ifd_type, "PhotometricInterpreation", tiff_handle, ifd_entry_offset, entries); break;
	case 0x010e: add_entry(ifd_type, "ImageDescription", tiff_handle, ifd_entry_offset, entries); break;
	case 0x010f: add_entry(ifd_type, "Make", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0110: add_entry(ifd_type, "Model", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0111: add_entry(ifd_type, "StripOffsets", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0112: add_entry(ifd_type, "Orientation", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0115: add_entry(ifd_type, "SamplesPerPixel", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0116: add_entry(ifd_type, "RowsPerStrip", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0117: add_entry(ifd_type, "StripByteCounts", tiff_handle, ifd_entry_offset, entries); break;
	case 0x011a: add_entry(ifd_type, "XResolution", tiff_handle, ifd_entry_offset, entries); break;
	case 0x011b: add_entry(ifd_type, "YResolution", tiff_handle, ifd_entry_offset, entries); break;
	case 0x011c: add_entry(ifd_type, "PlanarConfiguration", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0128: add_entry(ifd_type, "ResolutionUnit", tiff_handle, ifd_entry_offset, entries); break;
	case 0x012d: add_entry(ifd_type, "TransferFunction", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0131: add_entry(ifd_type, "Software", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0132: add_entry(ifd_type, "DateTime", tiff_handle, ifd_entry_offset, entries); break;
	case 0x013b: add_entry(ifd_type, "Artist", tiff_handle, ifd_entry_offset, entries); break;
	case 0x013e: add_entry(ifd_type, "WhitePoint", tiff_handle, ifd_entry_offset, entries); break;
	case 0x013f: add_entry(ifd_type, "PrimaryChromaticities", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0201: add_entry(ifd_type, "JPEGInterchangeFormat", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0202: add_entry(ifd_type, "JPEGInterchangeFormatLength", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0211: add_entry(ifd_type, "YCbCrCoefficients", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0212: add_entry(ifd_type, "YCbCrSubSampling", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0213: add_entry(ifd_type, "YCbCrPositioning", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0214: add_entry(ifd_type, "ReferenceBlackWhite", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8298: add_entry(ifd_type, "Copyright", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8769: // EXIF tag
            forwarded_ifd_offset = get_ifd_offset(tiff_handle, ifd_entry_offset);
            if (ifd_type == IFD0_TIFF) {
#ifdef DEBUG
                cout << "exif_entry.parse_ifd_entries case8769 IFD0_tiff to IFD0_EXIF: ifd_entry_offset: " << ifd_entry_offset << " forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD0_EXIF, tiff_handle, forwarded_ifd_offset, entries);
            } else if (ifd_type == IFD1_TIFF) {
#ifdef DEBUG
                cout << "exif_entry.parse_ifd_entries case8769 IFD1_tiff to IFD1_EXIF: ifd_entry_offset:" << ifd_entry_offset << " forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
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
                cout << "exif_entry.parse_ifd_entries case8825 IFD0_tiff to IFD0_GPS: forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD0_GPS, tiff_handle, forwarded_ifd_offset, entries);
            } else if (ifd_type == IFD1_TIFF) {
#ifdef DEBUG
                cout << "exif_entry.parse_ifd_entries case8825 IFD1_tiff to IFD1_GPS: forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD1_GPS, tiff_handle, forwarded_ifd_offset, entries);
            } else {
		assert(0);
            }
            break;
	default:
            add_generic_entry(ifd_type, tiff_handle, ifd_entry_offset, entries);
            break;
        }
        break;
    case IFD0_EXIF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
    case IFD1_EXIF:	// see table Tag Support Levels(1) for 0th IFD TIFF Tags
        switch (entry_tag) {
	case 0x829a: add_entry(ifd_type, "ExposureTime", tiff_handle, ifd_entry_offset, entries); break;
	case 0x829d: add_entry(ifd_type, "FNumber", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8822: add_entry(ifd_type, "ExposureProgram", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8824: add_entry(ifd_type, "SpectralSensitivity", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8827: add_entry(ifd_type, "PhotographicSensitivity", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8828: add_entry(ifd_type, "OECF", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8830: add_entry(ifd_type, "SensitivityType", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8831: add_entry(ifd_type, "StandardOutputSensitivity", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8832: add_entry(ifd_type, "RecommendedExposureIndex", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8833: add_entry(ifd_type, "ISOSpeed", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8834: add_entry(ifd_type, "ISOSpeedLatitudeyyy", tiff_handle, ifd_entry_offset, entries); break;
	case 0x8835: add_entry(ifd_type, "IOSpeedLatitudezzz", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9000: add_entry(ifd_type, "ExifVersion", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9003: add_entry(ifd_type, "DateTimeOriginal", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9004: add_entry(ifd_type, "DateTimeDigitized", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9101: add_entry(ifd_type, "ComponentsConfiguration", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9102: add_entry(ifd_type, "CompressedBitsPerPixel", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9201: add_entry(ifd_type, "ShutterSpeedValue", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9202: add_entry(ifd_type, "ApertureValue", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9203: add_entry(ifd_type, "BrightnessValue", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9204: add_entry(ifd_type, "ExposureBiasValue", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9205: add_entry(ifd_type, "MaxApertureValue", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9206: add_entry(ifd_type, "SubjectDistance", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9207: add_entry(ifd_type, "MeteringMode", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9208: add_entry(ifd_type, "LightSource", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9209: add_entry(ifd_type, "Flash", tiff_handle, ifd_entry_offset, entries); break;
	case 0x920a: add_entry(ifd_type, "FocalLength", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9214: add_entry(ifd_type, "SubjectArea", tiff_handle, ifd_entry_offset, entries); break;
	case 0x927c: add_maker_note_entry(ifd_type, "MakerNote", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9286: add_entry(ifd_type, "UserComment", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9290: add_entry(ifd_type, "SubSecTime", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9291: add_entry(ifd_type, "SubSecTimeOriginal", tiff_handle, ifd_entry_offset, entries); break;
	case 0x9292: add_entry(ifd_type, "SubSecTimeDigitized", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa000: add_entry(ifd_type, "FlashpixVersion", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa001: add_entry(ifd_type, "ColorSpace", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa002: add_entry(ifd_type, "PixelXDimension", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa003: add_entry(ifd_type, "PixelYDimension", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa004: add_entry(ifd_type, "RelatedSoundFile", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa005: // Interoperability tag
            forwarded_ifd_offset = get_ifd_offset(tiff_handle, ifd_entry_offset);
            if (ifd_type == IFD0_EXIF) {
#ifdef DEBUG
                cout << "exif_entry.parse_ifd_entries casea005 IFD0_EXIF to IFD0_INTEROP: ifd_entry_offset: " << ifd_entry_offset << ", forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD0_INTEROPERABILITY, tiff_handle, forwarded_ifd_offset, entries);
            } else if (ifd_type == IFD1_EXIF) {
#ifdef DEBUG
                cout << "exif_entry.parse_ifd_entries casea005 IFD1_EXIF to IFD1_INTEROP: forwarded_ifd_offset " << forwarded_ifd_offset << "\n";
#endif
		entry_reader::parse_ifd_entries(IFD1_INTEROPERABILITY, tiff_handle, forwarded_ifd_offset, entries);
            } else {
		assert(0);
            }
            break;
	case 0xa20b: add_entry(ifd_type, "FlashEnergy", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa20c: add_entry(ifd_type, "SpatialFrequencyResponse", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa20e: add_entry(ifd_type, "FocalPlaneXResolution", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa20f: add_entry(ifd_type, "FocalPlaneYResolution", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa210: add_entry(ifd_type, "FocalPlaneResolutionUnit", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa214: add_entry(ifd_type, "SubjectLocation", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa215: add_entry(ifd_type, "ExposureIndex", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa217: add_entry(ifd_type, "SensingMethod", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa300: add_entry(ifd_type, "FileSource", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa301: add_entry(ifd_type, "SceneType", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa302: add_entry(ifd_type, "CFAPattern", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa401: add_entry(ifd_type, "CustomRendered", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa402: add_entry(ifd_type, "ExposureMode", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa403: add_entry(ifd_type, "WhiteBalance", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa404: add_entry(ifd_type, "DigitalZoomRatio", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa405: add_entry(ifd_type, "FocalLengthIn35mmFilm", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa406: add_entry(ifd_type, "SceneCaptureType", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa407: add_entry(ifd_type, "GainControl", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa408: add_entry(ifd_type, "Contrast", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa409: add_entry(ifd_type, "Saturation", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa40a: add_entry(ifd_type, "Sharpness", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa40b: add_entry(ifd_type, "DeviceSettingDescription", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa40c: add_entry(ifd_type, "SubjectDistanceRange", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa420: add_entry(ifd_type, "ImageUniqueID", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa430: add_entry(ifd_type, "CameraOwnerName", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa431: add_entry(ifd_type, "BodySerialNumber", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa432: add_entry(ifd_type, "LensSpecification", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa433: add_entry(ifd_type, "LensMake", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa434: add_entry(ifd_type, "LensModel", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa435: add_entry(ifd_type, "LensSerialNumber", tiff_handle, ifd_entry_offset, entries); break;
	case 0xa500: add_entry(ifd_type, "Gamma", tiff_handle, ifd_entry_offset, entries); break;
	default:
            add_generic_entry(ifd_type, tiff_handle, ifd_entry_offset, entries);
            break;
        }
        break;
    case IFD0_GPS:
    case IFD1_GPS:
        switch (entry_tag) {
	case 0x0000: add_entry(ifd_type, "GPSVersionID", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0001: add_entry(ifd_type, "GPSLatitudeRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0002: add_entry(ifd_type, "GPSLatitude", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0003: add_entry(ifd_type, "GPSLongitudeRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0004: add_entry(ifd_type, "GPSLongitude", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0005: add_entry(ifd_type, "GPSAltitudeRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0006: add_entry(ifd_type, "GPSAltitude", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0007: add_entry(ifd_type, "GPSTimeStamp", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0008: add_entry(ifd_type, "GPSSatellites", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0009: add_entry(ifd_type, "GPSStatus", tiff_handle, ifd_entry_offset, entries); break;
	case 0x000a: add_entry(ifd_type, "GPSMeasureMode", tiff_handle, ifd_entry_offset, entries); break;
	case 0x000b: add_entry(ifd_type, "GPSDOP", tiff_handle, ifd_entry_offset, entries); break;
	case 0x000c: add_entry(ifd_type, "GPSSpeedRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x000d: add_entry(ifd_type, "GPSSpeed", tiff_handle, ifd_entry_offset, entries); break;
	case 0x000e: add_entry(ifd_type, "GPSTrackRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x000f: add_entry(ifd_type, "GPSTrack", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0010: add_entry(ifd_type, "GPSImgDirectionRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0011: add_entry(ifd_type, "GPSImgDirection", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0012: add_entry(ifd_type, "GPSMapDatum", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0013: add_entry(ifd_type, "GPSDestLatitudeRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0014: add_entry(ifd_type, "GPSDestLatitude", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0015: add_entry(ifd_type, "GPSDestLongitudeRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0016: add_entry(ifd_type, "GPSDestLongitude", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0017: add_entry(ifd_type, "GPSDestBearingRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0018: add_entry(ifd_type, "GPSDestBearing", tiff_handle, ifd_entry_offset, entries); break;
	case 0x0019: add_entry(ifd_type, "GPSDestDistanceRef", tiff_handle, ifd_entry_offset, entries); break;
	case 0x001a: add_entry(ifd_type, "GPSDestDistance", tiff_handle, ifd_entry_offset, entries); break;
	case 0x001b: add_entry(ifd_type, "GPSProcessingMethod", tiff_handle, ifd_entry_offset, entries); break;
	case 0x001c: add_entry(ifd_type, "GPSAreaInformation", tiff_handle, ifd_entry_offset, entries); break;
	case 0x001d: add_entry(ifd_type, "GPSDateStamp", tiff_handle, ifd_entry_offset, entries); break;
	case 0x001e: add_entry(ifd_type, "GPSDifferential", tiff_handle, ifd_entry_offset, entries); break;
	case 0x001f: add_entry(ifd_type, "GPSHPositioningError", tiff_handle, ifd_entry_offset, entries); break;
	default:
            add_generic_entry(ifd_type, tiff_handle, ifd_entry_offset, entries);
            break;
        }
        break;
    case IFD0_INTEROPERABILITY:
    case IFD1_INTEROPERABILITY:
        switch (entry_tag) {
	case 0x0001: add_entry(ifd_type, "InteroperabilityIndex", tiff_handle, ifd_entry_offset, entries); break;
	default:
            // do nothing by default; the following catch-all was recording huge amounts of redundant data
            //add_generic_entry(ifd_type, tiff_handle, ifd_entry_offset, entries);
            break;
        }
        break;

    default:
        // not a valid type
        cout << "exif_entry.parse_entry invalid entry tag: " << entry_tag << "\n";
        assert(0);
    }
}

// entry value
static string get_value(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // entry type and entry count
    const uint16_t entry_type = get_entry_type(tiff_handle, ifd_entry_offset);

    // value
    std::string output;
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

static void add_entry(ifd_type_t ifd_type, const string &name,
                      tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset,
                      entry_list_t &entries) {

    // push the name and value onto entries
    string value = get_value(tiff_handle, ifd_entry_offset);
#ifdef DEBUG
    cout << "exif_entry.add_entry ifd type: '" << (int)ifd_type << "'\n";
    cout << "exif_entry.add_entry name: '" << name << "'\n";
    cout << "exif_entry.add_entry value: '" << value << "'\n";
#endif

    entries.push_back(new exif_entry(ifd_type, name, value));
}

static void add_generic_entry(ifd_type_t ifd_type,
			      tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset,
			      entry_list_t &entries) {

    if (opt_exif_suppress_unknown_entry_types) {
        // suppress the unrecognized entry
        return;
    }

    // create a generic name as "entry(N)"
    uint16_t entry_tag;
    try {
        entry_tag = tiff_handle.sbuf->get16u(ifd_entry_offset + 0, tiff_handle.byte_order);
    } catch (sbuf_t::range_exception_t &e) {
        entry_tag = 0;
    }
    stringstream ss;
    ss << "entry_0x";
    ss.width(4);
    ss.fill('0');
    ss << hex << entry_tag;
    string name = ss.str();

    // add the entry using this created name
    add_entry(ifd_type, name, tiff_handle, ifd_entry_offset, entries);
}

static void add_maker_note_entry(ifd_type_t ifd_type, const string &name,
                      tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset,
                      entry_list_t &entries) {

#ifdef DEBUG
    string value = get_value(tiff_handle, ifd_entry_offset);
    cout << "exif_entry.add_entry ifd type: '" << (int)ifd_type << "' (skipped)\n";
    cout << "exif_entry.add_entry name: '" << name << "' (skipped)\n";
    cout << "exif_entry.add_entry value: '" << value << "' (skipped)\n";
#endif
    // currently, we skip the MakerNote entry rather than adding it to entries, so no action.
}

inline static uint16_t get_entry_tag(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    try {
        return tiff_handle.sbuf->get16u(ifd_entry_offset + 0, tiff_handle.byte_order);
    } catch (sbuf_t::range_exception_t &e) {
        return 0;
    }
}

inline static uint16_t get_entry_type(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    try {
        return tiff_handle.sbuf->get16u(ifd_entry_offset + 2, tiff_handle.byte_order);
    } catch (sbuf_t::range_exception_t &e) {
        return 0;
    }
}

// Note: protocol requires that entry count < 64K
inline static uint32_t get_entry_count(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // get requested count
    uint32_t requested_count;
    try {
        requested_count = tiff_handle.sbuf->get32u(ifd_entry_offset + 4, tiff_handle.byte_order);
    } catch (sbuf_t::range_exception_t &e) {
        requested_count = 0;
    }
    return requested_count;
}

inline static uint16_t get_data_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // get entry type
    uint16_t entry_type;
    try {
        entry_type = tiff_handle.sbuf->get16u(ifd_entry_offset + 2, tiff_handle.byte_order);
    } catch (sbuf_t::range_exception_t &e) {
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
    } catch (sbuf_t::range_exception_t &e) {
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
        } catch (sbuf_t::range_exception_t &e) {
            // return an out-of-range value
            return ifd_entry_offset + 12;
        }
    }
}

// Note: protocol requires that ifd offset < 64K
inline static uint32_t get_ifd_offset(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    try {
        return tiff_handle.sbuf->get32u(ifd_entry_offset + 8, tiff_handle.byte_order);
    } catch (sbuf_t::range_exception_t &e) {
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

static string get_possible_utf8(const sbuf_t &sbuf, size_t count) {
    if (chars_match(sbuf, count) || char_pairs_match(sbuf, count)) {
        // this is an uninteresting sequence, so return ""
        return "";
    } else {
        std::string s;
        sbuf.getUTF8WithQuoting(0, count, s);
        return s;
    }
}

template <typename u16bit_iterator, typename octet_iterator>
octet_iterator all_utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result);
string all_utf16to8 (wstring utf16_string);

// read utf16 and return unvalidated utf8
string get_possible_utf16(const sbuf_t sbuf, size_t count, sbuf_t::byte_order_t byte_order) {

    // check for sequence
    if (chars_match(sbuf, count) || char_pairs_match(sbuf, count)) {
        // this is an uninteresting sequence, so return ""
        return "";
    }

    // get wstring accounting for byte order
    wstring wstr;
    sbuf.getUTF16(0, count, byte_order, wstr);

    // convert wstring to string
    string utf8_string = all_utf16to8(wstr);

#ifdef DEBUG
    cout << "exif_entry.get_possible_utf16 utf8_string (escaped): '" << validateOrEscapeUTF8(utf8_string, true, true) << "'\n";
#endif
    return utf8_string;
}

// read utf32 and return unvalidated utf8
string get_possible_utf32(const sbuf_t &sbuf, size_t count, sbuf_t::byte_order_t byte_order) {

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
            } catch (utf8::invalid_code_point) {

                // invalid code point so put in the byte values directly,
                // disregarding endian convention
                utf8_string += (uint8_t)code_point;
                utf8_string += (uint8_t)code_point/0x100;
                utf8_string += (uint8_t)code_point/0x10000;
                utf8_string += (uint8_t)code_point/0x1000000;
            }
        } catch (sbuf_t::range_exception_t &e) {
            // at end of buffer
            break;
        }
    }
    return utf8_string;
}

// convert all possible utf16, along with any illegal values
string all_utf16to8 (wstring utf16_string) {
    std::string utf8_string;
    std::back_insert_iterator<std::basic_string<char> > result = back_inserter(utf8_string);

    wstring::const_iterator start = utf16_string.begin();
    wstring::const_iterator end = utf16_string.end();

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
        } catch (utf8::invalid_code_point) {
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
static string get_exif_byte(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
 
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count; // exif standard: 1 exif byte is 1 byte long
    if (count >= 0x10000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    if (count == 1) {
        // count is 1 so print the byte as an integer
        stringstream ss;
        try {
	    ss << (int)tiff_handle.sbuf->get8u(offset);
        } catch (sbuf_t::range_exception_t &e) {
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
static string get_exif_ascii(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
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
    } catch (sbuf_t::range_exception_t &e) {
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
static string get_exif_short(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each short separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 2; // exif standard: 1 exif short is 2 bytes long
    if (count >= 0x8000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    if (count == 1) {
        // count is 1 so print the uint16 as an integer
        stringstream ss;
        try {
	    ss << (int)tiff_handle.sbuf->get16u(offset, tiff_handle.byte_order);
        } catch (sbuf_t::range_exception_t &e) {
            // add nothing to ss
        }
        return ss.str();

    } else {
        // count is not 1 so print the uint16_t bytes as utf8
        std::string s = get_possible_utf16(*(tiff_handle.sbuf)+offset, count, tiff_handle.byte_order);
#ifdef DEBUG
        cout << "exif_entry.get_exif_short (escaped): '" << validateOrEscapeUTF8(s, true, true) << "'\n";
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
static string get_exif_long(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {

    // print each long separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 4; // exif standard: 1 exif long is 4 bytes long
    if (count >= 0x4000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    if (count == 1) {
        // count is 1 so print the long directly
        stringstream ss;
        try {
	    ss << (uint32_t)tiff_handle.sbuf->get32u(offset);
        } catch (sbuf_t::range_exception_t &e) {
            // add nothing to ss
        }
        return ss.str();

    } else {
        // count is not 1 so print the uint32_t bytes as utf8
        std::string s = get_possible_utf32(*(tiff_handle.sbuf)+offset, count, tiff_handle.byte_order);
#ifdef DEBUG
        cout << "exif_entry.get_exif_short (escaped): '" << validateOrEscapeUTF8(s, true, true) << "'\n";
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
static string get_exif_rational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each rational as 1'st uint32, "/", 2'nd uint32, separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 8; // exif standard: 1 exif rational is 8 bytes long
    if (count >= 0x2000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    stringstream ss;
    for (uint32_t i=0; i<count; i++) {
        try {
	    // return 1'st uint32, "/", 2'nd uint32
            ss << tiff_handle.sbuf->get32u(offset + i * 8, tiff_handle.byte_order) << "/"
               << tiff_handle.sbuf->get32u(offset + i * 8 + 4, tiff_handle.byte_order);
	    if (i + 1 < count) {
	        ss << " ";
	    }
        } catch (sbuf_t::range_exception_t &e) {
            break;
        }
    }
    return ss.str();
}

// EXIF_UNDEFINED byte whose value depends on the field definition
static string get_exif_undefined(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print UNDEFINED as byte
    return get_exif_byte(tiff_handle, ifd_entry_offset);
}

// EXIF_SLONG int32
/**
 * Throws exif_failure_exception if the exif data state is determined to be invalid
 * such that further entry parsing would be invalid.
 * @Throws exif_failure_exception_t
 */
static  string get_exif_slong(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each signed long
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 4; // exif standard: 1 exif slong is 4 bytes long
    if (count >= 0x4000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    stringstream ss;
    for (uint32_t i=0; i<count; i++) {
        try {
	    ss << tiff_handle.sbuf->get32i(offset + i * 4, tiff_handle.byte_order);
        } catch (sbuf_t::range_exception_t &e) {
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
static string get_exif_srational(tiff_handle_t &tiff_handle, uint32_t ifd_entry_offset) {
    // print each rational as 1'st uint32, "/", 2'nd uint32, separated by a space
    uint32_t count = get_entry_count(tiff_handle, ifd_entry_offset);
    uint32_t offset = get_data_offset(tiff_handle, ifd_entry_offset);

    // Abort on count overflow
    tiff_handle.bytes_read += count * 8; // exif standard: 1 exif srational is 8 bytes long
    if (count >= 0x2000 || tiff_handle.bytes_read >= 0x10000) throw exif_failure_exception_t();

    stringstream ss;
    for (uint32_t i=0; i<count; i++) {
        try {
	    // return 1'st int32, "/", 2'nd int32
            ss << tiff_handle.sbuf->get32i(offset + i * 8, tiff_handle.byte_order) << "/"
               << tiff_handle.sbuf->get32i(offset + i * 8 + 4, tiff_handle.byte_order);
	    if (i + 1 < count) {
	        ss << " ";
	    }
        } catch (sbuf_t::range_exception_t &e) {
            break;
        }
    }
    return ss.str();
}

