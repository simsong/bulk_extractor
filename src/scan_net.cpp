/**
 * nework scanner for bulk_extractor.
 * "ip carving".
 * Developed by Rob Beverly, based on a suggestion by Simson Garfinkel.
 * 2011-sep-13 - slg - modified to add packet carving
 * 2021-aug-13 - slg - refactored to separate the pcap writing from the carving.
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
 *
 * 2021 additions (C) Simson L. Garfinkel. See LICENSE.md for license.
 */


#include "config.h"

#include <set>
#include <mutex>
#include <ctype.h>

#include "be13_api/formatter.h"
#include "be13_api/utils.h"

#include "pcap_writer.h"
#include "scan_net.h"


#ifdef HAVE_PCAP_PCAP_H
#ifdef HAVE_DIAGNOSTIC_REDUNDANT_DECLS
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
#endif

int opt_report_checksum_bad= 0;		// if true, report bad chksums
int opt_report_packet_path = 0;         // if true, report packets to packets.txt

std::atomic<uint64_t> ctr1 = 0;
std::atomic<uint64_t> ctr2 = 0;

/* packetset is a set of the addresses of packets that have been written.
 * It prevents writing the packets that are carved from a pcap file and then
 * carved again from raw ethernet carving.
 *
 * Previously this was a set, but unordered_sets are faster.
 * It doesn't need to be mutex locked, because we do the tests for each thread.
 */

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


#  ifdef HAVE_DIAGNOSTIC_CAST_ALIGN
#    pragma GCC diagnostic ignored "-Wcast-align"
#  endif

scan_net_t::scan_net_t(const scanner_params &sp):
    pwriter(sp),
    ip_recorder(sp.named_feature_recorder("ip")),
    tcp_recorder(sp.named_feature_recorder("tcp")),
    ether_recorder(sp.named_feature_recorder("ether"))
{
}


/* compute an Internet-style checksum, from Stevens.
 * The ipchecksum is stored 10 bytes in, so do not include it.
 */
