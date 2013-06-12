/**
 * exif_reader: - custom exif reader for identifying specific features for bulk_extractor.
 * Revision history:
 * 2012-jan-05 bda - Created.
 *
 * TESTING:
 *  1) Validate that entries may be read from both Intel and Motorola file types.
 *  2) Validate that entries are gleaned from all (or required) IFD types, see ifd_type_t.
 *  3) Validate syntax of each entry name: each custom name and the generic name format.
 */

#ifndef EXIF_READER_H
#define EXIF_READER_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>

#include "sbuf.h"

/**
 * Handle to the beginning of the tiff structure.
 */
struct tiff_handle_t {
    const sbuf_t *sbuf;
    const sbuf_t::byte_order_t byte_order;
    uint32_t bytes_read;
    tiff_handle_t(const sbuf_t *sbuf_, sbuf_t::byte_order_t byte_order_)
                 :sbuf(sbuf_), byte_order(byte_order_), bytes_read(0) {
    }
};

//struct tiff_handle_t {
//    const uint8_t *buf;
//    const size_t bufsize;
//    const bool intel_byte_order;
//    tiff_handle_t(const uint8_t *buf_, size_t bufsize_, bool intel_byte_order_)
//                :buf(buf_), bufsize(bufsize_), intel_byte_order(intel_byte_order_) {
//    }
//};

/**
 * types of IFDs
 * NOTE: some manufacturers treat Maker Notes as an IFD, but not all.
 * We currently report maker not contents as one entry
 * rather than individually per manufacturer.
 */
typedef enum {
    IFD0_TIFF,
    IFD0_EXIF,
    IFD0_GPS,
    IFD0_INTEROPERABILITY,
    IFD1_TIFF,
    IFD1_EXIF,
    IFD1_GPS,
    IFD1_INTEROPERABILITY
} ifd_type_t;

/**
 * EXIF entry
 */
class exif_entry {
public:
    const uint16_t ifd_type;
    const std::string name;
    const std::string value;
    exif_entry(uint16_t ifd_type_, const std::string &name_, const std::string &value_);

    // copy
    exif_entry(const exif_entry &that);
    ~exif_entry();

    // prefix name with IFD type
    const std::string get_full_name() const;

private:
    // do not implement this operator
    const exif_entry &operator=(const exif_entry &that);
};

/**
 * EXIF entries
 */
typedef std::vector<exif_entry*> entry_list_t;

/**
 * Raise an exif_failure_exception to discontinue processing exif when it is determined that
 * the current and any further exif processing will be invalid.
 */
class exif_failure_exception_t: public std::exception {
public:
    virtual const char *what() const throw() {
        return "Error: exif failure";
    }
};

/**
 * Version, which encompasses Exif and PSD headers and the underlying TIFF tag code
 */
namespace exif {
  const char* version();
}

/**
 * Exif reader
 */
namespace exif_reader {
    size_t get_tiff_offset_from_exif(const sbuf_t &exif_sbuf);
}

/**
 * PSD Photoshop reader
 */
namespace psd_reader {
    size_t get_tiff_offset_from_psd(const sbuf_t &exif_sbuf);
}

/**
 * low-level TIFF reader.
 */
namespace tiff_reader {
    void read_tiff_data(const sbuf_t &tiff_sbuf, entry_list_t &entries);
    bool is_maybe_valid_tiff(const sbuf_t &tiff_sbuf);
    sbuf_t::byte_order_t get_tiff_byte_order(const sbuf_t &tiff_sbuf);
}

/**
 * IFD reader
 */
namespace tiff_ifd_reader {
    // these obtain information from IFDs
    uint16_t get_num_entries(const tiff_handle_t &tiff_handle, size_t ifd_offset);
    size_t get_entry_offset(size_t ifd_offset, uint16_t entry_index);
    size_t get_first_ifd_offset(const tiff_handle_t &tiff_handle);
    size_t get_next_ifd_offset(const tiff_handle_t &tiff_handle, size_t this_ifd_offset);
}

/**
 * entry reader
 */
namespace entry_reader {
    /** 
     * parse_ifd_entries() extracts entries from an offset given its type
     */
    void parse_ifd_entries(ifd_type_t ifd_type, tiff_handle_t &tiff_handle,
                       size_t ifd_offset, entry_list_t &entries);

    /** 
     * parse_entry() extracts entry and if entry add entry else IFD so parse its entries
     */
    void parse_entry(ifd_type_t ifd_type, tiff_handle_t &tiff_handle,
                       size_t ifd_entry_offset, entry_list_t &entries);
}

/**
 * Exif reader.
 */
namespace exif_reader {

  /** 
   * is_valid_exif() validates that sbuf is at valid exif structure
   */
  bool is_valid_exif(const sbuf_t &exif_sbuf);

  /** 
   * exif_reader() extracts specific hardcoded entry values, where present, into list.
   */
  void read_exif_data(const sbuf_t &exif_sbuf, entry_list_t &entries);
}

#endif

