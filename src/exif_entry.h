#ifndef EXIF_ENTRY_H
#define EXIF_ENTRY_H

#include <vector>
#include <memory>
#include <cstdint>


/**
 * EXIF entry
 */
class exif_entry {
    const exif_entry &operator=(const exif_entry &that);
public:
    const uint16_t ifd_type {};
    const std::string name {};
    const std::string value {};
    exif_entry(uint16_t ifd_type_, const std::string &name_, const std::string &value_);
    exif_entry(const exif_entry &that);     // copy
    exif_entry(const exif_entry &&that);     // move
    ~exif_entry();
    const std::string get_full_name() const;
};
typedef std::vector< std::unique_ptr<exif_entry >> entry_list_t;

#endif
