#ifndef SCAN_NET_H
#define SCAN_NET_H

#ifndef PACKAGE_NAME
#error scan_net.h requires that autoconf-generated config.h be included first
#endif

#include <cstdlib>
#include <cstring>

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

#include "be20_api/utils.h" // needs config.h
#include "be20_api/scanner_params.h"
#include "be20_api/sbuf.h"
#include "be20_api/packet_info.h"
#include "be20_api/feature_recorder.h"
#include "pcap_writer.h"

struct scan_net_t {

    /* Hardcoded tunings */
    const static inline uint16_t sane_ports[] = {80, 443, 53, 25, 110, 143, 993, 587, 23, 22, 21, 20, 119, 123};
    constexpr static inline uint16_t sane_ports_len = sizeof(sane_ports) / sizeof(uint16_t);

    const static inline uint32_t JAN1_1990 = 631152000;   // we won't get packets before this date.
    const static inline uint32_t TIME_MIN = JAN1_1990;
    const static inline uint32_t MAX_PACKET_BYTES = 65535;
    const static inline uint32_t DEFAULT_MIN_PACKET_BYTES = 40;
    const static inline uint32_t MIN_PACKET_BYTES = 40; // safety logic fails space beyond pos is smaller than this.

    /* State variables go here */
    bool carve_net_memory { false };      // should we carve for network memory?
    size_t min_packet_bytes { DEFAULT_MIN_PACKET_BYTES };

    /* generic ip header for IPv4 and IPv6 packets */
    typedef struct generic_iphdr {
        sa_family_t family {};		/* AF_INET or AF_INET6 */
        uint8_t src[16];		/* Source IP address; holds v4 or v6 */
        uint8_t dst[16];		/* Destination IP address; holds v4 or v6 */
        uint8_t ttl {};		/* ttl from ip_hdr and hop_limit for ip6_hdr */
        uint8_t nxthdr {};		/* nxt hdr type */
        uint8_t nxthdr_offs {};	/* nxt hdr offset, also IP hdr len */
        uint16_t payload_len {}; 	/* IP total len - IP hdr */
        bool checksum_valid {false};  // if computed checksum was valid
        bool is_4or6() const { return family==AF_INET || family==AF_INET6;};
        bool is_4() const { return family==AF_INET;};
        bool is_6() const { return family==AF_INET6;};
    } generic_iphdr_t;

    /* pseudo-header of our making */
    struct macip {
        be20::ether_addr ether_dhost;
        be20::ether_addr ether_shost;
        uint16_t ether_type;
        uint8_t ip_version;                    // ip header typically follows (4|header_len) or (6|traffic_class top 4 bits)
    };

    /* testing functions */
    static bool isPowerOfTwo(const uint8_t val);
    static bool invalidMAC(const be20::ether_addr *const e);
    static bool invalidIP4( const uint8_t *const cc);
    static bool invalidIP6(const uint16_t addr[8]);
    static bool invalidIP(const uint8_t addr[16], sa_family_t family);
    static std::string ip2string(const struct be20::ip4_addr *const a);
    static std::string ip2string(const uint8_t *addr, sa_family_t family);
    static std::string mac2string(const struct be20::ether_addr *const e);
    static inline std::string i2str(const int i);
    typedef std::unordered_set<size_t> sanityCache_t;
    static bool sanityCheckIP46Header(const sbuf_t &sbuf, size_t pos, generic_iphdr_t *h, sanityCache_t *sc);

    /* regular functions */

    scan_net_t(const scan_net_t &that) = delete;
    scan_net_t &operator=(const scan_net_t & that) = delete;
    scan_net_t(const scanner_params &sp);
    static inline const std::string CHKSUM_OK  {"cksum-ok"};
    static inline const std::string CHKSUM_BAD {"cksum-bad"};
    mutable pcap_writer    pwriter;
    feature_recorder &ip_recorder;
    feature_recorder &tcp_recorder;
    feature_recorder &ether_recorder;

    static uint16_t ip4_cksum(const sbuf_t &sbuf, size_t pos, size_t len);
    static bool ip6_cksum_valid(const sbuf_t &sbuf, size_t pos); // works for UDP
    static uint16_t IPv6L3Chksum(const sbuf_t &sbuf, size_t pos, u_int chksum_byteoffset);

