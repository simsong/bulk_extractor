#ifndef PACKET_INFO_H
#define PACKET_INFO_H

/*****************************************************************
 *** bulk_extractor has a private implementation of IPv4 and IPv6,
 *** UDP and TCP.
 ***
 *** We did this becuase we found slightly different versions on
 *** MacOS, Ubuntu Linux, Fedora Linux, Centos, Mingw, and Cygwin.
 *** TCP/IP isn't changing anytime soon, and when it changes (as it
 *** did with IPv6), these different systems all implemented it slightly
 *** differently, and that caused a lot of problems for us.
 *** So the BE20API has a single implementation and it's good enough
 *** for our uses.
 ***/

/* Network includes */

/****************************************************************
 *** pcap.h --- If we don't have it, fake it. ---
 ***/
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h> // for freebsd
#endif

#if defined(HAVE_LIBPCAP)
#ifdef HAVE_DIAGNOSTIC_REDUNDANT_DECLS
#pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
#if defined(HAVE_PCAP_PCAP_H)
#include <pcap/pcap.h>
#define GOT_PCAP
#endif
#if defined(HAVE_PCAP_H) && !defined(GOT_PCAP)
#include <pcap.h>
#define GOT_PCAP
#endif
#if defined(HAVE_WPCAP_PCAP_H) && !defined(GOT_PCAP)
#include <wpcap/pcap.h>
#define GOT_PCAP
#endif
#ifdef HAVE_DIAGNOSTIC_REDUNDANT_DECLS
#pragma GCC diagnostic warning "-Wredundant-decls"
#endif
#else
#include "pcap_fake.h"
#endif

/* namespace needed here because of conflicts below */
namespace be20 {

#ifndef ETH_ALEN
#define ETH_ALEN 6 // ethernet address len
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6 /* tcp */
#endif

    struct ether_addr {
        uint8_t ether_addr_octet[ETH_ALEN];
    } __attribute__((__packed__));

/* 10Mb/s ethernet header */
    struct ether_header {
        uint8_t ether_dhost[ETH_ALEN]; /* destination eth addr */
        uint8_t ether_shost[ETH_ALEN]; /* source ether addr    */
        uint16_t ether_type;           /* packet type ID field */
    } __attribute__((__packed__));

/* The mess below is becuase these items are typedefs and
 * structs on some systems and #defines on other systems
 * So in the interest of portability we need to define *new*
 * structures that are only used here
 */

    typedef uint32_t ip4_addr_t; // historical

// on windows we use the definition that's in winsock
    struct ip4_addr {
        ip4_addr_t addr;
    };

/*
 * Structure of an internet header, naked of options.
 */
    struct ip4 {
#ifdef BE20API_BIGENDIAN
        uint8_t ip_v : 4;  /* version */
        uint8_t ip_hl : 4; /* header length */
#else
        uint8_t ip_hl : 4;  /* header length */
        uint8_t ip_v : 4;   /* version */
#endif
        uint8_t  ip_tos;                 /* type of service */
    private:;
        uint16_t ip_len_;                /* total length */
    public:
        uint16_t ip_len() const { return ntohs(ip_len_);};
        uint16_t ip_id;                 /* identification */
        uint16_t ip_off;                /* fragment offset field */
#define IP_RF 0x8000                /* reserved fragment flag */
#define IP_DF 0x4000                /* dont fragment flag */
#define IP_MF 0x2000                /* more fragments flag */
#define IP_OFFMASK 0x1fff           /* mask for fragmenting bits */
        uint8_t ip_ttl;                 /* time to live */
        uint8_t ip_p;                   /* protocol */
        uint16_t ip_sum;                /* checksum */
        struct ip4_addr ip_src, ip_dst; /* source and dest address */
    } __attribute__((__packed__));

