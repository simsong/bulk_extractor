/**
 * exif_reader: - custom exif reader for identifying specific features for bulk_extractor.
 * Revision history:
 * 2012-jan-05 bda - Created.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>
#include "config.h"

#include "exif_reader.h"

// #define DEBUG 1
// ************************************************************
// exif_reader
// ************************************************************
namespace be_exif {
    const char* version() {
        return "0.01";
    }
}

namespace exif_reader {
    /** 
     * get_tiff_offset_from_exif() returns the offset to the TIFF data from the EXIF header,
     *                             or 0 if exif is invalid.
     */
    size_t get_tiff_offset_from_exif(const sbuf_t &exif_sbuf) {
        // validate the EXIF header
        if (exif_sbuf.pagesize < 4
         || exif_sbuf[0]!=0xff
         || exif_sbuf[1]!=0xd8
         || exif_sbuf[2]!=0xff) {
            return 0;
        }

        // find APP1 jpeg marker 0xe1, skipping other 0xffeX markers
        size_t next_offset = 2;
        while (true) {
//cout << "exif_reader.get_tiff_offset_from_exif next_offset: " << next_offset << "\n";
//cout << "exif_reader.get_tiff_offset_from_exif seeking, next 4: "
//                << (int)exif_sbuf[next_offset] << " " << (int)exif_sbuf[next_offset+1] << " " 
//                << (int)exif_sbuf[next_offset+ 2] << " " << (int)exif_sbuf[next_offset+3] << "\n";
            // abort if EOF
            if (exif_sbuf.pagesize < next_offset + 2 + 2 + 6) {
                return 0;
            }

            // abort if next marker is not an application-specific marker,
            // specifically, not 0xffeX
            if (exif_sbuf[next_offset] != 0xff
             || ((exif_sbuf[next_offset + 1] & 0xf0) != 0xe0)) {
                return 0;
            }

            // found the APP1 0xffe1 EXIF marker that we are looking for
            // Note that images can contain multiple FFE1 markers where the first is not Exif,
            // so we are very particular here.
            if (exif_sbuf[next_offset] == 0xff
             && exif_sbuf[next_offset + 1] == 0xe1
             // additionally validate APP1 as Exif since an image was found with a non-Exif FFE1
             && exif_sbuf[next_offset + 4] == 'E'
             && exif_sbuf[next_offset + 5] == 'x'
             && exif_sbuf[next_offset + 6] == 'i'
             && exif_sbuf[next_offset + 7] == 'f'
             && exif_sbuf[next_offset + 8] == '\0'
             && exif_sbuf[next_offset + 9] == '\0') {
                break;
            }

            // try the next marker at the next offset
            next_offset += (exif_sbuf[next_offset + 2]<<8 | exif_sbuf[next_offset + 3]) + 2;
        }

        //cout << "exif_reader.get_tiff_offset_from_exif: APP1 found at " << next_offset << "\n";

        // APP1 marker is after next header
        size_t app1_offset = next_offset + 4;

        // return the tiff offset, which is 6 bytes into the APP1 marker
        return app1_offset + 6;  // tiff offset is just past "Exif\0\0" in APP1
    }
}

// public accessors for a TIFF entry
namespace tiff_reader {
    /** 
     * read_tiff_data() extracts specific hardcoded entry values, where present, from exif.
     */
    void read_tiff_data(const sbuf_t &tiff_sbuf, entry_list_t &entries) {
        if (tiff_sbuf.bufsize < 4) {
            return;
        }
        sbuf_t::byte_order_t byte_order = get_tiff_byte_order(tiff_sbuf);

        // establish the tiff handle for working with this tiff
        tiff_handle_t tiff_handle(&tiff_sbuf, byte_order);

        // find ifd0
        uint32_t ifd0_offset = tiff_ifd_reader::get_first_ifd_offset(tiff_handle);
#ifdef DEBUG
        cout << "exif_reader read_tiff_data ifd0_offset is " << ifd0_offset << "\n";
#endif
        if (ifd0_offset == 0) {
            return;
        }

        // parse ifd0
        entry_reader::parse_ifd_entries(IFD0_TIFF, tiff_handle, ifd0_offset, entries);

        // find ifd1
        uint32_t ifd1_offset = tiff_ifd_reader::get_next_ifd_offset(tiff_handle, ifd0_offset);
#ifdef DEBUG
        cout << "exif_reader read_tiff_data ifd1_offset is " << ifd0_offset << "\n";
#endif
        if (ifd1_offset == 0) {
            return;
        }

        // parse ifd1
        entry_reader::parse_ifd_entries(IFD1_TIFF, tiff_handle, ifd1_offset, entries);
    }

