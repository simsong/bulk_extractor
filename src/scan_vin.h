/**
 * scan_vin.h:
 * Header file for VIN (Vehicle Identification Number) scanner
 */

#ifndef SCAN_VIN_H
#define SCAN_VIN_H

#include <cstddef>
#include "be20_api/sbuf.h"

// Debug flag
extern int scan_vin_debug;

/**
 * Main VIN validation function
 * @param buf Buffer containing potential VIN
 * @param buflen Length of buffer (must be 17 for valid VIN)
 * @return true if valid VIN, false otherwise
 */
bool valid_vin(const char *buf, int buflen);

// Internal validation functions (exposed for testing)
#ifdef DEBUG
static int validate_vin_debug(const char *buf, int buflen);
#endif

#endif // SCAN_VIN_H
