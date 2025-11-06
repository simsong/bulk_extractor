/**
 * scan_vin:
 * Scanner for validating Vehicle Identification Numbers (VINs)
 * Used by the scan_vehicle.flex system
 */

#include <cassert>
#include <cstring>
#include <cctype>

#include "config.h"
#include "scan_vin.h"

#include "be20_api/utils.h"
#include "dfxml_cpp/src/hash_t.h"
#include "be20_api/scanner_params.h"

int scan_vin_debug = 0;

/**
 * VIN (Vehicle Identification Number) validator
 * A valid VIN has 17 characters and follows specific rules:
 * - Letters I, O, or Q are not allowed
 * - Letters U and Z cannot appear in the date code (10th char)
 * - Character 9 is a check digit
 * - Specific format for different positions
 */

// Valid VIN characters (excludes I, O, Q)
static const char* valid_vin_chars = "0123456789ABCDEFGHJKLMNPRSTUVWXYZ";

// Transliteration values for VIN check digit calculation
static int char_to_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'H') return (c - 'A') + 1;
    if (c == 'J') return 1;
    if (c >= 'K' && c <= 'N') return (c - 'K') + 2;
    if (c == 'P') return 7;
    if (c == 'R') return 9;
    if (c >= 'S' && c <= 'Z') return (c - 'S') + 2;
    return -1;  // Invalid character
}

// Weight factors for VIN check digit calculation
static const int vin_weights[] = {8, 7, 6, 5, 4, 3, 2, 10, 0, 9, 8, 7, 6, 5, 4, 3, 2};

/**
 * Extract and validate VIN characters
 * Returns 0 if successful, -1 if invalid
 */
static int extract_vin_chars(const char *buf, int len, char *vin) {
    if (len != 17) return -1;  // VINs must be exactly 17 characters
    
    int pos = 0;
    for (int i = 0; i < len && buf[i]; i++) {
        char original = buf[i];
        char c = toupper(original);
        
        // Reject if lowercase letter found - real VINs are always uppercase
        if (original != c && isalpha(original)) {
            return -1;
        }
        
        // Check if character is valid for VIN
        if (strchr(valid_vin_chars, c) == NULL) {
            return -1;  // Invalid character found
        }
        
        vin[pos++] = c;
    }
    vin[pos] = '\0';
    
    return (pos == 17) ? 0 : -1;
}

/**
 * Validate VIN check digit (position 9, index 8)
 * Uses weighted calculation similar to credit card Luhn algorithm
 */
static int check_digit_test(const char *vin) {
    int sum = 0;
    
    for (int i = 0; i < 17; i++) {
        if (i == 8) continue;  // Skip check digit position
        
        int value = char_to_value(vin[i]);
        if (value == -1) return -1;  // Invalid character
        
        sum += value * vin_weights[i];
    }
    
    int check_digit = sum % 11;
    char expected_check;
    
    if (check_digit == 10) {
        expected_check = 'X';
    } else {
        expected_check = '0' + check_digit;
    }
    
    return (vin[8] == expected_check) ? 0 : -1;
}

/**
 * World Manufacturer Identifier (WMI) validation
 * First 3 characters identify the manufacturer
 */
static int wmi_test(const char *vin) {
    // Common WMI prefixes (first character indicates region)
    char first = vin[0];
    
    // 1-5: North America
    // 6-7: Oceania
    // 8-9: South America
    // A-H: Africa
    // J-R: Asia
    // S-Z: Europe
    
    if (first == '0') return -1;  // 0 is not used as first character
    
    // Additional validation could check against known manufacturer codes
    // For now, we just ensure it's in valid range
    return 0;
}

/**
 * Vehicle Descriptor Section (VDS) validation
 * Characters 4-9, with 9 being check digit
 */
static int vds_test(const char *vin) {
    // Characters 4-8 describe vehicle attributes
    // Character 9 is check digit (already validated)
    
    // Basic validation - ensure no invalid patterns
    for (int i = 3; i < 8; i++) {
        if (vin[i] == ' ') return -1;  // No spaces allowed
    }
    
    return 0;
}

