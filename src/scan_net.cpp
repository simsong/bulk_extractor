/**
 * nework scanner for bulk_extractor.
 * "ip carving".
 * Developed by Rob Beverly, based on a suggestion by Simson Garfinkel.
 * 2011-sep-13 - slg - modified to add packet carving
 */

/**
 * The software provided here is released by the Naval Postgraduate
 * School, an agency of the U.S. Department of Navy.  The software
 * bears no warranty, either expressed or implied. NPS does not assume
 * legal liability nor responsibility for a User's use of the software
 * or the results of such use.
 *
 * Please note that within the United States, copyright protection,
 * under Section 105 of the United States Code, Title 17, is not
 * available for any work of the United States Government and/or for
 * any works created by United States Government employees. User
 * acknowledges that this software contains work which was created by
 * NPS government employees and is therefore in the public domain and
 * not subject to copyright.
 */


#include "bulk_extractor.h"

#include <set>
#include <tr1/unordered_set>

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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



/****************************************************************/

#ifdef HAVE_PCAP_PCAP_H
#ifdef HAVE_DIAGNOSTIC_REDUNDANT_DECLS
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
#endif

#ifndef DLT_EN10MB
#define DLT_EN10MB	1	/* Ethernet (10Mb) */
#endif

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN      6
#endif

#ifndef ETHER_HEAD_LEN
#define ETHER_HEAD_LEN      14
#endif

#define ETHERTYPE_IP        0x0800  /* IP protocol */
#define	ETHERTYPE_VLAN	    0x8100		/* IEEE 802.1Q VLAN tagging */
#define	ETHERTYPE_IPV6	    0x86dd		/* IP protocol version 6 */

#define PCAP_MAX_PKT_LEN    65535               // The longest a packet may be; longer values make wireshark refuse to load

int opt_report_chksum_bad= 0;			// if true, report bad chksums
static const char *default_filename = "packets.pcap";

/* packetset is a set of the addresses of packets that have been written.
 * It prevents writing the packets that are carved from a pcap file and then
 * carved again from raw ethernet carving.
 * 
 * Previously this was a set, but unordered_sets are faster.
 * It doesn't need to be mutex locked, because we do the tests for each thread.
 */
// This is now in be13_api/net_ethernet.h
//typedef struct ether_addr {
//    u_char octet[ETHER_ADDR_LEN];
//} ether_addr_t;
//typedef struct ether_addr ether_addr_t;