    /* Header for the PCAP file */
    constexpr static uint8_t PCAP_FILE_HEADER[] {
        0xd4, 0xc3, 0xb2, 0xa1, // magic
        0x02, 0x00, 0x04, 0x00  // version_major, version_minor
            };
    /* Header for each packet */
    constexpr static size_t PCAP_RECORD_HEADER_SIZE = 16;

    /** Check the fields at sbuf+pos and as we decode them fill in h */
    bool likely_valid_pcap_packet_header(const sbuf_t &sbuf, size_t pos, struct pcap_writer::pcap_hdr *h) const {
        if (sbuf.bufsize < pos + PCAP_RECORD_HEADER_SIZE  ) return false; // no room

        h->seconds  = sbuf.get32u_unsafe( pos+0 );
        if (h->seconds==0 || h->seconds < TIME_MIN || h->seconds > TIME_MAX) return false;

        h->useconds = sbuf.get32u_unsafe( pos+4 );
        if (h->useconds>1000000) return false;

        h->cap_len  = sbuf.get32u_unsafe( pos+8 );
        if (h->cap_len<MIN_PACKET_BYTES || h->cap_len>MAX_PACKET_BYTES) return false;

        h->pkt_len  = sbuf.get32u_unsafe( pos+12);
        if (h->pkt_len<MIN_PACKET_BYTES || h->pkt_len>MAX_PACKET_BYTES) return false;

        if (h->cap_len > h->pkt_len) return false;
        return true;
    };

    // TIME_MAX is the maximum time for a pcap that the system will carve.
    // set it to two years in the future from when the program is running.
    const uint32_t TIME_MAX {static_cast<uint32_t>(time(0)) + 2 * 365 * 24 * 60 * 60};

    /* primitive port heuristics */
    bool sanePort(const uint16_t port) const {
        for (int i=0; i<sane_ports_len; i++) {
            if (port == ntohs(sane_ports[i]))
                return true;
        }
        return false;
    };

    class port0_exception : public std::exception {
    public:
        port0_exception(){};
    };

    /* Each of these carvers looks for a specific structure and if it finds the structure it returns the size in the sbuf */
    void   documentIPFields(const sbuf_t &sbuf, size_t pos, const generic_iphdr_t &h) const; // write ip fields to ip_recorder and possibly tcp_recorder
    size_t carveIPFrame(const sbuf_t &sbuf, size_t pos, sanityCache_t *sc) const;
    size_t carveTCPTOBJ(const sbuf_t &sbuf, size_t pos) const;
    size_t carveSockAddrIn(const sbuf_t &sbuf, size_t pos) const;
    size_t carvePCAPFileHeader(const sbuf_t &sbuf, size_t pos) const;
    size_t carvePCAPPackets(const sbuf_t &sbuf, size_t pos, sanityCache_t *sc) const;
    static const struct macip *validateEther(const sbuf_t &sbuf, size_t pos) {
        const struct macip *er = sbuf.get_struct_ptr_unsafe<struct macip>(pos);
        /* Only carve ether type ETHERTYPE_IP or ETHERTYPE_IPv6.
         * Run htons() on constants to compute at compile time
         */

        const uint16_t ip4_ether  = htons(ETHERTYPE_IP); // constexpr gives error on ubuntu20!
        const uint16_t ip6_ether  = htons(ETHERTYPE_IPV6);
        if (er->ether_type == ip4_ether && er->ip_version == 0x45) {
            return er;                  // ipv4
        }
        if (er->ether_type == ip6_ether && ((er->ip_version & 0xF0) == 0x60)) {
            return er;                  // ipv6
        }
        return nullptr;                 // couldn't find anything
    }

    size_t carveEther(const sbuf_t &sbuf, size_t pos, sanityCache_t *sc) const;
    void   carve(const sbuf_t &sbuf) const;
};

inline std::ostream& operator<<(std::ostream& os, const scan_net_t::generic_iphdr_t &h) {
    os << "<< generic_iphdr_t family=";
    switch (h.family) {
    case AF_INET: os << "AFINET";break;
    case AF_INET6: os << "AFINET6";break;
    default: os << int(h.family);
    }
    os << " ttl=" << int(h.ttl)
       << " payload_len=" << int(h.payload_len) << " checksum_valid=" << int(h.checksum_valid) << ">>";
    return os;
};


#endif
