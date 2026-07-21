/**
 * scan_exiv2: extract EXIF metadata using the optional Exiv2 library.
 */

#include "config.h"

#include <algorithm>
#include <string>

#include "be20_api/scanner_params.h"
#include "dfxml_cpp/src/dfxml_writer.h"

#ifdef HAVE_EXIV2
#include <exiv2/error.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>

namespace {
constexpr size_t MIN_EXIF_SIZE = 4096;
constexpr size_t EXIF_GULP_SIZE = 1024 * 1024;

bool jpeg_start(const sbuf_t& sbuf, size_t pos)
{
    return sbuf.left(pos) >= 3 && sbuf[pos] == 0xff && sbuf[pos + 1] == 0xd8 && sbuf[pos + 2] == 0xff;
}

std::string exif_xml(const Exiv2::Image& image)
{
    std::string xml = "<exiv2><width>" + std::to_string(image.pixelWidth()) + "</width><height>"
                    + std::to_string(image.pixelHeight()) + "</height>";
    for (const auto& entry : image.exifData()) {
        if (entry.count() <= 1000) {
            const std::string key(entry.key());
            xml += "<" + key + ">" + dfxml_writer::xmlescape(entry.value().toString()) + "</" + key + ">";
        }
    }
    return xml + "</exiv2>";
}
} // namespace

extern "C" void scan_exiv2(scanner_params& sp)
{
    sp.check_version();
    if (sp.phase == scanner_params::PHASE_INIT) {
        sp.info->set_name("exiv2");
        sp.info->author = "Simson L. Garfinkel";
        sp.info->description = "Searches for EXIF information using Exiv2.";
        sp.info->scanner_flags.default_enabled = false;
        sp.info->scanner_version = "2.0";
        feature_recorder_def::flags_t xml_flag;
        xml_flag.xml = true;
        sp.info->feature_defs.emplace_back("exiv2", xml_flag);
        return;
    }
    if (sp.phase != scanner_params::PHASE_SCAN || sp.sbuf->bufsize < MIN_EXIF_SIZE) {
        return;
    }

    const sbuf_t& sbuf = *sp.sbuf;
    feature_recorder& recorder = sp.named_feature_recorder("exiv2");
    const size_t pos_max = sbuf.bufsize - MIN_EXIF_SIZE;
    for (size_t pos = 0; pos <= pos_max; pos++) {
        if (pos % 512 != 0 && !jpeg_start(sbuf, pos)) {
            continue;
        }
        try {
            const size_t count = std::min(EXIF_GULP_SIZE, sbuf.left(pos));
            auto image = Exiv2::ImageFactory::open(sbuf.get_buf() + pos, count);
            image->readMetadata();
            if (!image->exifData().empty()) {
                const auto to_hash = sbuf.slice(pos, std::min<size_t>(4096, count));
                recorder.write(sbuf.pos0 + pos, recorder.hash(to_hash), exif_xml(*image));
            }
        } catch (const std::exception&) {
        }
    }
}
#endif
