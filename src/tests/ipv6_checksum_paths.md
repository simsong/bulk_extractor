# IPv6 checksum-path fixture provenance

`ipv6_checksum_paths.pcap` contains three unmodified Ethernet/IPv6 packets
from Wireshark's public [v6.pcap sample capture][source]. They exercise the
direct IPv6 next-header paths in `scan_net_t::ip6_cksum_valid()`:

1. TCP SYN (`next-header TCP (6)`), captured at `14:45:18.266121`.
2. DNS query over UDP (`next-header UDP (17)`), captured at `14:45:02.141757`.
3. ICMPv6 Neighbor Solicitation (`next-header ICMPv6 (58)`), captured at
   `14:45:07.494265`.

Downloaded 2026-07-27 from:

    https://wiki.wireshark.org/uploads/__moin_import__/attachments/SampleCaptures/v6.pcap

Each packet was extracted from that file with `tcpdump`, preserving its
original packet record and Ethernet framing:

    tcpdump -r v6.pcap -c 1 -w tcp.pcap 'ip6 and tcp'
    tcpdump -r v6.pcap -c 1 -w udp.pcap 'ip6 and udp'
    tcpdump -r v6.pcap -c 1 -w icmpv6.pcap 'ip6 and icmp6'

The three records were then combined under the shared libpcap file header.
Sanity-check the checked-in fixture with:

    tcpdump -nn -vv -r src/tests/ipv6_checksum_paths.pcap

Its SHA-256 is
`ddfc690d52063428f4843f0d17baf1429e966964efa2066a295d244589a676f9`.

[source]: https://wiki.wireshark.org/SampleCaptures