    struct ip4_dgram {
        const struct ip4* header;
        const uint8_t* payload;
        uint16_t payload_len;
    };

/*
 * IPv6 header structure
 */
    struct ip6_addr { // our own private ipv6 definition
        union {
            uint8_t addr8[16]; // three ways to get the data
            uint16_t addr16[8];
            uint32_t addr32[4];
        } addr; /* 128-bit IP6 address */
    };

    struct ip6_hdr {
        union {
            struct ip6_hdrctl {
                uint32_t ip6_un1_flow; /* 20 bits of flow-ID */
                uint16_t ip6_un1_plen; /* payload length */
                uint8_t ip6_un1_nxt;   /* next header */
                uint8_t ip6_un1_hlim;  /* hop limit */
            } ip6_un1;
            uint8_t ip6_un2_vfc; /* 4 bits version, top 4 bits class */
        } ip6_ctlun;
        struct ip6_addr ip6_src; /* source address */
        struct ip6_addr ip6_dst; /* destination address */

        uint8_t ip6_vfc() const { return ip6_ctlun.ip6_un2_vfc;};
        uint32_t ip6_flow() const { return ip6_ctlun.ip6_un1.ip6_un1_flow;};
        uint16_t ip6_plen() const { return ip6_ctlun.ip6_un1.ip6_un1_plen;};
        uint8_t ip6_nxt() const { return ip6_ctlun.ip6_un1.ip6_un1_nxt;};
        uint8_t ip6_hlim() const { return ip6_ctlun.ip6_un1.ip6_un1_hlim;};
        uint8_t ip6_hops() const { return ip6_ctlun.ip6_un1.ip6_un1_hlim;};
        bool    is_ipv6() const { return (ip6_vfc() & 0xF0)==0x60;};

    } __attribute__((__packed__));

    struct ip6_dgram {
        const struct ip6_hdr* header;
        const uint8_t* payload;
        uint16_t payload_len;
    };

/*
 * TCP header.
 * Per RFC 793, September, 1981.
 */
    typedef uint32_t tcp_seq;
    struct tcphdr {
        uint16_t th_sport; /* source port */
        uint16_t th_dport; /* destination port */
        tcp_seq th_seq;    /* sequence number */
        tcp_seq th_ack;    /* acknowledgement number */
#ifdef BE20_API_BIGENDIAN
        uint8_t th_off : 4; /* data offset */
        uint8_t th_x2 : 4;  /* (unused) */
#else
        uint8_t th_x2 : 4;  /* (unused) */
        uint8_t th_off : 4; /* data offset */
#endif
        uint8_t th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
        uint16_t th_win; /* window */
        uint16_t th_sum; /* checksum */
        uint16_t th_urp; /* urgent pointer */
    } __attribute__((packed));

    struct icmp6_hdr {
        uint8_t	icmp6_type;	/* type field */
        uint8_t	icmp6_code;	/* code field */
        uint16_t	icmp6_cksum;	/* checksum field */
        union {
            uint32_t	icmp6_un_data32[1]; /* type-specific field */
            uint16_t	icmp6_un_data16[2]; /* type-specific field */
            uint8_t	icmp6_un_data8[4];  /* type-specific field */
        } icmp6_dataun;
    } __attribute__((__packed__));



/*
 * The packet_info structure records packets after they are read from the pcap library.
 * It preserves the original pcap information and information decoded from the MAC and
 * VLAN (IEEE 802.1Q) layers, as well as information that might be present from 802.11
 * interfaces. However it does not preserve the full radiotap information.
 *
 * packet_info is created to make it easier to write network forensic software. It encapsulates
 * much of the common knowledge needed to operate on packet-based IP networks.
 *
 * @param ts   - the actual packet time to use (adjusted)
 * @param pcap_data - Original data offset point from pcap
 * @param data - the actual packet data, minus the MAC layer
 * @param datalen - How much data is available at the datalen pointer
 *
 */
    class packet_info {
    public:
        // IPv4 header offsets
        static const size_t ip4_proto_off = 9;
        static const size_t ip4_src_off = 12;
        static const size_t ip4_dst_off = 16;
        // IPv6 header offsets
        static const size_t ip6_nxt_hdr_off = 6;
        static const size_t ip6_plen_off = 4;
        static const size_t ip6_src_off = 8;
        static const size_t ip6_dst_off = 24;
        // TCP header offsets
        static const size_t tcp_sport_off = 0;
        static const size_t tcp_dport_off = 2;

