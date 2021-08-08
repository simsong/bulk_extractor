#ifndef SCAN_NET_H
#define SCAN_NET_H

#ifndef PACKAGE_NAME
#error scan_net.h requires that autoconf-generated config.h be included first
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_WSIPX_H
typedef char sa_family_t;
#endif

#include "be13_api/utils.h" // needs config.h
#include "be13_api/scanner_params.h"
#include "be13_api/sbuf.h"
#include "be13_api/packet_info.h"

struct scan_net {
    static inline const std::string CHKSUM_OK  {"cksum-ok"};
    static inline const std::string CHKSUM_BAD {"cksum-bad"};

/* generic ip header for IPv4 and IPv6 packets */
    typedef struct generic_iphdr {
        sa_family_t family;		/* AF_INET or AF_INET6 */
        uint8_t src[16];		/* Source IP address; holds v4 or v6 */
        uint8_t dst[16];		/* Destination IP address; holds v4 or v6 */
        uint8_t ttl;		/* ttl from ip_hdr and hop_limit for ip6_hdr */
        uint8_t nxthdr;		/* nxt hdr type */
        uint8_t nxthdr_offs;	/* nxt hdr offset, also IP hdr len */
        uint16_t payload_len; 	/* IP total len - IP hdr */
    } generic_iphdr_t;

/* pseudo-header of our making */
    struct macip {
        be13::ether_addr ether_dhost;
        be13::ether_addr ether_shost;
        uint16_t ether_type;
        uint8_t ipv;
    };

    static uint16_t ip4_cksum(const sbuf_t &sbuf, size_t pos, size_t len);
    static uint16_t IPv6L3Chksum(const sbuf_t &sbuf, size_t pos, u_int chksum_byteoffset);
    static bool sanityCheckIP46Header(const sbuf_t &sbuf, size_t pos, bool *checksum_valid,
                                      generic_iphdr_t *h);

    bool carve_net_memory {false};      // should we carve for network memory?

};

#endif