uint16_t scan_net_t::ip4_cksum(const sbuf_t &sbuf, size_t pos, size_t len)
{
    uint32_t  sum = 0;  /* assume 32 bit long, 16 bit short */

    for (size_t offset = 0; offset+pos < sbuf.bufsize && offset<len; offset+=2){
        if (offset != 10) {   // do not include the checksum field
            sum += sbuf.get16u_unsafe( pos+offset );
            if (sum & 0x80000000){   /* if high order bit set, fold */
                sum = (sum & 0xFFFF) + (sum >> 16);
	    }
        }
    }

    if (len % 2 != 0){
        sum += sbuf[ pos+len-1];     /* take care of left over byte */
    }

    /* Now add all of the 1st complements */
    while (sum>>16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}



/**
 * Computes the checksum for IPv6 TCP, UDP and ICMPv6.
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
 *
 * @param - sbuf - the sbuf in which the packet is
 * @param - pos  - the byte offset of the packet header
 * @param - chksum_byteoffset - from the start of the header, where the checksum is.
 */
uint16_t scan_net_t::IPv6L3Chksum(const sbuf_t &sbuf, size_t pos, u_int chksum_byteoffset)
{
    const struct ip6_hdr *ip6 = sbuf.get_struct_ptr_unsafe<struct ip6_hdr>(pos);

    int len      = ntohs(ip6->ip6_plen) + 40;/* payload len + size of pseudo hdr */
    uint32_t sum = 0;			//
    u_int octets_processed = 0;

    /** We start counting at offset 8, which is the source address
     *  which is followed by the ipv6 dst addr field and then the tcp
     * payload
     */
    for(size_t i=pos+8 ; pos+i+1 < sbuf.bufsize && len > 0 ; i+=2 ){
	if (i==40){			// reached the end of ipv6 header
	    sum += ip6->ip6_plen;
	    if (sum & 0x80000000){   /* if high order bit set, fold */
		sum = (sum & 0xFFFF) + (sum >> 16);
	    }

	    /* putting the nxt pseudo header field in network-byte order via SHL(8) */
	    sum += (uint16_t)(ip6->ip6_nxt << 8);
	    if (sum & 0x80000000){   /* if high order bit set, fold */
		sum = (sum & 0xFFFF) + (sum >> 16);
	    }
	    len -= 8; 	    /* we've processed 8 octets in the pseudo header */
	    octets_processed += 8;
	} else {
	    /* check if we are at the offset of the L3 chksum field*/
	    if ((octets_processed != chksum_byteoffset)) {
		sum += sbuf.get16u_unsafe(i);
		if (sum & 0x80000000){   /* if high order bit set, fold */
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

    return ~sum;			// return the complement of the checksum
}

/* determine if an integer is a power of two; used for the TTL */
bool scan_net_t::isPowerOfTwo(const uint8_t val)
{
    switch(val){
    case 32:
    case 64:
    case 128:
    case 255:
	return true;
    }
    return false;
}

/* test for obviously bogus Ethernet addresses (heuristic) */
bool scan_net_t::invalidMAC(const be13::ether_addr *const e)
{
    int zero_octets = 0;
    int ff_octets = 0;
    for ( int i=0;i<ETHER_ADDR_LEN;i++) {
        if (e->ether_addr_octet[i] == 0x00) zero_octets++;
        if (e->ether_addr_octet[i] == 0xFF) ff_octets++;
        if ( (zero_octets > 1) || (ff_octets > 1) ) return true;
    }
    return false;
}

/* test for obviously bogus IPv4 addresses (heuristics).
 * This could be tuned for better performance
 */
bool scan_net_t::invalidIP4( const uint8_t *const cc)
{
    /* Leading zero or 0xff */
    if ( (cc[0] == 0) || (cc[0] == 255) ){
        return true;
    }

    /* IANA Reserved http://www.iana.org/assignments/ipv4-address-space/ipv4-address-space.txt */
    if (cc[0] == 127) return true;
    if (cc[0] >= 224) return true;

    /* Sequences of middle 0x0000 or 0xffff */
    if ( (cc[1] == 0)   && (cc[2] == 0) ) return true;
    if ( (cc[1] == 255) && (cc[2] == 255) ) return true;

    /* Trailing zero or 0xff */
    if ( (cc[3] == 0) || (cc[3] == 255) ) return true;
    if ( (cc[0]==cc[1]) && (cc[1]==cc[2]) ) return true;    /* Palendromes; needed? */
    return false;
}

/*
 * Note addr must be in host order, not network order.
 */
bool scan_net_t::invalidIP6(const uint16_t addr[8])
{
    /* IANA Reserved https://www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xml
     * We define valid addresses as IPv6 addresses of type Link Local Unicast (FE80::/10),
     * Multicast (FF00::/8), or Global Unicast (2000::/3).  Any other address is invalid.
     */

    /* compare values in network order, becuase that's the way they appear in memory */
    if ( (addr[0] & 0x00E0) == 0x0020 ) return false; /* Global Unicast */
    if ( (addr[0] & 0xC0FF) == 0x80FE ) return false; /* Link Local Unicast */
    if ( (addr[0] & 0x00FF) == 0x00FF ){
        /* Multicast FF00
         * See https://www.iana.org/assignments/ipv6-multicast-addresses/ipv6-multicast-addresses.xhtml
         */
        if (addr[1]!=0 || addr[2]!=0 || addr[3]!=0 || addr[4]!=0 ) return true;
        if (addr[0] > 0x0FFF) return true; // anything other than FF0X: is not valid.
        return false;
    }
    return true;
}


bool scan_net_t::invalidIP(const uint8_t addr[16], sa_family_t family)
{
    switch (family) {
    case AF_INET:
	return invalidIP4(addr+12);
    case AF_INET6:
	return invalidIP6(reinterpret_cast<const uint16_t *>(addr));
    default:
	return false;
    }
}

#ifndef INET4_ADDRSTRLEN
#define INET4_ADDRSTRLEN 64
#endif
std::string scan_net_t::ip2string(const struct be13::ip4_addr *const a)
{
    const uint8_t *b = (const uint8_t *)a;

    char buf[INET4_ADDRSTRLEN];
    snprintf(buf,sizeof(buf),"%d.%d.%d.%d",b[0],b[1],b[2],b[3]);
    return std::string(buf);
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


std::string scan_net_t::ip2string(const uint8_t *addr, sa_family_t family)
{
    char printstr[INET6_ADDRSTRLEN+1];
    switch (family) {
    case AF_INET:
	return std::string(inet_ntop(family,reinterpret_cast<const struct in_addr *>(addr+12), printstr, sizeof(printstr)));
    case AF_INET6:
	return std::string(inet_ntop(family, addr, printstr, sizeof(printstr)));
    }
    return std::string("INVALID family ");
}

#ifndef MAC_ADDRSTRLEN
#define MAC_ADDRSTRLEN 256
#endif

std::string scan_net_t::mac2string(const struct be13::ether_addr *const e)
{
    char addr[MAC_ADDRSTRLEN];
    snprintf(addr,sizeof(addr),"%02X:%02X:%02X:%02X:%02X:%02X",
	     e->ether_addr_octet[0], e->ether_addr_octet[1], e->ether_addr_octet[2],
	     e->ether_addr_octet[3], e->ether_addr_octet[4], e->ether_addr_octet[5]);
    return std::string(addr);
}

std::string scan_net_t::i2str(const int i)
{
    return std::to_string(i);
}

/** Sanity-check an IP packet header.
 * Return false if it looks insane, true if it looks sane
 * @param sbuf - the location of the header
 * @param pos  - offset within the sbuf.
 * @param h - set with the generic header that is extracted and checksum validity
 * @return  true if IPv4 checksum is valid, or if IPv6 TCP, UDP, or ICMP checksum is valid.
 *
 * References:
 * http://answers.yahoo.com/question/index?qid=20080529062909AAAYN3X
 * http://en.wikipedia.org/wiki/Transmission_Control_Protocol#TCP_checksum_for_IPv6
 */
bool scan_net_t::sanityCheckIP46Header(const sbuf_t &sbuf, size_t pos, scan_net_t::generic_iphdr_t *h, sanityCache_t *sc)
{
    if (sbuf.get8u_unsafe(pos)==69) {           // v4 20 bytes
        const struct be13::ip4 *ip = sbuf.get_struct_ptr_unsafe<struct be13::ip4>( pos );
        //ctr1 += 1;
        if (ip->ip_v == 4){                     // ipv4 packet
            if (ip->ip_hl != 5) return false;	// IPv4 header length is 20 bytes (5 quads) (ignores options)
            //if (sc && sc->find(pos)!=sc->end()) return true; // it's ipv4 and in the cache

            if ( (ip->ip_off != 0x0) && (ip->ip_off != ntohs(IP_DF)) ) return false;

            // only do TCP and UDP
            if ( (ip->ip_p != IPPROTO_TCP) && (ip->ip_p != IPPROTO_UDP) ) return false;

            /* reject anything larger than a jumbo gram or smaller than min-size IP */
            if ( (ip->ip_len() > 8192) || (ip->ip_len() < 28) ) return false;

            /* Reject if not enough space in the sbuf. This allows checksum computation to be unsafe */
            if (pos + ip->ip_len() > sbuf.bufsize) return false;

            /* Validate the checksum */
            h->checksum_valid = (ip->ip_sum == ip4_cksum(sbuf, pos, ip->ip_hl * 4 ));

            /* create a generic_iphdr_t, similar to tcpip.c from tcpflow code */
            h->family = AF_INET;

            /* similar to tcpip.c from tcpflow code */
            uint32_t src[4] = {0, 0, 0, 0}; // bottom 32 bits of a 128-bit value
            uint32_t dst[4] = {0, 0, 0, 0};
            memcpy(&src[3], &ip->ip_src.addr, 4); // avoids the need to be quadbyte aligned.
            memcpy(&dst[3], &ip->ip_dst.addr, 4);
            memcpy(h->src, src, sizeof(src));
            memcpy(h->dst, dst, sizeof(dst));
            h->ttl = ip->ip_ttl;
            h->nxthdr = ip->ip_p;
            h->nxthdr_offs = (ip->ip_hl * 4);
            h->payload_len = (ip->ip_len() - (ip->ip_hl * 4));
            //if (sc) sc->insert(pos);
            return true;
        }
    }

    //ctr2 += 1;
    /* ipv6 attempt */
    const struct be13::ip6_hdr *ip6 = sbuf.get_struct_ptr_unsafe<struct be13::ip6_hdr>( pos );
    if ((ip6->ip6_vfc & 0xF0) == 0x60){
	//only do TCP, UDP and ICMPv6
	if ( (ip6->ip6_nxt != IPPROTO_TCP) &&
	     (ip6->ip6_nxt != IPPROTO_UDP) &&
	     (ip6->ip6_nxt != IPPROTO_ICMPV6) ) return false;

        if (sc && sc->find(pos)!=sc->end()) return true;
	uint16_t ip_payload_len = ntohs(ip6->ip6_plen);

        /* Make sure there is sufficient room in the sbuf */
        if (pos + ip_payload_len > sbuf.bufsize) return false;

	/* Reject anything larger than a jumbo gram or smaller than the
	 * minimum size TCP, UDP or ICMPv6 packet (i.e. just header, no payload
	 */
	if ( (ip_payload_len > 8192) ||
	     ((ip6->ip6_nxt == IPPROTO_TCP) && (ip_payload_len < 20)) ||
	     ((ip6->ip6_nxt == IPPROTO_UDP) && (ip_payload_len < 8)) ||
	     ((ip6->ip6_nxt == IPPROTO_ICMPV6) && (ip_payload_len < 4)) )
	    return false;


        // now it is faster to check the cache, before doing the checksum check
	switch (ip6->ip6_nxt) {
	default:
	case IPPROTO_TCP:
            if (pos+56 < sbuf.bufsize) {
                const struct be_tcphdr *tcp = sbuf.get_struct_ptr_unsafe<struct be_tcphdr>( pos+40 );
                /* tcp chksum is at byte offset 16 from tcp hdr + 40 w/ pseudo hdr */
                h->checksum_valid = (tcp->th_sum == scan_net_t::IPv6L3Chksum(sbuf, pos, 56));
            }
            break;

	case IPPROTO_UDP:
            if (pos+46 < sbuf.bufsize) {
                const struct be_udphdr *udp = sbuf.get_struct_ptr_unsafe<struct be_udphdr>( pos+40 );

                /* udp chksum is at byte offset 6 from udp hdr + 40 w/ pseudo hdr */
                h->checksum_valid = (udp->uh_sum == scan_net_t::IPv6L3Chksum(sbuf, pos, 46));
            }
            break;
	case IPPROTO_ICMPV6:
            if (pos+42 < sbuf.bufsize ) {
                const struct icmp6_hdr *icmp6 = sbuf.get_struct_ptr_unsafe<struct icmp6_hdr>( pos+40 );
                if (!icmp6) return false;	// not sufficient room

                /* icmpv6 chksum is at byte offset 2 from icmpv6 hdr + 40 w/ pseudo hdr */
                h->checksum_valid = (icmp6->icmp6_cksum == scan_net_t::IPv6L3Chksum(sbuf, pos, 42));
            }
            break;
        }
	/* create a generic_iphdr_t, similar to tcpip.c from tcpflow code */
	h->family = AF_INET6;
	memcpy(h->src, ip6->ip6_src.addr.addr8, sizeof(ip6->ip6_src.addr.addr8));
	memcpy(h->dst, ip6->ip6_dst.addr.addr8, sizeof(ip6->ip6_dst.addr.addr8));
	h->ttl = ip6->ip6_hlim;
	h->nxthdr = ip6->ip6_nxt;
	h->nxthdr_offs = 40; 	/* ipv6 headers are a fixed length of 40 bytes */
	h->payload_len = ntohs(ip6->ip6_plen);
        //if (sc) sc->insert(pos);
	return true;
    }
    return false;			// right now we only do IPv4 and IPv6
}

/* Test for a possible IP header. (see struct ip <netinet/ip.h> or struct ip6_hdr <netinet/ip6.h>)
 * These structures will be MEMORY STRUCTURES from swap files, hibernation files, or virtual machines
 * Please remember this is called for every byte of the disk image,
 * so it needs to be as fast as possible.
 */
/** Write the ethernet addresses and the TCP info into the appropriate feature files.
 * Return true if we should write the packet
 */

void  scan_net_t::documentIPFields(const sbuf_t &sbuf, size_t pos, const generic_iphdr_t &h) const
{
    pos0_t pos0 = sbuf.pos0 + pos;

    /* Report the IP address */
    /* based on the TTL, infer whether remote or local */
    const std::string &chksum_status = h.checksum_valid ? scan_net_t::CHKSUM_OK : scan_net_t::CHKSUM_BAD;
    std::string src,dst;

    if (isPowerOfTwo(h.ttl)){
        src = "L";	// src is local because the power of two hasn't been decremented
        dst = "R";	// dst is remote because the power of two hasn't been decremented
    } else {
        src = "R";	// src is remote
        dst = "L";	// dst is local
    }

    const std::string name = (h.family==AF_INET) ? "ip " : "ip6_hdr ";
    ip_recorder.write(pos0, ip2string(h.src, h.family), "struct " + name + src + " (src) " + chksum_status);
    ip_recorder.write(pos0, ip2string(h.dst, h.family), "struct " + name + dst + " (dst) " + chksum_status);

    /* Now report TCP, UDP and/or IPv6 contents if it is one of those */
    if (h.nxthdr==IPPROTO_TCP){
        const struct be_tcphdr *tcp = sbuf.get_struct_ptr<struct be_tcphdr>(pos+h.nxthdr_offs);
        if (tcp){
            if (ntohs(tcp->th_sport)==0 || ntohs(tcp->th_dport)==0) throw port0_exception();
            tcp_recorder.write(pos0,
                                     ip2string(h.src, h.family) + " " + i2str(ntohs(tcp->th_sport)) + " -> " +
                                     ip2string(h.dst, h.family) + " " + i2str(ntohs(tcp->th_dport)) + " (TCP)",
                                     " Size: " + i2str(h.payload_len+h.nxthdr_offs) );
        }
    }
    if (h.nxthdr==IPPROTO_UDP){
        const struct be_udphdr *udp = sbuf.get_struct_ptr<struct be_udphdr>(pos+h.nxthdr_offs);
        if (udp){
            if (ntohs(udp->uh_sport)==0 || ntohs(udp->uh_dport)==0) throw port0_exception();
            tcp_recorder.write(pos0,
                                     ip2string(h.src, h.family) + " " + i2str(ntohs(udp->uh_sport)) + " -> " +
                                     ip2string(h.dst, h.family) + " " + i2str(ntohs(udp->uh_dport)) + " (UDP)",
                                     " Size: " + i2str(h.payload_len+h.nxthdr_offs) );
        }
    }
    if (h.nxthdr==IPPROTO_ICMPV6){
        const struct icmp6_hdr *icmp6 = sbuf.get_struct_ptr<struct icmp6_hdr>(pos+h.nxthdr_offs);
        if (icmp6) tcp_recorder.write(pos0,
                                       ip2string(h.src, h.family) + " -> " +
                                       ip2string(h.dst, h.family) + " (ICMPv6)",
                                       " Type: " + i2str(icmp6->icmp6_type) + " Code: " + i2str(icmp6->icmp6_code) );
    }
}

/* Return bytes carved, or 0 if nothing carved */
size_t scan_net_t::carveIPFrame(const sbuf_t &sbuf, size_t pos, sanityCache_t *sc) const
{
    generic_iphdr_t h {};

    /* check if it looks like ipv4 or ipv6 packet
     * if neither, return false.
     * Unfortunately the sanity checks are not highly discriminatory.
     * The call below sets the 'h' structure as necessary
     */

    if (!sanityCheckIP46Header(sbuf, pos, &h, sc)) return 0;
    if (invalidIP(h.src, h.family) || invalidIP(h.dst, h.family)) return 0;
    if (h.family!=AF_INET && h.family!=AF_INET6) return 0; // only care about IPv4 and IPv6


    /* To decrease false positives, we typically do not carve packets with bad checksums.
     * With IPv6 there is no IP checksum, but there are L3 checksums, and
     * sanityCheckIP46Header checks them.
     */

    /* IPv4 has a checksum; use it if we can */
    if (h.checksum_valid==false && opt_report_checksum_bad==false) return 0; // user does not want invalid checksums

    /* A valid IPframe but not proceeded by an Ethernet or a pcap header. */
    uint8_t buf[pcap_writer::PCAP_MAX_PKT_LEN+14];
    size_t ip_len         = h.nxthdr_offs + h.payload_len;
    if (ip_len + pos > sbuf.bufsize) ip_len = sbuf.bufsize-pos;
    size_t packet_len = 14 + ip_len ;
    if (packet_len > pcap_writer::PCAP_MAX_PKT_LEN){
        packet_len = pcap_writer::PCAP_MAX_PKT_LEN;
    }
    if (packet_len < 14) return 0;	// this should never happen

    /* Create a bogus ethernet DST and SRC addresses */
    for (int i=0;i<12;i++){
        buf[i] = i;
    }
    switch(h.family){
    case AF_INET:  buf[12] = 0x08; buf[13] = 0x00; break;
    case AF_INET6: buf[12] = 0xdd; buf[13] = 0x86; break;
    default:       buf[12] = 0xff; buf[13] = 0xff; break; // shouldn't happen
    }
    memcpy(buf+14,sbuf.get_buf(),packet_len-14);       // copy the packet data

    /* make an sbuf to write */
    sbuf_t sb3(sbuf.pos0 + pos, buf, packet_len);
    struct pcap_writer::pcap_hdr ph(0, 0, packet_len, packet_len);  // make a fake header
    if (h.is_4or6()){
        std::cout << "1. sb3=" << sb3 << std::endl;
        try {
            documentIPFields(sb3, 0, h);
            pwriter.pcap_writepkt(ph, sb3, 0, false, 0x0000);	   // write the packet
        }
        catch (port0_exception &e) {
            return 0;
        }
    }
    return ip_len;                                     // return that we processed this much
}



/* Test for Ethernet link-layer MACs
 * Returns the size of the object carved
 */

size_t scan_net_t::carveEther(const sbuf_t &sbuf, size_t pos, sanityCache_t *sc) const
{
    const struct macip *er = validateEther(sbuf, pos);
    if (er == nullptr) return 0;

    size_t data_offset = (2*ETHER_ADDR_LEN)+sizeof(uint16_t);

    /**
     * We have enough data to document what is in the packet! Hurrah! Let's save it.
     */
    if (data_offset + pos < sbuf.bufsize){
        generic_iphdr_t h;

        if (sanityCheckIP46Header(sbuf, pos + data_offset, &h, sc) && h.checksum_valid) {
            if (!invalidMAC(&(er->ether_dhost))){
                ether_recorder.write(sbuf.pos0 + pos, mac2string(&(er->ether_dhost)), "(ether_dhost)");
            }
            if (!invalidMAC(&(er->ether_shost))){
                ether_recorder.write(sbuf.pos0 + pos, mac2string(&(er->ether_shost)), "(ether_shost)");
            }
            try {
                documentIPFields(sbuf, pos+data_offset, h);
                std::cout << "2. sbuf="<<sbuf<<" pos=" << pos << " data_offset=" << data_offset << std::endl;
            }
            catch (port0_exception &e) {
                return 0;
            }

        }
    }

    /* Possibly a valid ethernet frame but not preceeded by a pcap_record_header
     * (otherwise it would have been written and skipped...)
     * Write it out with a capture time of 1.
     */
    generic_iphdr_t h;
    /* the IP pkt starts after the Ethernet header, 14 byte offset */
    if (sanityCheckIP46Header(sbuf, pos+14,  &h, sc)){
        if (h.checksum_valid){
            ssize_t packet_len     = 14 + h.nxthdr_offs + h.payload_len; // ether size + ip size + ip data
            if (packet_len + pos > sbuf.bufsize) packet_len = sbuf.bufsize - pos;
            if (packet_len > 0 ){
                struct pcap_writer::pcap_hdr hz(0, 0, packet_len, packet_len);
                pwriter.pcap_writepkt(hz, sbuf, pos, false, 0x0000);
                return packet_len;
            }
        }
    }
    return 0;
}


/* Test for a possible sockaddr_in <netinet/in.h>
 * Please remember that this is called for every byte, so it needs to be fast.
 */
size_t scan_net_t::carveSockAddrIn(const sbuf_t &sbuf, size_t pos) const
{
    const struct sockaddr_in *in = sbuf.get_struct_ptr_unsafe<struct sockaddr_in>( pos );
    if (in==0) return 0;

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
    if (in->sin_zero[0]!=0 || in->sin_zero[1]!=0 || in->sin_zero[2]!=0 || in->sin_zero[3]!=0 ||
        in->sin_zero[4]!=0 || in->sin_zero[5]!=0 || in->sin_zero[6]!=0 || in->sin_zero[7]!=0){
        return 0;
    }

    /* Weed out any obviously bad IP addresses */
    if (invalidIP4((const uint8_t *)&(in->sin_addr))) return 0;

    /* Only use candidate with ports we believe most likely */
    if (!sanePort(in->sin_port)) return 0;

    ip_recorder.write(sbuf.pos0 + pos, ip2string((const be13::ip4_addr *)&(in->sin_addr)), "sockaddr_in");
    return sizeof(struct sockaddr_in);
}

/* Test for possible _TCPT_OBJECT
 * Please remember that this is called for every byte, so it needs to be fast.
 */
size_t scan_net_t::carveTCPTOBJ(const sbuf_t &sbuf, size_t pos) const
{
    static_assert( sizeof(struct tcpt_object) < MIN_PACKET_BYTES );
    const struct tcpt_object *to = sbuf.get_struct_ptr_unsafe<struct tcpt_object>( pos );

    /* 0x54455054 == "TCPT" */
    if ( (to->sig == htonl(0x54435054)) && (to->pool_size == htons(0x330A)) ) {
        pos0_t p = sbuf.pos0 + pos;
        ip_recorder.write(  p, ip2string(&(to->src)), "tcpt");
        ip_recorder.write(  p, ip2string(&(to->dst)), "tcpt");
        tcp_recorder.write( p, ip2string(&(to->src)) + ":" + i2str(ntohs(to->src_port)) + " -> " +
                             ip2string(&(to->dst)) + ":" + i2str(ntohs(to->dst_port)),  "TCPT");
        return sizeof(struct tcpt_object);
    }
    return 0;
}



/**
 * carvePCAPHeader
 * Record if we found a PCAP file header.
 */

size_t scan_net_t::carvePCAPFileHeader(const sbuf_t &sbuf, size_t pos) const
{
    /* If this is a pcap file, write it out.
     *
     * Currently we just remember the packets themselves,
     * which may cause issues if there is encapsulation present
     */

    static_assert( sizeof(PCAP_FILE_HEADER) < MIN_PACKET_BYTES );
    if ( sbuf.memcmp_unsafe(PCAP_FILE_HEADER, pos, sizeof(PCAP_FILE_HEADER)) == 0 ) {
        /* Found a header! That's great! Record this important fact */
        ip_recorder.write(sbuf.pos0+pos, pcap_writer::TCPDUMP_FR_FEATURE, pcap_writer::TCPDUMP_FR_CONTEXT);
        return pcap_writer::TCPDUMP_HEADER_SIZE;
    }
    return 0;
}

/**
 * Look for pcap packets, rather than pcap files.
 * So we look for a packet that has a pcap packet header.
 * Validate and write a pcap packet. Return the number of bytes written.
 * Called on every byte, so it must be fast.
 */
size_t scan_net_t::carvePCAPPackets(const sbuf_t &sbuf, size_t pos, sanityCache_t *sc) const
{
    struct pcap_writer::pcap_hdr pch {};
    if (likely_valid_pcap_packet_header(sbuf, pos, &pch)==false){
        return 0;
    }

    /* likely pcap per-packet header! */
    if (pos+PCAP_RECORD_HEADER_SIZE+pch.cap_len > sbuf.bufsize ){
        /* packet was truncated */
        throw sbuf_t::range_exception_t(pos, PCAP_RECORD_HEADER_SIZE+pch.cap_len);
    }

    /* We found what could be a validate packet header.
     * However, they aren't a lot of checks.
     * So carve this packet if the *next* packet looks valid or if we are at the end of the buffer.
     */
    struct pcap_writer::pcap_hdr h_next {};
    bool pcap_at_end_of_sbuf   = ( pos + pch.cap_len+PCAP_RECORD_HEADER_SIZE == sbuf.bufsize);
    bool next_pcap_header_looks_valid = likely_valid_pcap_packet_header(sbuf, pos+PCAP_RECORD_HEADER_SIZE+pch.cap_len, &h_next);

    if ( pcap_at_end_of_sbuf || next_pcap_header_looks_valid ){

        // If we got here, then carve the packet.
        // pcap files can contain either raw IP or ethernet followed by IP.
        // If it is a raw IP, we make a pseudo frame

        generic_iphdr_t h2;
        uint16_t pseudo_frame_ethertype = 0;
        bool is_raw_ip = sanityCheckIP46Header(sbuf, pos+PCAP_RECORD_HEADER_SIZE, &h2, sc);
        if (is_raw_ip) {
            // It's raw IP if the IP46 header validated. So make a fake header. We've already learned the IPv46 header.
            pseudo_frame_ethertype = (h2.family == AF_INET6) ? ETHERTYPE_IPV6 : ETHERTYPE_IP;
        } else {
            // Otherwise, learn the IPv46 header
            sanityCheckIP46Header(sbuf, pos+PCAP_RECORD_HEADER_SIZE+ETHER_HEAD_LEN, &h2, sc);
        }

        /* If this is a IPv4 or IPv6 (from the learned header), write it out.*/
        if (h2.is_4or6()) {
            try {
                documentIPFields(sbuf, pos+PCAP_RECORD_HEADER_SIZE, h2);
                pwriter.pcap_writepkt(pch, sbuf, pos+PCAP_RECORD_HEADER_SIZE, is_raw_ip, pseudo_frame_ethertype);
                std::cout << "3. sbuf=" << sbuf << " pos=" << pos << std::endl;
            }
            catch (port0_exception &e) {
                return 0;
            }
        }

        return PCAP_RECORD_HEADER_SIZE + pch.cap_len;
    }
    return 0;                       // not written
}

void scan_net_t::carve(const sbuf_t &sbuf) const
{
    /* Scan through every byte of the buffer for all possible packets
     * If we find a pcap file or a packet at the present location,
     * don't bother with the remainder.
     *
     * Please remember that this is called for every byte, so it needs to be fast.
     * min_packet_bytes in this loop allows all memory operations to be unsafe.
     *
     * - First we check to see if this is the start of a PCAP eader followed by a frame. If so, we carve and write to the pcap file. We got the time/date stamp!
     * - Second, we check to see if this is the start of an ethernet header. If it is, we carve the packet and advance the pointer.
     * - Third, just check to see if it is an IP packet.
     * - Next, check to see if it is a recognized memory structure.
     * - if it is none of those things, advance a byte and try again.
     * - stop when there is not enough space left for a packet.
     */
    size_t pos = 0;
    sanityCache_t sc {};
    while (pos + min_packet_bytes < sbuf.pagesize) {
        /* Look for a PCAPFile header */
        size_t file_header_bytes = carvePCAPFileHeader( sbuf, pos);
        if (file_header_bytes > 0) {
            pos += file_header_bytes;
            continue;
        }

        /* Look for a PCAP Packet without a PCAP File header. Could just be floating in space. Or it could be following a header.*/
        size_t packet_bytes = carvePCAPPackets( sbuf, pos, &sc );
        if (packet_bytes > 0) {
            pos += packet_bytes;
            continue;
        }

        /* Look for another recognizable structure. If we find it, advance as far as a the biggest one */
        // carve either caused the problems!
        size_t carved_ether_bytes = carveEther( sbuf, pos, &sc ); // look for an ethernet packet; true causes the packet to be carved if found
        if (carved_ether_bytes > 0 ){
            pos += carved_ether_bytes;
            continue;
        }
        size_t carved_ip_bytes = carveIPFrame( sbuf, pos, &sc ); // look for an IP packet
        if (carved_ip_bytes > 0) {
            pos += carved_ip_bytes;
            continue;
        }
        if (carve_net_memory){
            /* If we can't carve a packet, look for these two memory structures */
            size_t carved_memory_bytes = std::max(carveSockAddrIn( sbuf, pos ), carveTCPTOBJ( sbuf, pos ));
            if (carved_memory_bytes > 0 ) {
                pos += carved_memory_bytes;
                continue;
            }
        }
        pos += 1;
    }
    pwriter.flush();
};

extern "C"
void scan_net(scanner_params &sp)
{
    static scan_net_t *scanner = nullptr;
    static bool opt_carve_net_memory = false;
    static int  opt_min_carve_packet_bytes = scan_net_t::DEFAULT_MIN_PACKET_BYTES;
    sp.check_version();
    if (sp.phase==scanner_params::PHASE_INIT){

        sp.get_scanner_config("carve_net_memory",&opt_carve_net_memory,"Carve network  memory structures");
        sp.get_scanner_config("min_carve_packet_bytes",&opt_min_carve_packet_bytes,"Smallest network packet to carve");

	assert(sizeof(struct be13::ip4)==20);	// we've had problems on some systems
        sp.info->set_name("net");
        sp.info->author         = "Simson Garfinkel and Rob Beverly";
        sp.info->description    = "Scans for IP packets";
        sp.info->scanner_version= "1.0";
        sp.info->min_sbuf_size  = 16;

	sp.info->feature_defs.push_back( feature_recorder_def("ip"));
	sp.info->feature_defs.push_back( feature_recorder_def("ether"));

	/* changed the pattern to be the entire feature,
	 * since histogram was not being created with previous pattern
	 */
        /*                                              name,  feature_file, pattern, require, filename_suffix , flags */
        histogram_def::flags_t f;
        f.require_context = true;
        f.require_feature = false;
	sp.info->histogram_defs.push_back( histogram_def("ip",  "ip",      "", scan_net_t::CHKSUM_OK, "histogram", f));
        sp.info->histogram_defs.push_back( histogram_def("ether","ether", "([^\(]+)","", "histogram", histogram_def::flags_t()));

        sp.info->feature_defs.push_back( feature_recorder_def("tcp"));
        sp.info->histogram_defs.push_back(histogram_def("tcp", "tcp", "", "", "histogram", histogram_def::flags_t()));
        return;
    }
    if (sp.phase==scanner_params::PHASE_INIT2){
        scanner = new scan_net_t(sp);

        if (uint32_t(opt_min_carve_packet_bytes) < scan_net_t::MIN_PACKET_BYTES ){
            throw std::runtime_error("min_carve_packet_bytes too small");
        }

        scanner->carve_net_memory = opt_carve_net_memory;
        scanner->min_packet_bytes  = opt_min_carve_packet_bytes;
    }
    if (sp.phase==scanner_params::PHASE_SCAN){
        try {
            scanner->carve(*sp.sbuf);
            //std::cout << "ctr1=" << ctr1 << " ctr2=" << ctr2 << std::endl;
        }
        catch (const sbuf_t::range_exception_t &e ) {
            /*
             * the decoder above happily goes off the end of the sbuf. When that happens,
             * we get an exception, which is caught here. It is non-fatal. The code below
             * was just for testing.
             */
            //std::cerr << "scan_net: " << e << std::endl;
        }
    }
    if (sp.phase==scanner_params::PHASE_CLEANUP){
        if (scanner){
            delete scanner;
            scanner = nullptr;
        }
	return;
    }
}
