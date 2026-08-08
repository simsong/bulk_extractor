/**
 * Carves 8-bit RawTherapee thumbnail images.
 *
 * RTTI Image8 records contain "Image8\n", little-endian width and height
 * values, and contiguous RGB bytes. A PPM header makes the pixels directly
 * viewable without copying or transforming the image data.
 */

#include "config.h"

#include <cstdint>
#include <memory>
#include <string>

#include "be20_api/scanner_params.h"

namespace {

constexpr char IMAGE8_SIGNATURE[] = "Image8\n";
constexpr size_t SIGNATURE_SIZE = sizeof(IMAGE8_SIGNATURE) - 1;
constexpr size_t IMAGE8_HEADER_SIZE = SIGNATURE_SIZE + sizeof(uint32_t) * 2;
constexpr size_t RGB_CHANNELS = 3;
constexpr size_t MAX_PIXEL_BYTES = 16 * 1024 * 1024;

void carve_image8(const sbuf_t &sbuf, feature_recorder &recorder)
{
    size_t search_offset = 0;
    while (search_offset < sbuf.pagesize) {
        const ssize_t found = sbuf.find(IMAGE8_SIGNATURE, search_offset);
        if (found < 0 || static_cast<size_t>(found) >= sbuf.pagesize) {
            return;
        }

        const size_t start = static_cast<size_t>(found);
        search_offset = start + SIGNATURE_SIZE;
        if (sbuf.left(start) < IMAGE8_HEADER_SIZE) {
            return;
        }

        const uint32_t width = sbuf.get32u(start + SIGNATURE_SIZE);
        const uint32_t height = sbuf.get32u(start + SIGNATURE_SIZE + sizeof(width));
        if (width == 0 || height == 0 ||
            height > MAX_PIXEL_BYTES / RGB_CHANNELS / width) {
            continue;
        }

        const size_t pixel_bytes = static_cast<size_t>(width) * height * RGB_CHANNELS;
        const size_t pixel_offset = start + IMAGE8_HEADER_SIZE;
        if (pixel_bytes > sbuf.left(pixel_offset)) {
            continue;
        }

        const std::string ppm_header = "P6\n" + std::to_string(width) + " " +
                                       std::to_string(height) + "\n255\n";
        std::unique_ptr<sbuf_t> header(sbuf_t::sbuf_malloc(pos0_t(), ppm_header));
        std::unique_ptr<sbuf_t> pixels(
            sbuf.new_slice(sbuf.pos0 + start, pixel_offset, pixel_bytes));
        recorder.carve(*header, *pixels, ".ppm");
        search_offset = pixel_offset + pixel_bytes;
    }
}

} // namespace

extern "C"
void scan_rtti(scanner_params &sp)
{
    sp.check_version();
    if (sp.phase == scanner_params::PHASE_INIT) {
        sp.info->set_name("rtti");
        sp.info->author = "Simson Garfinkel and Rannek";
        sp.info->description = "Carves 8-bit RawTherapee thumbnail images";
        sp.info->scanner_version = "1.0";
        sp.info->min_sbuf_size = IMAGE8_HEADER_SIZE;

        feature_recorder_def::flags_t flags;
        flags.carve = true;
        sp.info->feature_defs.emplace_back("rtti", flags);
        return;
    }
    if (sp.phase == scanner_params::PHASE_SCAN) {
        carve_image8(*sp.sbuf, sp.named_feature_recorder("rtti"));
    }
}