        class frame_too_short : public std::logic_error {
        public:
            frame_too_short() : std::logic_error("frame too short to contain requisite network structures") {}
        };

        enum vlan_t { NO_VLAN = -1 };
        /** create a packet, usually an IP packet.
         * @param d - start of MAC packet
         * @param d2 - start of IP data
         */
        packet_info(const int dlt, const struct pcap_pkthdr* h, const uint8_t* d, const struct timeval& ts_,
                    const uint8_t* d2, size_t dl2)
            : pcap_dlt(dlt), pcap_hdr(h), pcap_data(d), ts(ts_), ip_data(d2), ip_datalen(dl2) {}
        packet_info(const int dlt, const struct pcap_pkthdr* h, const uint8_t* d)
            : pcap_dlt(dlt), pcap_hdr(h), pcap_data(d), ts(h->ts), ip_data(d), ip_datalen(h->caplen) {}

        const int pcap_dlt;                 // data link type; needed by libpcap, not provided
        const struct pcap_pkthdr* pcap_hdr; // provided by libpcap
        const uint8_t* pcap_data;            // provided by libpcap; where the MAC layer begins
        const struct timeval& ts;           // when packet received; possibly modified before packet_info created
        const uint8_t* const ip_data;       // pointer to where ip data begins
        const size_t ip_datalen;            // length of ip data

        static uint16_t nshort(const uint8_t* buf, size_t pos); // return a network byte order short at offset pos
        int ip_version() const;                               // returns 4, 6 or 0
        uint16_t ether_type() const;                           // returns 0 if not IEEE802, otherwise returns ether_type
        int vlan() const;                                     // returns NO_VLAN if not IEEE802 or not VLAN, othererwise VID
        const uint8_t* get_ether_dhost() const;               // returns a pointer to ether dhost if ether packet
        const uint8_t* get_ether_shost() const;               // returns a pointer to ether shost if ether packet

        // packet typing
        bool is_ip4() const;
        bool is_ip6() const;
        bool is_ip4_tcp() const;
        bool is_ip6_tcp() const;
        // packet extraction
        // IPv4 - return pointers to fields or throws frame_too_short exception
        const struct in_addr* get_ip4_src() const;
        const struct in_addr* get_ip4_dst() const;
        uint8_t get_ip4_proto() const;
        // IPv6
        uint8_t get_ip6_nxt_hdr() const;
        uint16_t get_ip6_plen() const;
        const struct ip6_addr* get_ip6_src() const;
        const struct ip6_addr* get_ip6_dst() const;
        // TCP
        uint16_t get_ip4_tcp_sport() const;
        uint16_t get_ip4_tcp_dport() const;
        uint16_t get_ip6_tcp_sport() const;
        uint16_t get_ip6_tcp_dport() const;
    };

#ifdef DLT_IEEE802
    inline uint16_t packet_info::ether_type() const {

        if (pcap_dlt == DLT_IEEE802 || pcap_dlt == DLT_EN10MB) {
            const struct ether_header* eth_header = (struct ether_header*)pcap_data;
            return ntohs(eth_header->ether_type);
        }
        return 0;
    }
#endif


#ifndef ETHERTYPE_PUP
#define ETHERTYPE_PUP 0x0200 /* Xerox PUP */
#endif

#ifndef ETHERTYPE_SPRITE
#define ETHERTYPE_SPRITE 0x0500 /* Sprite */
#endif

#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP 0x0800 /* IP */
#endif

#ifndef ETHERTYPE_ARP
#define ETHERTYPE_ARP 0x0806 /* Address resolution */
#endif

#ifndef ETHERTYPE_REVARP
#define ETHERTYPE_REVARP 0x8035 /* Reverse ARP */
#endif

#ifndef ETHERTYPE_AT
#define ETHERTYPE_AT 0x809B /* AppleTalk protocol */
#endif

#ifndef ETHERTYPE_AARP
#define ETHERTYPE_AARP 0x80F3 /* AppleTalk ARP */
#endif

#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN 0x8100 /* IEEE 802.1Q VLAN tagging */
#endif

#ifndef ETHERTYPE_IPX
#define ETHERTYPE_IPX 0x8137 /* IPX */
#endif

#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6 0x86dd /* IP protocol version 6 */
#endif

#ifndef ETHERTYPE_LOOPBACK
#define ETHERTYPE_LOOPBACK 0x9000 /* used to test interfaces */
#endif