    bool is_maybe_valid_tiff(const sbuf_t &tiff_sbuf) {
        // check endian and magic number
        if (tiff_sbuf.bufsize < 4 || !(
          // Intel
            (tiff_sbuf[0] == 'I'
          && tiff_sbuf[1] == 'I'
          && tiff_sbuf[2] == 42
          && tiff_sbuf[3] == 0)
          // Motorola
         || (tiff_sbuf[0] == 'M'
          && tiff_sbuf[1] == 'M'
          && tiff_sbuf[2] == 0
          && tiff_sbuf[3] == 42))) {
            // not a match
#ifdef DEBUG
            cout << "exif_reader is_maybe_valid_tiff false, invalid endian indicator\n";
#endif
            return false;
        }

        // perform IFD entry inspection to improve confidence and reduce false positives
        // establish a tiff handle for working with this tiff
        sbuf_t::byte_order_t byte_order = get_tiff_byte_order(tiff_sbuf);
        tiff_handle_t tiff_handle(&tiff_sbuf, byte_order);

        // improve confidence by inspecting the first IFD entry
        uint32_t ifd0_offset = tiff_ifd_reader::get_first_ifd_offset(tiff_handle);
        if (ifd0_offset == 0) {
#ifdef DEBUG
            cout << "exif_reader is_maybe_valid_tiff false, invalid endian indicator\n";
#endif
            return false;
        }

        // it is reasonable for the number of fields in the first entry is between 1 and 200
        uint32_t num_entries = tiff_ifd_reader::get_num_entries(tiff_handle, ifd0_offset);
        if (num_entries == 0 || num_entries > 200) {
#ifdef DEBUG
            cout << "exif_reader is_maybe_valid_tiff false, too many entries\n";
#endif
            return false;
        }

        // check that the field type of the first entry is in range
        // use TIFF standard which goes to 12 rather than exif standard which goes to 10
        uint32_t ifd_entry_0_offset = tiff_ifd_reader::get_entry_offset(ifd0_offset, 0);

        // validate that there is space for reading the whole ifd_entry_0
        if (ifd_entry_0_offset + 12 > tiff_sbuf.bufsize) {
#ifdef DEBUG
            cout << "exif_reader is_maybe_valid_tiff false, end of buffer\n";
#endif
            return false;
        }

        // check that the field type is in range and has a reasonable value
        uint16_t field_type;
        try {
            field_type = tiff_handle.sbuf->get16u(ifd_entry_0_offset + 2, tiff_handle.byte_order);
        } catch (sbuf_t::range_exception_t &e) {
#ifdef DEBUG
            cout << "exif_reader is_maybe_valid_tiff false, field type outside of buffer\n";
#endif
            return false;
        }
        if (field_type == 0 || field_type > 12) {
#ifdef DEBUG
            cout << "exif_reader is_maybe_valid_tiff false, field type value " << field_type << " is invalid\n";
#endif
            return false;
        }

        // the pointer may be at a valid TIFF
#ifdef DEBUG
        cout << "exif_reader is_maybe_vaid_tiff is true\n";
#endif
        return true;
    }

    sbuf_t::byte_order_t get_tiff_byte_order(const sbuf_t &tiff_sbuf) {
        if (
          // Intel
             tiff_sbuf[0] == 'I'
          && tiff_sbuf[1] == 'I'
          && tiff_sbuf[2] == 42
          && tiff_sbuf[3] == 0) {
            // probably a match
            return sbuf_t::BO_LITTLE_ENDIAN;
        } else if (
          // Motorola
             tiff_sbuf[0] == 'M'
          && tiff_sbuf[1] == 'M'
          && tiff_sbuf[2] == 0
          && tiff_sbuf[3] == 42) {
            // probably a match
            return sbuf_t::BO_BIG_ENDIAN;
        } else {
            // don't call get_tiff_byte_order if TIFF is clearly invalid, callee is required to know better
            assert(0);
            // pick one because there is no empty set
            return sbuf_t::BO_LITTLE_ENDIAN;
        }
    }
}
 
// ************************************************************
// IFD reader
// ************************************************************
namespace tiff_ifd_reader {
    uint16_t get_num_entries(const tiff_handle_t &tiff_handle, size_t ifd_offset) {
        try {
            return tiff_handle.sbuf->get16u(ifd_offset + 0, tiff_handle.byte_order);
        } catch (sbuf_t::range_exception_t &e) {
            return 0;
        }
    }

    // entry from IFD's entry index
    size_t get_entry_offset(size_t ifd_offset, uint16_t entry_index) {
        return ifd_offset + 2 + (entry_index * 12);
    }

    size_t get_first_ifd_offset(const tiff_handle_t &tiff_handle) {
        try {
            return tiff_handle.sbuf->get32u(4, tiff_handle.byte_order);
        } catch (sbuf_t::range_exception_t &e) {
            return 0;
        }
    }

    size_t get_next_ifd_offset(const tiff_handle_t &tiff_handle, size_t this_ifd_offset) {
        // the pointer to the next ifd is at the end of this ifd
        // an IFD consists of num_entries, the 12-byte entries, then a 4 byte pointer to the next IFD
        if (this_ifd_offset == 0) {
            // ifd offset is out of range
            return 0;
        }
        uint16_t num_entries = get_num_entries(tiff_handle, this_ifd_offset);
        uint32_t next_ifd_offset_pointer = this_ifd_offset + 2 + (num_entries * 12);
        try {
            return tiff_handle.sbuf->get32u(next_ifd_offset_pointer, tiff_handle.byte_order);
        } catch (sbuf_t::range_exception_t &e) {
            return 0;
        }
    }
}

