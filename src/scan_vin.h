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

/**
 * VIN validation with sbuf_t position information
 * @param sbuf Buffer structure
 * @param pos Starting position in buffer
 * @param len Length to check (must be 17)
 * @return true if valid VIN, false otherwise
 */
bool valid_vin_sbuf(const sbuf_t &sbuf, size_t pos, size_t len);

// Internal validation functions (exposed for testing)
#ifdef DEBUG
static int validate_vin_debug(const char *buf, int buflen);
#endif

#endif // SCAN_VIN_H