    inline uint16_t packet_info::nshort(const uint8_t* buf, size_t pos) { return (buf[pos] << 8) | (buf[pos + 1]); }

    inline int packet_info::vlan() const {
        if (ether_type() == ETHERTYPE_VLAN) { return nshort(pcap_data, sizeof(struct ether_header)); }
        return -1;
    }

    inline int packet_info::ip_version() const {
        /* This takes advantage of the fact that ip4 and ip6 put the version number in the same place */
        if (ip_datalen >= sizeof(struct ip4)) {
            const struct ip4* ip_header = (struct ip4*)ip_data;
            switch (ip_header->ip_v) {
            case 4: return 4;
            case 6: return 6;
            }
        }
        return 0;
    }

// packet typing

    inline bool packet_info::is_ip4() const { return ip_version() == 4; }

    inline bool packet_info::is_ip6() const { return ip_version() == 6; }

    inline bool packet_info::is_ip4_tcp() const {
        if (ip_datalen < sizeof(struct ip4) + sizeof(struct tcphdr)) { return false; }
        return *((uint8_t*)(ip_data + ip4_proto_off)) == IPPROTO_TCP;
        return false;
    }

    inline bool packet_info::is_ip6_tcp() const {
        if (ip_datalen < sizeof(struct ip6_hdr) + sizeof(struct tcphdr)) { return false; }
        return *((uint8_t*)(ip_data + ip6_nxt_hdr_off)) == IPPROTO_TCP;
    }

// packet extraction
// precondition: the apropriate packet type function must return true before using these functions.
//     example: is_ip4_tcp() must return true before calling get_ip4_tcp_sport()

// Get ether addresses; should this handle vlan and such?
    inline const uint8_t* packet_info::get_ether_dhost() const {
        if (pcap_hdr->caplen < sizeof(struct ether_addr)) { throw frame_too_short(); }
        return ((const struct ether_header*)pcap_data)->ether_dhost;
    }

