#ifndef SCAN_EXIF_H
#define SCAN_EXIF_H

#include "exif_reader.h"
#include "be13_api/sbuf.h"
#include "be13_api/scanner_params.h"
#include "jpeg_validator.h"

extern void record_exif_data(feature_recorder *exif_recorder, const pos0_t &pos0,
                             const std::string &hash_hex, const entry_list_t &entries);

extern void record_gps_data(feature_recorder *gps_recorder, const pos0_t &pos0,
                            const std::string &hash_hex, const entry_list_t &entries);

struct exif_scanner {
    bool exif_scanner_debug {false};

    exif_scanner(const scanner_params &sp):
        ss(sp.ss),
        exif_recorder(sp.ss.named_feature_recorder("exif")),
        gps_recorder(sp.ss.named_feature_recorder("gps")),
        jpeg_recorder(sp.ss.named_feature_recorder("jpeg_carved")) {
    }

    static void clear_entries(entry_list_t &entries) {
        for (entry_list_t::const_iterator it = entries.begin(); it!=entries.end(); it++) {
            delete *it;
        }
        entries.clear();
    }

    entry_list_t entries {};
    scanner_set &ss;
    feature_recorder &exif_recorder;
    feature_recorder &gps_recorder;
    feature_recorder &jpeg_recorder;

    /* Verify a jpeg internal structure and return the length of the validated portion */
    // http://www.w3.org/Graphics/JPEG/itu-t81.pdf
    // http://stackoverflow.com/questions/1557071/the-size-of-a-jpegjfif-image

    /**
     * Process the JPEG, including - calculate its hash, carve it, record exif and gps data
     * Return the size of the object carved, or 0 if unknown
     */
    size_t process(const sbuf_t &sbuf,bool found_start);
    void scan(const sbuf_t &sbuf);
};

#endif
