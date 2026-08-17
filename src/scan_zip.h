#ifndef SCAN_ZIP_H
#define SCAN_ZIP_H

#include <cstdint>
#include <string>

class pos0_t;

std::string zip_carve_filename(const std::string &name);
uint32_t zip_depth(const pos0_t &pos0);

#endif
