#include "config.h"

#include <set>
#include <mutex>
#include <ctype.h>

#include "be13_api/formatter.h"
#include "be13_api/utils.h"
#include "pcap_writer.h"

/****************************************************************
 ** pcap_writer code
 **/

pcap_writer::pcap_writer(const scanner_params &sp):
    outpath(sp.sc.outdir / OUTPUT_FILENAME)
{
}

pcap_writer::~pcap_writer()
{
    const std::lock_guard<std::mutex> lock(Mfcap);
    if (fcap){
        fcap->close();
        delete fcap;
        fcap = nullptr;
    }
}

void pcap_writer::pcap_write_bytes(const uint8_t * const val, size_t num_bytes)
{
    fcap->write(reinterpret_cast<const char *>(val),num_bytes);
    if (fcap->rdstate() & (std::ios::failbit|std::ios::badbit)){
        throw std::runtime_error(Formatter() << "scanner pcap_writer is unable to write to file " << outpath);
    }
}

/* Write a 16-bit value, little end first */
void pcap_writer::pcap_write2(const uint16_t val)
{
    char ch = val & 0xff;
    *fcap << ch;
    ch = val >> 8;
    *fcap << ch;
    if (fcap->rdstate() & (std::ios::failbit|std::ios::badbit)){
        throw std::runtime_error(Formatter() << "scanner scan_net is unable to write to file " << outpath);
    }
}

void pcap_writer::pcap_write4(const uint32_t val)
{
    fcap->write(reinterpret_cast<const char *>(&val), 4);
    if (fcap->rdstate() & (std::ios::failbit|std::ios::badbit)){
        throw std::runtime_error(Formatter() << "scanner scan_net is unable to write to file " << outpath);
    }
}


/*
 * @param add_frame - should we add a frame?
 * @param frame_type - the ethernet frame type. Note that this could be combined with add_frame, with frame_type=0 for no add.
 */
void pcap_writer::pcap_writepkt(const struct pcap_writer::pcap_hdr &h, // packet header
                                const sbuf_t &sbuf,       // sbuf where packet is located
                                const size_t pos,         // position within the sbuf
                                const bool add_frame,     // whether or not to create a synthetic ethernet frame
                                const uint16_t frame_type)  // if we add a frame, the frame type
{
    // Make sure that neither this packet nor an encapsulated version of this packet has been written
    const std::lock_guard<std::mutex> lock(Mfcap);  // lock the mutex
    if (fcap==0){
        fcap = new std::ofstream(outpath, std::ios::binary); // write the output
        if (fcap->is_open()==false){
            throw std::runtime_error(Formatter() << "pcap_writer.cpp: cannot open " << outpath << " for  writing");
        }
        pcap_write4(0xa1b2c3d4);
        pcap_write2(2);			// major version number
        pcap_write2(4);			// minor version number
        pcap_write4(0);			// time zone offset; always 0
        pcap_write4(0);			// accuracy of time stamps in the file; always 0
        pcap_write4(PCAP_MAX_PKT_LEN);	// snapshot length
        pcap_write4(DLT_EN10MB);	// link layer encapsulation
        assert( fcap->tellp() == TCPDUMP_HEADER_SIZE );
    }

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
    pcap_write4(h.seconds);		// time stamp, seconds avalue
    pcap_write4(h.useconds);		// time stamp, microseconds
    pcap_write4(h.cap_len + forged_header_len);
    pcap_write4(h.pkt_len + forged_header_len);
    if (add_frame_and_safe) {
        pcap_write_bytes(forged_header, sizeof(forged_header));
    }
    sbuf.write(*fcap, pos, h.cap_len );	// the packet
}

void pcap_writer::flush()
{
    const std::lock_guard<std::mutex> lock(Mfcap);
    if (fcap){
        fcap->flush();
    }
}