/* generic ip header for IPv4 and IPv6 packets */
typedef struct generic_iphdr {
    sa_family_t family;		/* AF_INET or AF_INET6 */	
    uint8_t src[16];		/* Source IP address; holds v4 or v6 */
    uint8_t dst[16];		/* Destination IP address; holds v4 or v6 */
    uint8_t ttl;			/* ttl from ip_hdr and hop_limit for ip6_hdr */
    uint8_t nxthdr;			/* nxt hdr type */
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

/****************************************************************/
/*
 * Private definitions for internet protocol version 6.
 * RFC 2460
 */

/*
 * IPv6 address
 */
#ifndef s6_addr
struct in6_addr {
    union {
	uint8_t   __u6_addr8[16];
	uint16_t  __u6_addr16[8];
	uint32_t  __u6_addr32[4];
    } __u6_addr;			/* 128-bit IP6 address */
};
#define s6_addr   __u6_addr.__u6_addr8
#endif

struct ip6_hdr {
	union {
		struct ip6_hdrctl {
			uint32_t ip6_un1_flow;	/* 20 bits of flow-ID */
			uint16_t ip6_un1_plen;	/* payload length */
			uint8_t  ip6_un1_nxt;	/* next header */
			uint8_t  ip6_un1_hlim;	/* hop limit */
		} ip6_un1;
		uint8_t ip6_un2_vfc;	/* 4 bits version, top 4 bits class */
	} ip6_ctlun;
	struct in6_addr ip6_src;	/* source address */
	struct in6_addr ip6_dst;	/* destination address */
} __attribute__((__packed__));
#define ip6_vfc		ip6_ctlun.ip6_un2_vfc
#define ip6_flow	ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen	ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt		ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim	ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops	ip6_ctlun.ip6_un1.ip6_un1_hlim

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



/* _TCPT_OBJECT; a windows connection state object as taken from
 *    Volatility
 */
struct tcpt_object {
    uint16_t junk1;
    uint16_t pool_size;
    uint32_t sig;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    struct be13::ip4_addr dst;
    struct be13::ip4_addr src;
    uint16_t dst_port;
    uint16_t src_port;
};


/* create a bulk_extractor specific tcp structure to avoid
   FAVOR_BSD type differences among systems */
struct be_tcphdr {
    uint16_t th_sport;
    uint16_t th_dport;
    uint32_t th_seq;
    uint32_t th_ack;
    unsigned int th_x2:4, th_off:4;
    uint8_t th_flags;
    uint16_t th_win;
    uint16_t th_sum;
    uint16_t th_urp;
};

struct be_udphdr {
    uint16_t uh_sport;
    uint16_t uh_dport;
    uint16_t uh_ulen;	
    uint16_t uh_sum;
};

#if 0
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#else
/* For defaults, assume we are on Intel, which is __LITTLE_ENDIAN */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#define __BIG_ENDIAN 2
#define __BYTE_ORDER 1
#endif

#ifndef AF_INET
#define AF_INET 2			// default on most systems
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

#ifndef IPPROTO_UDP
#define IPPROTO_UDP 17
#endif

//#ifdef _WIN32
//#define u_int8_t  unsigned __int8
//#define u_int16_t unsigned __int16
//#define u_int32_t unsigned __int32
//#endif

/* Structure of an internet header, naked of options. */
struct ip {
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t ip_v:4;		/* version */
    uint8_t ip_hl:4;		/* header length */
#else
    uint8_t ip_hl:4;		/* header length */
    uint8_t ip_v:4;		/* version */
#endif
    uint8_t ip_tos;			/* type of service */
    uint16_t ip_len;			/* total length */
    uint16_t ip_id;			/* identification */
    uint16_t ip_off;			/* fragment offset field */
#define	IP_RF 0x8000			/* reserved fragment flag */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
    uint8_t ip_ttl;			/* time to live */
    uint8_t ip_p;			/* protocol */
    uint16_t ip_sum;			/* checksum */
    uint32_t ip_src;
    uint32_t ip_dst;	/* source and dest address */
};
#endif
#endif	// if 0


/* Computes the checksum for IPv6 TCP, UDP and ICMPv6. 
 * 
 * TCP, UDP and ICMPv6 contain a checksum that is computed 
 * over an IP pseudo header and the entire L3 datagram. 
 * The IPv6 pseudo-header includes the following values in 	
 * the listed order:
 *	- Source address (16 octets)
 * 	- Destination address (16 octets)
 * 	- TCP Length (4 octets)
 *	- 24 zeros (3 octets)
 *	- Next Header (1 octet)
 * Total: 40 octets
 */
static uint16_t IPv6L3Chksum(const sbuf_t &sbuf, u_int chksum_byteoffset)
{
    const struct ip6_hdr *ip6 = sbuf.get_struct_ptr<struct ip6_hdr>(0);
    if(ip6==0) return 0;		// cannot compute; not enough data
	
    int len = ntohs(ip6->ip6_plen) + 40;/* payload len + size of pseudo hdr */
    uint32_t sum = 0;			// 
    u_int octets_processed = 0;

    ///* we initialize ipp to the beginning of the ipv6 src addr field
    //* which is followed by the ipv6 dst addr field and then the tcp
    //* payload
    //*/
    //const unsigned short *ipp = (unsigned short *)ip6->ip6_src.s6_addr16;
    //const unsigned short *ipp = (unsigned short *)ip6->ip6_src.s6_addr;

    /** We start counting at offset 8, which is the source address */
    for(size_t i=8; i+1<sbuf.bufsize && len>0 ;i+=2){
	if(i==40){			// reached the end of ipv6 header
	    sum += ip6->ip6_plen;
	    if(sum & 0x80000000){   /* if high order bit set, fold */
		sum = (sum & 0xFFFF) + (sum >> 16);
	    }

	    /* putting the nxt pseudo header field in network-byte order via SHL(8) */
	    sum += (uint16_t)(ip6->ip6_nxt << 8);			
	    if(sum & 0x80000000){   /* if high order bit set, fold */
		sum = (sum & 0xFFFF) + (sum >> 16);
	    }
	    len -= 8; 	    /* we've processed 8 octets in the pseudo header */
	    octets_processed += 8;
	} else {
	    /* check if we are at the offset of the L3 chksum field*/
	    if ((octets_processed != chksum_byteoffset)) {
		sum += sbuf.get16u(i);
		if(sum & 0x80000000){   /* if high order bit set, fold */
		    sum = (sum & 0xFFFF) + (sum >> 16);
		}
	    }

	    /* update len and octets_processed */
	    len -= 2;
	    octets_processed += 2;
	}
    }
    while( (sum & 0xFFFF0000) > 0 ){
	sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}
/* compute an Internet-style checksum, from Stevens */
static uint16_t cksum(const struct be13::ip4 * const ip, int len) 
{
    long sum = 0;  /* assume 32 bit long, 16 bit short */
    const uint16_t *ipp = (uint16_t *) ip;
    int octets_processed = 0;

    while (len > 1) {
        if (octets_processed != 10) {
            sum += *ipp;
            if(sum & 0x80000000)   /* if high order bit set, fold */
                sum = (sum & 0xFFFF) + (sum >> 16);
        }
        ipp++;
        len -= 2;
        octets_processed+=2;
    }

    if (len)       /* take care of left over byte */
        sum += *ipp;

    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return ~sum;
}

/* determine if an integer is a power of two; used for the TTL */
static bool isPowerOfTwo(const uint8_t val) 
{
    /* early reject */
    if ( (val % 2 != 0) && (val != 255) )
        return false;
    if ( (val == 32) || (val == 64) || (val == 128) || (val == 255) )
        return true;
    return false;
}

/* test for obviously bogus Ethernet addresses (heuristic) */
static bool invalidMAC(const be13::ether_addr *const e) 
{
    int zero_octets = 0;
    int ff_octets = 0;
    for (int i=0;i<ETHER_ADDR_LEN;i++) {
        if (e->ether_addr_octet[i] == 0x00)
            zero_octets++;
        if (e->ether_addr_octet[i] == 0xFF)
            ff_octets++;
        if ( (zero_octets > 1) || (ff_octets > 1) )
            return true;
    }
    return false;
}

/* test for obviously bogus IPv4 addresses (heuristics) */
static bool invalidIP4(const uint8_t *const cc)
{
    /* Leading zero or 0xff */
    if ( (cc[0] == 0) || (cc[0] == 255) ){
        return true;
    }
    /* IANA Reserved http://www.iana.org/assignments/ipv4-address-space/ipv4-address-space.txt */
    if (cc[0] == 127) {
        return true;
    }
    if (cc[0] >= 224) {
        return true;
    }
    /* Sequences of middle 0x0000 or 0xffff */
    if ( (cc[1] == 0) && (cc[2] == 0) ) {
        return true;
    }
    if ( (cc[1] == 255) || (cc[2] == 255) ) {
        return true;
    }
    /* Trailing zero or 0xff */
    if ( (cc[3] == 0) || (cc[3] == 255) ) {
        return true;
    }
    if( (cc[0]==cc[1]) && (cc[1]==cc[2]) ) {
        return true;
    }
    return false;
}

static bool invalidIP6(const uint16_t addr[8])
{
    /* IANA Reserved http://www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xml 
     * We define valid addresses as IPv6 addresses of type Link Local Unicast (FE80::/10), 
     * Multicast (FF00::/8), or Global Unicast (2000::/3).  Any other address is invalid.
     */

    /* compare values in network order */
    if ( (addr[0] & 0x00E0) == 0x0020 ) return false; /* Global Unicast */
    if ( (addr[0] & 0x00FF) == 0x00FF ) return false; /* Multicast */
    if ( (addr[0] & 0xC0FF) == 0x80FE ) return false; /* Link Local Unicast */
    return true;
}


static bool invalidIP(const uint8_t addr[16], sa_family_t family) {	
    switch (family) {
    case AF_INET:
	return invalidIP4(addr+12);
	break;
    case AF_INET6:
	return invalidIP6(reinterpret_cast<const uint16_t *>(addr));
	break;
    default:
	return false;
    }
}

/* Additional versions to avoid casting */
//inline bool invalidIP(const in_addr *a)
//{
//    return invalidIP4((const unsigned char *)a);
//}
//static string ip2string(const unsigned int *const i)
//{
//    return ip2string((const struct in_addr *)i);
//}
//#endif

//static bool invalidIP(const struct be13::ip4_addr *const a) 
//{
//    return invalidIP4((const uint8_t *) &(a->s_addr));
//}

static string ip2string(const struct be13::ip4_addr *const a)
{
    const uint8_t *b = (const uint8_t *)a;

    char buf[1024];
    snprintf(buf,sizeof(buf),"%d.%d.%d.%d",b[0],b[1],b[2],b[3]);
    return std::string(buf);
    //string res(inet_ntoa(*a));
    //return res;
}

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 256
#endif

#ifdef _WIN32
const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
    return("inet_ntop win32");
}
#endif


static string ip2string(const uint8_t *addr, sa_family_t family)
{
    char printstr[INET6_ADDRSTRLEN+1];
    switch (family) {
    case AF_INET: 
	return string(inet_ntop(family,reinterpret_cast<const struct in_addr *>(addr+12), printstr, sizeof(printstr)));
    case AF_INET6: 
	return string(inet_ntop(family, addr, printstr, sizeof(printstr)));
    }
    return string("INVALID family ");
}

static string mac2string(const struct be13::ether_addr *const e)
{
    char addr[32];
    snprintf(addr,sizeof(addr),"%02X:%02X:%02X:%02X:%02X:%02X",
	     e->ether_addr_octet[0], e->ether_addr_octet[1], e->ether_addr_octet[2],
	     e->ether_addr_octet[3], e->ether_addr_octet[4], e->ether_addr_octet[5]);
    string res(addr);
    return res;
}

static string i2str(const int i)
{
    std::stringstream  ss;
    ss << i;
    return ss.str();
}

/* primitive port heuristics */
static bool sanePort(const uint16_t port) {
    static const uint16_t ports[] = {80, 443, 53, 25, 110, 143, 993, 587, 23, 22, 21, 20, 119, 123};
    static const uint16_t ports_len = sizeof(ports) / sizeof(uint16_t);

    for (int i=0; i<ports_len; i++) {
        if (port == ntohs(ports[i]))
            return true;
    }
    return false;
}

/** Sanity-check an IP packet header.
 * Return false if it looks insane, true if it looks sane
 * @param sbuf - the location of the header
 * @param checksum_valid - set TRUE if checksum is valid, FALSE of it is not.
 * @param h - set with the generic header that is extracted.
 *
 * This is called twice for every byte of the header, so we might want to cache
 * the results, but currently we don't.
 */
static bool sanityCheckIPHeader(const sbuf_t &sbuf, bool *checksum_valid, generic_iphdr_t *h)
{
    const struct be13::ip4 *ip = sbuf.get_struct_ptr<struct be13::ip4>(0);
    if (ip && ip->ip_v == 4){
	if (ip->ip_hl != 5) return false;	// IPv4 header length is 20 bytes (5 quads)
	if ( (ip->ip_off != 0x0) && (ip->ip_off != ntohs(IP_DF)) ) return false;
	
	// only do TCP and UDP
	if ( (ip->ip_p != IPPROTO_TCP) && (ip->ip_p != IPPROTO_UDP) ) return false;

	/* reject anything larger than a jumbo gram or smaller than min-size IP */
	if ( (ntohs(ip->ip_len) > 8192) || (ntohs(ip->ip_len) < 28) ) return false;

    	(*checksum_valid) = (ip->ip_sum == cksum(ip, ip->ip_hl * 4));
	
	/* create a generic_iphdr_t, similar to tcpip.c from tcpflow code */
	h->family = AF_INET;

	/* similar to tcpip.c from tcpflow code */
	uint32_t src[4] = {0, 0, 0, 0};
	uint32_t dst[4] = {0, 0, 0, 0};
//#ifdef _WIN32
//	src[3] = ip->ip_src;
//	dst[3] = ip->ip_dst;
//#else
	memcpy(&src[3],&ip->ip_src.addr,4);
	memcpy(&dst[3],&ip->ip_dst.addr,4);
//#endif
	memcpy(h->src, src, sizeof(src));
	memcpy(h->dst, dst, sizeof(dst)); 
	h->ttl = ip->ip_ttl;
	h->nxthdr = ip->ip_p;
	h->nxthdr_offs = (ip->ip_hl * 4);
	h->payload_len = (ntohs(ip->ip_len) - (ip->ip_hl * 4));
	return true;
    } 

    const struct be13::ip6_hdr *ip6 = sbuf.get_struct_ptr<struct be13::ip6_hdr>(0);
    if (ip6 && ((ip6->ip6_vfc & 0xF0) == 0x60)){
		
	//only do TCP, UDP and ICMPv6
	if ( (ip6->ip6_nxt != IPPROTO_TCP) && 
	     (ip6->ip6_nxt != IPPROTO_UDP) &&
	     (ip6->ip6_nxt != IPPROTO_ICMPV6) )
	    return false;

	uint16_t ip_payload_len = ntohs(ip6->ip6_plen);

	/* Reject anything larger than a jumbo gram or smaller than the 
	 * minimum size TCP, UDP or ICMPv6 packet (i.e. just header, no payload 
	 */
	if ( (ip_payload_len > 8192) ||
	     ((ip6->ip6_nxt == IPPROTO_TCP) && (ip_payload_len < 20)) ||
	     ((ip6->ip6_nxt == IPPROTO_UDP) && (ip_payload_len < 8)) ||
	     ((ip6->ip6_nxt == IPPROTO_ICMPV6) && (ip_payload_len < 4)) )
	    return false;

	switch (ip6->ip6_nxt) {
	default:
	case IPPROTO_TCP:
	    {
	    const struct be_tcphdr *tcp = sbuf.get_struct_ptr<struct be_tcphdr>(40);
	    if(!tcp) return false;	// not sufficient room

	    /* tcp chksum is at byte offset 16 from tcp hdr + 40 w/ pseudo hdr */
	    (*checksum_valid) = (tcp->th_sum == IPv6L3Chksum(sbuf, 56));
	    break; 
	    } 
	case IPPROTO_UDP:
	    {
	    const struct be_udphdr *udp = sbuf.get_struct_ptr<struct be_udphdr>(40);
	    if(!udp) return false;	// not sufficient room

	    /* udp chksum is at byte offset 6 from udp hdr + 40 w/ pseudo hdr */
	    (*checksum_valid) = (udp->uh_sum == IPv6L3Chksum(sbuf, 46));
	    break; 
	    }
	case IPPROTO_ICMPV6:
	    {
	    const struct icmp6_hdr *icmp6 = sbuf.get_struct_ptr<struct icmp6_hdr>(40);
	    if(!icmp6) return false;	// not sufficient room

	    /* icmpv6 chksum is at byte offset 2 from icmpv6 hdr + 40 w/ pseudo hdr */
	    (*checksum_valid) = (icmp6->icmp6_cksum == IPv6L3Chksum(sbuf, 42));
	    break;
	    }
	}
	/* create a generic_iphdr_t, similar to tcpip.c from tcpflow code */
	h->family = AF_INET6;
	memcpy(h->src, ip6->ip6_src.__u6_addr.__u6_addr8, sizeof(ip6->ip6_src.__u6_addr.__u6_addr8));
	memcpy(h->dst, ip6->ip6_dst.__u6_addr.__u6_addr8, sizeof(ip6->ip6_dst.__u6_addr.__u6_addr8));
	h->ttl = ip6->ip6_hlim;
	h->nxthdr = ip6->ip6_nxt;
	h->nxthdr_offs = 40; 	/* ipv6 headers are a fixed length of 40 bytes */
	h->payload_len = ntohs(ip6->ip6_plen);
	return true;
    }
    return false;			// right now we only do IPv4 and IPv6
}

/* mutex for writing packets.
 * This is not in the class because it will be accessed by multiple threads.
 */
static pthread_mutex_t M;
static FILE *fcap = 0;		// capture file, protected by M

/*
 * Look at the sbuf and see if it beings with a packet.
 * If it does, write it to fcap.
 * Return the length of the packet that was written.
 * Currently we assume that a packet is valid if the next packet is valid.
 * This means we won't get the last packet.
 * We assume that a pcap packet is valid if the timestamp and size are sane.
 */
 
const uint32_t jan1_1990 = 631152000;
const uint32_t jan1_2020 = 1577836800;
const uint32_t max_packet_len = 65535;
const uint32_t min_packet_size = 20;		// don't bother with ethernet packets smaller than this

/*
 * Sanity check the header values
 */
static bool   likely_valid_pcap_header(const sbuf_t &sbuf)
{
    uint32_t seconds = sbuf.get32u(0);
    uint32_t useconds = sbuf.get32u(4);
    uint32_t cap_len = sbuf.get32u(8);
    uint32_t pkt_len = sbuf.get32u(12);

    if(seconds!=0 && (seconds<jan1_1990 || seconds>jan1_2020)) return false;
    if(useconds>1000000) return false;
    if(cap_len<min_packet_size || cap_len>max_packet_len) return false;
    if(pkt_len<min_packet_size || pkt_len>max_packet_len) return false;
    if(cap_len > pkt_len) return false;
    return true;
}

/*
 * Currently this will not write out a truncated packet.
 */
static const size_t PCAP_RECORD_HEADER_SIZE = 16;
class packet_carver {
private:
    packet_carver(const packet_carver &pc):fs(pc.fs),ps(pc.ps),ip_recorder(pc.ip_recorder),tcp_recorder(pc.tcp_recorder),ether_recorder(pc.ether_recorder){
    }
    packet_carver &operator=(const packet_carver &that){
	return *this;			// no-op
    }
public:
    typedef std::tr1::unordered_set<const void *> packetset;
    feature_recorder_set &fs;
    packetset ps;
    feature_recorder *ip_recorder;
    feature_recorder *tcp_recorder;
    feature_recorder *ether_recorder;

    packet_carver(const class scanner_params &sp):
	fs(sp.fs),ps(),ip_recorder(0),tcp_recorder(0),ether_recorder(0){
	ip_recorder = fs.get_name("ip");
	tcp_recorder = fs.get_name("tcp");
	ether_recorder = fs.get_name("ether");
    }


private:
    /* 
     * According to 'man pcap-savefile', you need to implement this file format,
     * but there are no functions to do so.
     * 
     * pcap_write_bytes writes bytes; pcap accomidates.
     * pcap_write2 writes a 2-byte value in native byte order; pcap accomidates.
     * pcap_write4 writes a 4-byte value in native byte order; pcap accomidates.
     * pcap_writepkt writs a packet
     */
    void pcap_write_bytes(const uint8_t * const val, size_t num_bytes) {
        size_t count = fwrite(val,1,num_bytes,fcap);
        if (count != num_bytes) {
            err(1, "scanner scan_net is unable to write to file %s", default_filename);
        }
    }
    void pcap_write2(const uint16_t val) {
        size_t count = fwrite(&val,1,2,fcap);
        if (count != 2) {
            err(1, "scanner scan_net is unable to write to file %s", default_filename);
        }
    }
    void pcap_write4(const uint32_t val) {
        size_t count = fwrite(&val,1,4,fcap);
        if (count != 4) {
            err(1, "scanner scan_net is unable to write to file %s", default_filename);
        }
    }

public:
    void pcap_writepkt(const uint32_t seconds,const uint32_t useconds,
		       const size_t cap_len,
		       const size_t pkt_len,
		       const sbuf_t &sbuf,const size_t offset,
                       const bool add_frame,
                       const uint16_t frame_type) {
	// Make sure that neither this packet nor an encapsulated version of this packet has been written
	//if(ps.find(pkt)==ps.end() && ps.find(pkt-4)==ps.end()){
	//ps.insert(pkt);
	    pthread_mutex_lock(&M);		// lock the mutex
	    if(fcap==0){
		string ofn = ip_recorder->outdir+"/" + default_filename;
		fcap = fopen(ofn.c_str(),"wb"); // write the output
		pcap_write4(0xa1b2c3d4);
		pcap_write2(2);			// major version number
		pcap_write2(4);			// minor version number
		pcap_write4(0);			// time zone offset; always 0
		pcap_write4(0);			// accuracy of time stamps in the file; always 0
		pcap_write4(PCAP_MAX_PKT_LEN);	// snapshot length
		pcap_write4(DLT_EN10MB);	// link layer encapsulation
	    }

            size_t forged_header_len = 0;
            // if requested, forge an Ethernet II header and prepend it to the packet so raw packets can
            // coexist happily in an ethernet pcap file.  Don't do this if the resulting length would make
            // the pcap file invalid.
            bool add_frame_and_safe = add_frame && cap_len + ETHER_HEAD_LEN <= PCAP_MAX_PKT_LEN;
            uint8_t forged_header[ETHER_HEAD_LEN];
            if(add_frame_and_safe) {
                forged_header_len = sizeof(forged_header);
                // forge Ethernet II header
                //   - source and destination addrs are all zeroes, ethernet type is supplied by function caller
                memset(forged_header, 0x00, sizeof(forged_header));
                // final two bytes of header hold the type value
                forged_header[sizeof(forged_header)-2] = (uint8_t) (frame_type >> 8);
                forged_header[sizeof(forged_header)-1] = (uint8_t) frame_type;
            }

	    /* Write a packet */
	    pcap_write4(seconds);		// time stamp, seconds avalue
	    pcap_write4(useconds);		// time stamp, microseconds
	    pcap_write4(cap_len + forged_header_len);
	    pcap_write4(pkt_len + forged_header_len);
            if(add_frame_and_safe) {
                pcap_write_bytes(forged_header, sizeof(forged_header));
            }
	    sbuf.write(fcap,offset,cap_len);	// the packet
	    pthread_mutex_unlock(&M);		// lock the mutex
	    //}
    }

    /**
     * Validate and write a pcap packet. Return the number of bytes written.
     */
    size_t carvePCAPPacket(const sbuf_t &sb2) {
	if(sb2.bufsize < PCAP_RECORD_HEADER_SIZE) return 0;
	uint32_t seconds  = sb2.get32u(0);  if(seconds==0) return 0;
	uint32_t useconds = sb2.get32u(4); 
	uint32_t cap_len  = sb2.get32u(8);  if(cap_len<min_packet_size) return 0;
	uint32_t pkt_len  = sb2.get32u(12); if(pkt_len<min_packet_size) return 0;
	
	if(sb2.bufsize < PCAP_RECORD_HEADER_SIZE+cap_len) return 0; // packet was truncated
	   
	if(likely_valid_pcap_header(sb2)){
	    /* If buffer is the size of the record,
	     * or if the next header looks good,
	     * then carve the packet.
	     */
	    if((sb2.bufsize==cap_len+PCAP_RECORD_HEADER_SIZE) ||
	       likely_valid_pcap_header(sb2+PCAP_RECORD_HEADER_SIZE+cap_len)){

                // If it looks like the pcap record begins with an IP header rather than a link-level frame,
                // tell writepkt what kind of header so it can create a valid pseudo Ethernet II header

                // assume IPv4, but this field is only relevant if the header looks sane (is_raw_ip is true)
                uint16_t pseudo_frame_ethertype = ETHERTYPE_IP;
                bool checksum_valid = false; // ignored
                generic_iphdr_t header_info;
                bool is_raw_ip = sanityCheckIPHeader(sb2+PCAP_RECORD_HEADER_SIZE, &checksum_valid, &header_info);
                if(header_info.family == AF_INET6) {
                    pseudo_frame_ethertype = ETHERTYPE_IPV6;
                }
		
		/* We are at the end of the file, or the next slot is also a packet */
		pcap_writepkt(seconds,useconds,cap_len,pkt_len,sb2,PCAP_RECORD_HEADER_SIZE,is_raw_ip,pseudo_frame_ethertype);
		return 16+cap_len;
	    }
	}
	return 0;
    }
    
    size_t carvePCAPFile(const sbuf_t &sb2) {
	/* If this is a pcap file, write it out. 
	 *
	 * Currently we just remember the packets themselves,
	 * which may cause issues if there is encapsulation present
	 */
	size_t len = 0;
	if(sb2[0]==0xd4 && sb2[1]==0xc3 && sb2[2]==0xb2 && sb2[3]==0xa1 && // magic
	   sb2[4]==0x02 && sb2[5]==0x00 && sb2[6]==0x04 && sb2[7]==0x00){ // version_major, version_minor
	    ip_recorder->write(sb2.pos0,"0xd4,0xc3,0xb2,0xa1","TCPDUMP file");
	    /* now scan for packets */
	    len = 24;
	    while(len<sb2.pagesize && len<sb2.bufsize){
		size_t len2 = carvePCAPPacket(sb2+len);
		if(len2==0) break;
		len += len2;
	    }
	}
	return len;
    }

    /* Test for a possible struct ip <netinet/ip.h> or struct ip6_hdr <netinet/ip6.h>
     * Please remember this is called for every byte of the disk image,
     * so it needs to be as fast as possible.
     */
    static string chksum_ok;
    static string chksum_bad;
    size_t carveStructIP(const sbuf_t &sb2) {
	generic_iphdr_t h;
	bool checksum_valid = false; 
	
	/* check if it looks like ipv4 or ipv6 packet 
	 * if neither, return false 
	 */
	if (!sanityCheckIPHeader(sb2, &checksum_valid, &h)) return false;

	if (!invalidIP(h.src, h.family) && !invalidIP(h.dst,h.family)){
	    /* based on the TTL, infer whether remote or local */
	    if(h.family==AF_INET || (h.family==AF_INET6 && checksum_valid)){
		if(checksum_valid || opt_report_chksum_bad){
		    const string &chksum_status = checksum_valid ? chksum_ok : chksum_bad;
		    string src;
		    string dst;
		    
		    if(isPowerOfTwo(h.ttl)){
			src = "L";	// src is local because the power of two hasn't been decremented
			dst = "R";	// dst is remote because the power of two hasn't been decremented
		    } else {
			src = "R";	// src is remote
			dst = "L";	// dst is local
		    }
		    const string name = (h.family==AF_INET) ? "ip " : "ip6_hdr ";
		    
		    ip_recorder->write(sb2.pos0, ip2string(h.src, h.family),
				       "struct " + name + src + " (src) " + chksum_status);
		    ip_recorder->write(sb2.pos0, ip2string(h.dst, h.family),
				       "struct " + name + dst + " (dst) " + chksum_status);
		}
	    }
	}
	
	// Currently those below just generate too many false positives for IPv6, so we ignore them
	if(h.family==AF_INET6) return 0;
	if (h.nxthdr == IPPROTO_TCP) {
	    const struct be_tcphdr *tcp = sb2.get_struct_ptr<struct be_tcphdr>(h.nxthdr_offs);
	    if(tcp){
		tcp_recorder->write(sb2.pos0,
				    ip2string(h.src, h.family) + ":" + i2str(ntohs(tcp->th_sport)) + " -> " +
				    ip2string(h.dst, h.family) + ":" + i2str(ntohs(tcp->th_dport)) + " (TCP)",
				    " Size: " + i2str(h.payload_len+h.nxthdr_offs)
				    );
		return sizeof(*tcp)+20;
	    }
	}
	
	if (h.nxthdr == IPPROTO_UDP) {
	    const struct be_udphdr *udp = sb2.get_struct_ptr<struct be_udphdr>(h.nxthdr_offs);
	    if(udp){
		tcp_recorder->write(sb2.pos0, 
				    ip2string(h.src, h.family) + ":" + i2str(ntohs(udp->uh_sport)) + " -> " +
				    ip2string(h.dst, h.family) + ":" + i2str(ntohs(udp->uh_dport)) + " (UDP)",
				    " Size: " + i2str(h.payload_len+h.nxthdr_offs)			
				    );
		return sizeof(*udp)+20;
	    }
	}
	
	if (h.nxthdr == IPPROTO_ICMPV6) {
	    const struct icmp6_hdr *icmp6 = sb2.get_struct_ptr<struct icmp6_hdr>(h.nxthdr_offs);
	    if(icmp6){
		tcp_recorder->write(sb2.pos0,
				    ip2string(h.src, h.family) + " -> " + 
				    ip2string(h.dst, h.family) + " (ICMPv6)",
				    " Type: " + i2str(icmp6->icmp6_type) + " Code: " + i2str(icmp6->icmp6_code)
				    );
		return sizeof(*icmp6);
	    }
	}
	return 0;
    }

    /* Test for a possible sockaddr_in <netinet/in.h> 
     * Please remember that this is called for every byte, so it needs to be fast.
     */
    size_t carveSockAddrIn(const sbuf_t &sb2) {
	const struct sockaddr_in *in = sb2.get_struct_ptr<struct sockaddr_in>(0);
	if(in==0) return 0;

	/* Note that the sin_len member of the sockaddr_in struct is optional
	 * and not supported by all vendors (Stevens, Network Programming pp.58-59.)
	 * Therefore, look for the AF_INET in either the first or second octet
	 */
	if (
#ifdef HAVE_SOCKADDR_IN_SIN_LEN
	    (in->sin_len != AF_INET) &&
#endif
	    (in->sin_family != AF_INET) ){
	    return false;
	}
	
	/* Ensure that sin_zero is all zeros... */
	if(in->sin_zero[0]!=0 || in->sin_zero[1]!=0 || in->sin_zero[2]!=0 || in->sin_zero[3]!=0 || 
	   in->sin_zero[4]!=0 || in->sin_zero[5]!=0 || in->sin_zero[6]!=0 || in->sin_zero[7]!=0){
	    return 0;
	}

	/* Weed out any obviously bad IP addresses */
	if (invalidIP4((const uint8_t *)&(in->sin_addr))) return 0;
	
	/* Only use candidate with ports we believe most likely */
	if (!sanePort(in->sin_port)) return 0;
	
	ip_recorder->write(sb2.pos0, ip2string((const be13::ip4_addr *)&(in->sin_addr)), "sockaddr_in");
	return sizeof(struct sockaddr_in);
    }

    /* Test for possible _TCPT_OBJECT
     * Please remember that this is called for every byte, so it needs to be fast.
     */
    size_t carveTCPTOBJ(const sbuf_t &sb2){
	//const uint8_t *buf = sbuf.buf+i;
	//size_t buflen = sbuf.bufsize-i;
	//if(buflen < sizeof(struct tcpt_object)) return false;
	const struct tcpt_object *to = sb2.get_struct_ptr<struct tcpt_object>(0);
	if(to==0) return false;
	
	/* 0x54455054 == "TCPT" */
	if ( (to->sig == htonl(0x54435054)) && (to->pool_size == htons(0x330A)) ) {
	    ip_recorder->write( sb2.pos0, ip2string(&(to->src)), "tcpt");
	    ip_recorder->write( sb2.pos0, ip2string(&(to->dst)), "tcpt");
	    tcp_recorder->write(sb2.pos0, ip2string(&(to->src)) + ":" + i2str(ntohs(to->src_port)) + " -> " +
				ip2string(&(to->dst)) + ":" + i2str(ntohs(to->dst_port)) + " (TCPT)",
				""
				);
	    return sizeof(struct tcpt_object);
	}
	return 0;
    }
    
    /* Test for Ethernet link-layer MACs
     * Returns the size of the object carved
     */
    size_t carveEther(const sbuf_t &sb2,bool write_pcap)    {
	const struct macip *er = sb2.get_struct_ptr<struct macip>(0);
	if(er){
	    if ( (er->ether_type != htons(ETHERTYPE_IP)) &&  // 0x0800 
		 (er->ether_type != htons(ETHERTYPE_IPV6)) ){ // 0x86dd
		return 0;
	    }
	    if ( (er->ipv != 0x45) && ((er->ipv & 0xF0) != 0x60) ){ // ipv4 and ipv6
		return 0;
	    }
	}

	//const uint8_t *buf = sbuf.buf+i;
	//size_t buflen = sbuf.bufsize-i;
	size_t data_offset = (2*ETHER_ADDR_LEN)+sizeof(uint16_t);

	if(data_offset < sb2.bufsize){
	    bool checksum_valid = false;
	    generic_iphdr_t h;
	    if (sanityCheckIPHeader(sb2+data_offset, &checksum_valid, &h) && checksum_valid) {
		if (!invalidMAC(&(er->ether_dhost))){
		    ether_recorder->write(sb2.pos0, mac2string(&(er->ether_dhost)), " (ether_dhost) ");
		}
		if (!invalidMAC(&(er->ether_shost))){
		    ether_recorder->write(sb2.pos0, mac2string(&(er->ether_shost)), " (ether_shost) ");
		}
	    }
	}
	if(write_pcap){
	    /* Possibly a valid ethernet frame but not preceeded by a pcap_record_header
	     * (otherwise it would have been written and skipped...)
	     * Write it out with time of 1.
	     */
	    generic_iphdr_t h;
	    bool checksum_valid = false;
	    /* the IP pkt starts after the Ethernet header, 14 byte offset */
	    if (sanityCheckIPHeader(sb2+14, &checksum_valid, &h)){
		if(checksum_valid){
		    ssize_t ip_payload_len = h.payload_len;
		    ssize_t ip_header_len  = h.nxthdr_offs;
		    ssize_t packet_len     = ip_header_len + ip_payload_len + 14; 
		    if(packet_len > (ssize_t)sb2.bufsize) packet_len = sb2.bufsize;
		    if(packet_len > 0 ){
			pcap_writepkt(1,0,packet_len,packet_len,sb2,0,false,0x0000);
			return packet_len;
		    }
		}
	    }
	}
	return 0;
    }

    void carve(const sbuf_t &sbuf){
	if(sbuf.bufsize<16) return;		// no sense
	/* Scan through every byte of the buffer for all possible packets
	 * If we find a pcap file or a packet at the present location,
	 * don't bother with the remainder.
	 *
	 * Please remember that this is called for every byte, so it needs to be fast.
	 */
	for(u_int i=0 ; i<sbuf.pagesize && i<sbuf.bufsize;){
	    const sbuf_t sb2 = sbuf+i;
	    size_t sfile = carvePCAPFile( sb2 );
	    if(sfile>0) { i+= sfile;continue;}
	    size_t spacket = carvePCAPPacket( sb2 );
	    if(spacket>0) { i+= spacket; continue;}

	    size_t maxed = 0;
	    maxed = max(maxed,carveStructIP( sb2));
	    maxed = max(maxed,carveSockAddrIn( sb2 ));
	    maxed = max(maxed,carveTCPTOBJ( sb2 ));
	    maxed = max(maxed,carveEther( sb2,true ));
	    if(maxed){
		i+= maxed;
	    } else {
		i+=1;
	    }
	}
    };
};

string packet_carver::chksum_ok("cksum-ok");
string packet_carver::chksum_bad("cksum-bad");


extern "C"
void scan_net(const class scanner_params &sp,const recursion_control_block &rcb)
{
    assert(sp.sp_version==scanner_params::CURRENT_SP_VERSION);      
    if(sp.phase==scanner_params::startup){
        assert(sp.info->si_version==scanner_info::CURRENT_SI_VERSION);
	assert(sizeof(struct be13::ip4)==20);	// we've had problems on some systems
	sp.info->name  = "net";
        sp.info->author         = "Simson Garfinkel and Rob Beverly";
        sp.info->description    = "Scans for IP packets";
        sp.info->scanner_version= "1.0";

	// no longer disabled by default!
	//sp.info->flags = scanner_info::SCANNER_DISABLED; // really slow!
	sp.info->feature_names.insert("ip");
	sp.info->feature_names.insert("tcp");
	sp.info->feature_names.insert("ether");

	/* changed the pattern to be the entire feature,
	 * since histogram was not being created with previous pattern
	 */
	sp.info->histogram_defs.insert(histogram_def("ip",   "","cksum-ok","histogram"));
	sp.info->histogram_defs.insert(histogram_def("tcp",  "","histogram"));
	sp.info->histogram_defs.insert(histogram_def("ether","([^\(]+)","histogram"));

	/* scan_net has its own output as well */
	pthread_mutex_init(&M,NULL);
	return;
    }
    if(sp.phase==scanner_params::shutdown){
	if(fcap) fclose(fcap);
	return;
    }
    if(sp.phase==scanner_params::scan){
	packet_carver carver(sp);
	carver.carve(sp.sbuf);
    }
}