    inline const uint8_t* packet_info::get_ether_shost() const {
        if (pcap_hdr->caplen < sizeof(struct ether_addr)) { throw frame_too_short(); }
        return ((const struct ether_header*)pcap_data)->ether_shost;
    }

// IPv4
#ifdef HAVE_DIAGNOSTIC_CAST_ALIGN
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
    inline const struct in_addr* packet_info::get_ip4_src() const {
        if (ip_datalen < sizeof(struct ip4)) { throw frame_too_short(); }
        return (const struct in_addr*)ip_data + ip4_src_off;
    }
    inline const struct in_addr* packet_info::get_ip4_dst() const {
        if (ip_datalen < sizeof(struct ip4)) { throw frame_too_short(); }
        return (const struct in_addr*)ip_data + ip4_dst_off;
    }
#ifdef HAVE_DIAGNOSTIC_CAST_ALIGN
#pragma GCC diagnostic warning "-Wcast-align"
#endif
    inline uint8_t packet_info::get_ip4_proto() const {
        if (ip_datalen < sizeof(struct ip4)) { throw frame_too_short(); }
        return *((uint8_t*)(ip_data + ip4_proto_off));
    }
// IPv6
    inline uint8_t packet_info::get_ip6_nxt_hdr() const {
        if (ip_datalen < sizeof(struct ip6_hdr)) { throw frame_too_short(); }
        return *((uint8_t*)(ip_data + ip6_nxt_hdr_off));
    }
    inline uint16_t packet_info::get_ip6_plen() const {
        if (ip_datalen < sizeof(struct ip6_hdr)) { throw frame_too_short(); }
        // return ntohs(*((uint16_t *) (ip_data + ip6_plen_off)));
        return nshort(ip_data, ip6_plen_off);
    }
#ifdef HAVE_DIAGNOSTIC_CAST_ALIGN
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
    inline const struct ip6_addr* packet_info::get_ip6_src() const {
        if (ip_datalen < sizeof(struct ip6_hdr)) { throw frame_too_short(); }
        return (const struct ip6_addr*)ip_data + ip6_src_off;
    }
    inline const struct ip6_addr* packet_info::get_ip6_dst() const {
        if (ip_datalen < sizeof(struct ip6_hdr)) { throw frame_too_short(); }
        return (const struct ip6_addr*)ip_data + ip6_dst_off;
    }
#ifdef HAVE_DIAGNOSTIC_CAST_ALIGN
#pragma GCC diagnostic warning "-Wcast-align"
#endif

// TCP
    inline uint16_t packet_info::get_ip4_tcp_sport() const {
        if (ip_datalen < sizeof(struct tcphdr) + sizeof(struct ip4)) { throw frame_too_short(); }
        // return ntohs(*((uint16_t *) (ip_data + sizeof(struct ip4) + tcp_sport_off)));
        return nshort(ip_data, sizeof(struct ip4) + tcp_sport_off);
    }
    inline uint16_t packet_info::get_ip4_tcp_dport() const {
        if (ip_datalen < sizeof(struct tcphdr) + sizeof(struct ip4)) { throw frame_too_short(); }
        // return ntohs(*((uint16_t *) (ip_data + sizeof(struct ip4) + tcp_dport_off)));
        return nshort(ip_data, sizeof(struct ip4) + tcp_dport_off); //
    }
    inline uint16_t packet_info::get_ip6_tcp_sport() const {
        if (ip_datalen < sizeof(struct tcphdr) + sizeof(struct ip6_hdr)) { throw frame_too_short(); }
        // return ntohs(*((uint16_t *) (ip_data + sizeof(struct ip6_hdr) + tcp_sport_off)));
        return nshort(ip_data, sizeof(struct ip6_hdr) + tcp_sport_off); //
    }
    inline uint16_t packet_info::get_ip6_tcp_dport() const {
        if (ip_datalen < sizeof(struct tcphdr) + sizeof(struct ip6_hdr)) { throw frame_too_short(); }
        // return ntohs(*((uint16_t *) (ip_data + sizeof(struct ip6_hdr) + tcp_dport_off)));
        return nshort(ip_data, sizeof(struct ip6_hdr) + tcp_dport_off); //
    }

/* A packet_info provided as a callback.
 */
    typedef void packet_callback_t(void* user, const be20::packet_info& pi);

    /* a class to compute 1s complement checksums */
    class adder1 {
        uint32_t sum {0};
    public:
        void add(uint16_t val) {
            sum += val;
            if (sum & 0x80000000){   /* if high order bit set, fold */
                sum = (sum & 0xFFFF) + (sum >> 16);
            }
        };

        uint16_t chksum()  {
            while (sum>>16) {
                sum = (sum & 0xFFFF) + (sum >> 16);
            }
            return ~sum;
        };
    };
}; // namespace be20

#endif
