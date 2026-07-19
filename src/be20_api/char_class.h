/**
 * \class CharClass
 * Examine a block of text and count the number of characters
 * in various ranges. This is useful for determining if a block of
 * bytes is coded in BASE16, BASE64, etc.
 */

#ifndef CHAR_CLASS_H
#define CHAR_CLASS_H
struct CharClass {
    uint32_t range_0_9{0};  // a range_0_9 character
    uint32_t range_A_Fi{0}; // a-f or A-F
    uint32_t range_g_z{0};  // g-z
    uint32_t range_G_Z{0};  // G-Z
    CharClass() {}
    void add(const uint8_t ch) {
        if (ch >= 'a' && ch <= 'f') range_A_Fi++;
        if (ch >= 'A' && ch <= 'F') range_A_Fi++;
        if (ch >= 'g' && ch <= 'z') range_g_z++;
        if (ch >= 'G' && ch <= 'Z') range_G_Z++;
        if (ch >= '0' && ch <= '9') range_0_9++;
    }
    void add(const uint8_t* buf, size_t len) {
        for (size_t i = 0; i < len; i++) { add(buf[i]); }
    }
};

#endif
