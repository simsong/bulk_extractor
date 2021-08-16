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

#include "be13_api/utils.h" // needs config.h
#include "be13_api/scanner_params.h"
#include "be13_api/sbuf.h"
#include "be13_api/packet_info.h"
#include "be13_api/feature_recorder.h"

struct scan_net_t {
    scan_net_t(const scan_net_t &that) = delete;
    scan_net_t &operator=(const scan_net_t & that) = delete;

    scan_net_t();
    virtual ~scan_net_t();
    static inline const std::string CHKSUM_OK  {"cksum-ok"};
    static inline const std::string CHKSUM_BAD {"cksum-bad"};
    feature_recorder *ip_recorder {nullptr};
    feature_recorder *tcp_recorder {nullptr};
    feature_recorder *ether_recorder {nullptr};

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
    static bool sanityCheckIP46Header(const sbuf_t &sbuf, size_t pos, generic_iphdr_t *h);

    bool carve_net_memory {false};      // should we carve for network memory?

    /* Header for the PCAP file */
    constexpr static uint8_t PCAP_HEADER[] {
        0xd4, 0xc3, 0xb2, 0xa1, // magic
        0x02, 0x00, 0x04, 0x00  // version_major, version_minor
            };
    /* Header for each packet */
    constexpr static size_t PCAP_RECORD_HEADER_SIZE = 16;

    /*
     * Sanity check the header values
     */
    struct pcap_hdr {
        pcap_hdr(uint32_t s,uint32_t u,uint32_t c,uint32_t l):seconds(s),useconds(u),cap_len(c),pkt_len(l){}
        pcap_hdr():seconds(0),useconds(0),cap_len(0),pkt_len(0){}
        uint32_t seconds;
        uint32_t useconds;
        uint32_t cap_len;
        uint32_t pkt_len;
    };


    bool likely_valid_pcap_packet_header(const sbuf_t &sbuf, size_t pos, struct pcap_hdr &h) const {
        if (sbuf.bufsize < pos + PCAP_RECORD_HEADER_SIZE  ) return false; // no room

        h.seconds  = sbuf.get32u( pos+0 ); if (h.seconds==0) return false;
        h.useconds = sbuf.get32u( pos+4 ); if (h.useconds>1000000) return false;
        h.cap_len  = sbuf.get32u( pos+8 ); if (h.cap_len<min_packet_size) return false;
        h.pkt_len  = sbuf.get32u( pos+12); if (h.pkt_len<min_packet_size) return false;

        if (h.seconds < TIME_MIN || h.seconds > TIME_MAX) return false;

        if (h.cap_len<min_packet_size || h.cap_len>max_packet_len) return false;
        if (h.pkt_len<min_packet_size || h.pkt_len>max_packet_len) return false;
        if (h.cap_len > h.pkt_len) return false;
        return true;
    };

    /* Hardcoded tunings */
    const static inline size_t PCAP_MAX_PKT_LEN  = 65535;	// The longest a packet may be; longer values make wireshark refuse to load
    const static inline uint16_t sane_ports[] = {80, 443, 53, 25, 110, 143, 993, 587, 23, 22, 21, 20, 119, 123};
    constexpr static inline uint16_t sane_ports_len = sizeof(sane_ports) / sizeof(uint16_t);

    const static inline uint32_t JAN1_1990 = 631152000;   // we won't get packets before this date.
    const static inline uint32_t TIME_MIN = JAN1_1990;
    const static inline uint32_t max_packet_len = 65535;
    const static inline uint32_t min_packet_size = 20;		// don't bother with ethernet packets smaller than this
    const static inline uint32_t MIN_SBUF_SIZE = 16; // min structure size

    // TIME_MAX is the maximum time for a pcap that the system will carve.
    // set it to two years in the future from when the program is running.
    const uint32_t TIME_MAX {static_cast<uint32_t>(time(0)) + 2 * 365 * 24 * 60 * 60};
    const static inline std::string TCPDUMP_FR_FEATURE {"0xd4,0xc3,0xb2,0xa1"};
    const static inline std::string TCPDUMP_FR_CONTEXT {"TCPDUMP file"};
    const static inline uint32_t TCPDUMP_HEADER_SIZE = 24;

    /* primitive port heuristics */
    bool sanePort(const uint16_t port) const {
        for (int i=0; i<sane_ports_len; i++) {
            if (port == ntohs(sane_ports[i]))
                return true;
        }
        return false;
    };
    /* subclassed in pcap_writer.
     * Write the packet to the packet stream, writing the pcap header first and optionally adding a synthetic ethernet frame.
     */
    virtual void flush() const {};
    virtual void pcap_writepkt(const struct pcap_hdr &h, // packet header
                               const sbuf_t &sbuf,       // sbuf where packet is located
                               const size_t pos,         // position within the sbuf
                               const bool add_frame,     // whether or not to create a synthetic ethernet frame
                               const uint16_t frame_type) const;  // ethernet frame type

    /* Each of these carvers looks for a specific structure and if it finds the structure it returns the size in the sbuf */
    void documentIPFields(const sbuf_t &sbuf, size_t pos, const generic_iphdr_t &h) const;
    size_t carveIPFrame(const sbuf_t &sbuf, size_t pos) const;
    size_t carveTCPTOBJ(const sbuf_t &sbuf, size_t pos) const;
    size_t carveSockAddrIn(const sbuf_t &sbuf, size_t pos) const;
    size_t carvePCAPPacket(const sbuf_t &sbuf, size_t pos) const;
    size_t carvePCAPFile(const sbuf_t &sbuf, size_t pos) const;
    size_t carveEther(const sbuf_t &sbuf, size_t pos) const;
    void carve(const sbuf_t &sbuf) const;

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