/**
 * Vehicle Identifier Section (VIS) validation
 * Characters 10-17
 */
static int vis_test(const char *vin) {
    // Character 10 is model year
    // Valid year codes: 1-9, A-H, J-N, P, R-Y (excludes 0, I, O, Q, U, Z)
    char year_char = vin[9];
    const char* valid_year_codes = "123456789ABCDEFGHJKLMNPRSTVWXY";
    if (strchr(valid_year_codes, year_char) == NULL) return -1;
    
    // Character 11 is plant code
    char plant = vin[10];
    if (strchr(valid_vin_chars, plant) == NULL) return -1;
    
    // Characters 12-17 are sequential production number
    // For model years 2001-2009 and 2031-2039 (year codes '1'-'9'), these must be numeric
    bool requires_numeric = (year_char >= '1' && year_char <= '9');
    
    if (requires_numeric) {
        for (int i = 11; i < 17; i++) {
            if (!isdigit(vin[i])) return -1;
        }
    }
    
    return 0;
}

/**
 * Pattern detection to avoid false positives
 * Similar to credit card pattern test
 */
static int pattern_test(const char *vin) {
    // Check if VIN is entirely numeric (invalid for post-1981 VINs)
    bool all_numeric = true;
    for (int i = 0; i < 17; i++) {
        if (!isdigit(vin[i])) {
            all_numeric = false;
            break;
        }
    }
    if (all_numeric) return -1;
    
    // Check for repeating patterns that indicate non-VIN data
    int same_count = 1;
    char last_char = vin[0];
    
    for (int i = 1; i < 17; i++) {
        if (vin[i] == last_char) {
            same_count++;
            if (same_count > 4) return -1;  // Too many repeated characters
        } else {
            same_count = 1;
            last_char = vin[i];
        }
    }
    
    // Check for sequential patterns
    int sequential = 0;
    for (int i = 1; i < 17; i++) {
        if (abs(vin[i] - vin[i-1]) == 1) {
            sequential++;
            if (sequential > 5) return -1;  // Too many sequential characters
        } else {
            sequential = 0;
        }
    }
    
    return 0;
}

#define RETURN(code, reason) {if(scan_vin_debug){std::cerr << reason << "\n";} return code;}

/**
 * Main VIN validation function
 * Returns true if valid VIN, false otherwise
 */
bool valid_vin(const char *buf, int buflen) {
    if (buflen != 17) RETURN(false, "VIN must be exactly 17 characters");
    
    char vin[18];  // 17 chars + null terminator
    memset(vin, 0, sizeof(vin));
    
    // Extract and validate characters
    if (extract_vin_chars(buf, buflen, vin)) RETURN(false, "Failed character extraction");
    
    // Run validation tests
    if (check_digit_test(vin)) RETURN(false, "Failed check digit test");
    if (wmi_test(vin)) RETURN(false, "Failed WMI test");
    if (vds_test(vin)) RETURN(false, "Failed VDS test");
    if (vis_test(vin)) RETURN(false, "Failed VIS test");
    if (pattern_test(vin)) RETURN(false, "Failed pattern test");
    
    // Note: Context checking is done at the scanner level in scan_vehicle.flex
    // where we have safe access to surrounding buffer contents
    
    return true;
}

#ifdef DEBUG
static int validate_vin_debug(const char *buf, int buflen) {
    char vin[18];
    
    printf("Running VIN validation tests. 0 means passed, -1 means failed.\n\n");
    printf("extract_vin_chars(%s) = %d\n", buf, extract_vin_chars(buf, buflen, vin));
    printf("check_digit_test(%s) = %d\n", vin, check_digit_test(vin));
    printf("wmi_test(%s) = %d\n", vin, wmi_test(vin));
    printf("vds_test(%s) = %d\n", vin, vds_test(vin));
    printf("vis_test(%s) = %d\n", vin, vis_test(vin));
    printf("pattern_test(%s) = %d\n", vin, pattern_test(vin));
    
    return valid_vin(buf, buflen);
}
#endif
