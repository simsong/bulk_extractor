#ifndef SCAN_NET_H
#define SCAN_NET_H
#include "be13_api/sbuf.h"

struct scan_net {
    static uint16_t IPv6L3Chksum(const sbuf_t &sbuf, size_t pos, u_int chksum_byteoffset);
    static inline const std::string CHKSUM_OK  {"cksum-ok"};
    static inline const std::string CHKSUM_BAD {"cksum-bad"};
};

#endif
