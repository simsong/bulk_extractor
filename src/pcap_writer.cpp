#include "config.h"

#include <set>
#include <mutex>
#include <ctype.h>

#include "be20_api/formatter.h"
#include "be20_api/utils.h"
#include "pcap_writer.h"

/****************************************************************
 ** pcap_writer code
 **/

pcap_writer::pcap_writer(const scanner_params &sp):
    outdir(sp.sc.outdir), streams()
{
}

pcap_writer::~pcap_writer()
{
    const std::lock_guard<std::mutex> lock(Mfcap);
    for (auto &[link_type, stream] : streams){
        if (stream.fcap){
            stream.fcap->close();
        }
    }
}

void pcap_writer::pcap_write_bytes(std::ofstream &fcap, const uint8_t *val, size_t num_bytes) const
{
    fcap.write(reinterpret_cast<const char *>(val),num_bytes);
    if (fcap.rdstate() & (std::ios::failbit|std::ios::badbit)){
        throw std::runtime_error("scanner pcap_writer is unable to write packet data");
    }
}

/* Write a 16-bit value, little end first */
void pcap_writer::pcap_write2(std::ofstream &fcap, const uint16_t val) const
{
    char ch = val & 0xff;
    fcap << ch;
    ch = val >> 8;
    fcap << ch;
    if (fcap.rdstate() & (std::ios::failbit|std::ios::badbit)){
        throw std::runtime_error("scanner pcap_writer is unable to write packet data");
    }
}

void pcap_writer::pcap_write4(std::ofstream &fcap, const uint32_t val) const
{
    pcap_write2(fcap, static_cast<uint16_t>(val));
    pcap_write2(fcap, static_cast<uint16_t>(val >> 16));
}

std::ofstream &pcap_writer::pcap_stream(uint32_t link_type)
{
    auto result = streams.try_emplace(link_type);
    stream_t &stream = result.first->second;
    if (stream.fcap) return *stream.fcap;

    const std::string filename = link_type == DLT_EN10MB ? OUTPUT_FILENAME :
        link_type == DLT_IEEE802_11 ? "packets_80211.pcap" :
        "packets_linktype_" + std::to_string(link_type) + ".pcap";
    stream.outpath = outdir / filename;
    stream.fcap = std::make_unique<std::ofstream>(stream.outpath, std::ios::binary);
    if (!stream.fcap->is_open()){
        throw std::runtime_error(Formatter() << "pcap_writer.cpp: cannot open " << stream.outpath << " for writing");
    }
    auto &fcap = *stream.fcap;
    pcap_write4(fcap, 0xa1b2c3d4);
    pcap_write2(fcap, 2);
    pcap_write2(fcap, 4);
    pcap_write4(fcap, 0);
    pcap_write4(fcap, 0);
    pcap_write4(fcap, PCAP_MAX_PKT_LEN);
    pcap_write4(fcap, link_type);
    assert(fcap.tellp() == TCPDUMP_HEADER_SIZE);
    return fcap;
}


/*
 * @param add_frame - should we add a frame?
 * @param frame_type - the ethernet frame type. Note that this could be combined with add_frame, with frame_type=0 for no add.
 */
void pcap_writer::pcap_writepkt(const struct pcap_writer::pcap_hdr &h, // packet header
                                const sbuf_t &sbuf,       // sbuf where packet is located
                                const size_t pos,         // position within the sbuf
                                const bool add_frame,     // whether or not to create a synthetic ethernet frame
                                const uint16_t frame_type,
                                const uint32_t link_type)
{
    // Make sure that neither this packet nor an encapsulated version of this packet has been written
    const std::lock_guard<std::mutex> lock(Mfcap);  // lock the mutex
    const uint32_t output_link_type = add_frame ? DLT_EN10MB : link_type;
    std::ofstream &fcap = pcap_stream(output_link_type);

    size_t forged_header_len = 0;
    uint8_t forged_header[ETHER_HEAD_LEN];
    /*
     * if requested, forge an Ethernet II header and prepend it to the packet so raw packets can
     * coexist happily in an ethernet pcap file.  Don't do this if the resulting packet length
     * make the packet larger than the largest allowable packet in a pcap file.
     */
    bool add_frame_and_safe = add_frame && h.cap_len + ETHER_HEAD_LEN <= PCAP_MAX_PKT_LEN;
    if (add_frame_and_safe) {
        forged_header_len = sizeof(forged_header);

        // forge Ethernet II header
        //   - source and destination addrs are all zeroes, ethernet type is supplied by function caller
        memset(forged_header, 0x00, sizeof(forged_header));
        // final two bytes of header hold the type value
        forged_header[sizeof(forged_header)-2] = (uint8_t) (frame_type >> 8);
        forged_header[sizeof(forged_header)-1] = (uint8_t) frame_type;
    }

    /* Write a packet */
    pcap_write4(fcap, h.seconds);
    pcap_write4(fcap, h.useconds);
    pcap_write4(fcap, h.cap_len + forged_header_len);
    pcap_write4(fcap, h.pkt_len + forged_header_len);
    if (add_frame_and_safe) {
        pcap_write_bytes(fcap, forged_header, sizeof(forged_header));
    }
    sbuf.write(fcap, pos, h.cap_len );
}

void pcap_writer::flush()
{
    const std::lock_guard<std::mutex> lock(Mfcap);
    for (auto &[link_type, stream] : streams){
        if (stream.fcap){
            stream.fcap->flush();
        }
    }
}
