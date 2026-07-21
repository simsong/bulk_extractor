#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstdio>
#include <cstring>

#include "pcap_fake.h"

namespace {
FILE *pcap_file(uint32_t snaplen, uint32_t caplen = 0, size_t data_size = 0)
{
    FILE *fp = tmpfile();
    REQUIRE(fp != nullptr);

    pcap_file_header header {};
    header.magic = 0xa1b2c3d4;
    header.version_major = PCAP_VERSION_MAJOR;
    header.version_minor = PCAP_VERSION_MINOR;
    header.snaplen = snaplen;
    REQUIRE(fwrite(&header, sizeof(header), 1, fp) == 1);

    if (caplen != 0) {
        uint32_t value = 0;
        REQUIRE(fwrite(&value, sizeof(value), 1, fp) == 1);
        REQUIRE(fwrite(&value, sizeof(value), 1, fp) == 1);
        REQUIRE(fwrite(&caplen, sizeof(caplen), 1, fp) == 1);
        REQUIRE(fwrite(&caplen, sizeof(caplen), 1, fp) == 1);
        for (size_t i = 0; i < data_size; i++) { REQUIRE(fputc(0, fp) != EOF); }
    }
    rewind(fp);
    return fp;
}

void count_packet(uint8_t *count, const pcap_pkthdr *, const uint8_t *)
{
    ++*count;
}
}

TEST_CASE("pcap fallback rejects oversized snaplen", "[pcap_fake]")
{
    char errbuf[PCAP_ERRBUF_SIZE] {};
    FILE *fp = pcap_file(PCAP_MAX_SNAPLEN + 1);
    REQUIRE(pcap_fopen_offline(fp, errbuf) == nullptr);
    REQUIRE(std::strstr(errbuf, "snaplen") != nullptr);
    fclose(fp);
}

TEST_CASE("pcap fallback rejects a record larger than snaplen", "[pcap_fake]")
{
    char errbuf[PCAP_ERRBUF_SIZE] {};
    FILE *fp = pcap_file(64, 65);
    pcap_t *pcap = pcap_fopen_offline(fp, errbuf);
    REQUIRE(pcap != nullptr);

    uint8_t callbacks = 0;
    REQUIRE(pcap_loop(pcap, 1, count_packet, &callbacks) == -1);
    REQUIRE(callbacks == 0);
    REQUIRE(std::strstr(pcap_geterr(pcap), "exceeds snaplen") != nullptr);
    pcap_close(pcap);
    fclose(fp);
}

TEST_CASE("pcap fallback rejects truncated packet data", "[pcap_fake]")
{
    char errbuf[PCAP_ERRBUF_SIZE] {};
    FILE *fp = pcap_file(64, 64, 1);
    pcap_t *pcap = pcap_fopen_offline(fp, errbuf);
    REQUIRE(pcap != nullptr);

    uint8_t callbacks = 0;
    REQUIRE(pcap_loop(pcap, 1, count_packet, &callbacks) == -1);
    REQUIRE(callbacks == 0);
    REQUIRE(std::strstr(pcap_geterr(pcap), "Truncated") != nullptr);
    pcap_close(pcap);
    fclose(fp);
}
