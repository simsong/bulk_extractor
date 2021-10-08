#ifndef PCAP_WRITER_H
#define PCAP_WRITER_H

/* pcap_writer:
 * Encapsulates the logic of writing pcap files.
 *
 * Currently this will not write out a truncated packet.
 * multi-threaded, supporting a single object for multiple threads that's used for the entire bulk_extractor run.
 *
 * Should probably be implemented as a stand-alone class, rather than a subclass of scan_net, to make it testable.
 */
class pcap_writer: public scan_net_t {
    static const inline std::string OUTPUT_FILENAME {"packets.pcap"};
    pcap_writer(const pcap_writer &pc) = delete;
    pcap_writer &operator=(const pcap_writer &that) = delete;
    mutable std::mutex Mfcap {};              // mutex for fcap
    mutable FILE *fcap = 0;		      // capture file, protected by M
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
    void pcap_write_bytes(const uint8_t * const val, size_t num_bytes) const {
        size_t count = fwrite(val,1,num_bytes,fcap);
        if (count != num_bytes) {
            std::cerr << "scanner scan_net is unable to write to file " << outpath << "\n";
            throw std::runtime_error("fwrite failed");
        }
    }
    void pcap_write2(const uint16_t val) const {
        size_t count = fwrite(&val,1,2,fcap);
        if (count != 2) {
            std::cerr << "scanner scan_net is unable to write to file " << outpath << "\n";
            throw std::runtime_error("fwrite failed");
        }
    }
    void pcap_write4(const uint32_t val) const {
        size_t count = fwrite(&val,1,4,fcap);
        if (count != 4) {
            std::cerr << "scanner scan_net is unable to write to file " << outpath << "\n";
            throw std::runtime_error("fwrite failed");
        }
    }

public:
    pcap_writer(const scanner_params &sp);
    ~pcap_writer();

    void flush() const override {
	if (fcap){
            const std::lock_guard<std::mutex> lock(Mfcap);
            fflush(fcap);
        }
    }

    /* write an IP packet to the output stream, optionally writing a pcap header.
     * Length of packet is determined from IP header.
     */
    void pcap_writepkt(const struct pcap_hdr &h, // packet header
		       const sbuf_t &sbuf,       // sbuf where packet is located
                       const size_t pos,         // position within the sbuf
                       const bool add_frame,     // whether or not to create a synthetic ethernet frame
                       const uint16_t frame_type) const override;


};


#endif
