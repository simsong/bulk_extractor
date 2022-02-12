#ifndef PCAP_WRITER_H
#define PCAP_WRITER_H

#include <mutex>
#include <filesystem>
#include <iostream>

#include "be20_api/scanner_params.h"

/* pcap_writer:
 * Encapsulates the logic of writing pcap files.
 *
 * Currently this will not write out a truncated packet.
 * multi-threaded, supporting a single object for multiple threads that's used for the entire bulk_extractor run.
 *
 */

#ifndef DLT_EN10MB
#define DLT_EN10MB	1	/* Ethernet (10Mb) */
#endif

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN      6
#endif

#ifndef ETHER_HEAD_LEN
#define ETHER_HEAD_LEN      14
#endif

#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP        0x0800	/* IP protocol */
#endif

#ifndef ETHERTYPE_VLAN
#define	ETHERTYPE_VLAN	    0x8100	/* IEEE 802.1Q VLAN tagging */
#endif

#ifndef ETHERTYPE_IPV6
#define	ETHERTYPE_IPV6	    0x86dd	/* IP protocol version 6 */
#endif

class pcap_writer {
    static const inline std::string OUTPUT_FILENAME {"packets.pcap"};
    pcap_writer(const pcap_writer &pc) = delete;
    pcap_writer &operator=(const pcap_writer &that) = delete;
    std::mutex Mfcap {};              // mutex for fcap
    std::ofstream *fcap = nullptr;		      // capture file, protected by M
    std::filesystem::path outpath;            // where it gets written

    /*
     * According to 'man pcap-savefile', you need to implement this file format,
     * but there are no functions to do so.
     *
     * pcap_write_bytes writes bytes; pcap accomidates.
     * pcap_write2 writes a 2-byte value in native byte order; pcap accomidates.
     * pcap_write4 writes a 4-byte value in native byte order; pcap accomidates.
     * pcap_writepkt writes a packet
     */
    void pcap_write_bytes(const uint8_t * const val, size_t num_bytes);
    void pcap_write2(const uint16_t val);
    void pcap_write4(const uint32_t val);

public:
    const static inline size_t PCAP_MAX_PKT_LEN  = 65535;	// The longest a packet may be; longer values make wireshark refuse to load
    const static inline std::string TCPDUMP_FR_FEATURE {"0xd4,0xc3,0xb2,0xa1"};
    const static inline std::string TCPDUMP_FR_CONTEXT {"TCPDUMP file"};
    const static inline uint32_t    TCPDUMP_HEADER_SIZE = 24;
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


    pcap_writer(const scanner_params &sp);
    ~pcap_writer();

    void flush();

    /* write an IP packet to the output stream, optionally writing a pcap header.
     * Length of packet is determined from IP header.
     */
    void pcap_writepkt(const struct pcap_hdr &h, // packet header
		       const sbuf_t &sbuf,       // sbuf where packet is located
                       const size_t pos,         // position within the sbuf
                       const bool add_frame,     // whether or not to create a synthetic ethernet frame
                       const uint16_t frame_type);


};


#endif
