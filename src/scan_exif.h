#ifndef SCAN_EXIF_H
#define SCAN_EXIF_H

#include "be20_api/sbuf.h"
#include "be20_api/scanner_params.h"
#include "be20_api/scanner_set.h"
#include "exif_reader.h"                // provides entry_list_t
#include "jpeg_validator.h"

struct exif_scanner {
    bool exif_scanner_debug {false};
    exif_scanner(const exif_scanner&) = delete;
    exif_scanner & operator=(const exif_scanner &) = delete;

    exif_scanner(const scanner_params &sp):
        ss(sp.ss),
        exif_recorder(sp.named_feature_recorder("exif")),
        gps_recorder(sp.named_feature_recorder("gps")),
        jpeg_recorder(sp.named_feature_recorder("jpeg")) {
    }

    entry_list_t entries {};
    scanner_set *ss;            //  for the hashing function
    feature_recorder &exif_recorder;
    feature_recorder &gps_recorder;
    feature_recorder &jpeg_recorder;

    /* Verify a jpeg internal structure and return the length of the validated portion */
    // http://www.w3.org/Graphics/JPEG/itu-t81.pdf
    // http://stackoverflow.com/questions/1557071/the-size-of-a-jpegjfif-image

    void record_exif_data(const pos0_t &pos0, std::string hash_hex);

    void record_gps_data(const pos0_t &pos0, std::string hash_hex);

    /**
     * Process the JPEG, including - calculate its hash, carve it, record exif and gps data
     * Return the size of the object carved, or 0 if unknown
     */
    size_t process_possible_jpeg(const sbuf_t &sbuf,bool found_start);
    void   scan(const sbuf_t &sbuf);    // scan and possibly carve
};

#endif
