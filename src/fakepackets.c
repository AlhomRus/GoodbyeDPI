#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <in6addr.h>
#include <ws2tcpip.h>
#include "windivert.h"
#include "goodbyedpi.h"

static const unsigned char fake_http_request[] = "GET / HTTP/2.0\r\nHost: google.com\r\n"
                                                 "User-Agent: curl/8.2.1\r\nAccept: */*\r\n"
                                                 "Accept-Encoding: deflate, gzip, br\r\n\r\n";
static const unsigned char fake_https_request[] = {
    0x16, 0x03, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01, 0xfc, 0x03, 0x03, 0x9a, 0x8f, 0xa7, 0x6a, 0x5d,
    0x57, 0xf3, 0x62, 0x19, 0xbe, 0x46, 0x82, 0x45, 0xe2, 0x59, 0x5c, 0xb4, 0x48, 0x31, 0x12, 0x15,
    0x14, 0x79, 0x2c, 0xaa, 0xcd, 0xea, 0xda, 0xf0, 0xe1, 0xfd, 0xbb, 0x20, 0xf4, 0x83, 0x2a, 0x94,
    0xf1, 0x48, 0x3b, 0x9d, 0xb6, 0x74, 0xba, 0x3c, 0x81, 0x63, 0xbc, 0x18, 0xcc, 0x14, 0x45, 0x57,
    0x6c, 0x80, 0xf9, 0x25, 0xcf, 0x9c, 0x86, 0x60, 0x50, 0x31, 0x2e, 0xe9, 0x00, 0x22, 0x13, 0x01,
    0x13, 0x03, 0x13, 0x02, 0xc0, 0x2b, 0xc0, 0x2f, 0xcc, 0xa9, 0xcc, 0xa8, 0xc0, 0x2c, 0xc0, 0x30,
    0xc0, 0x0a, 0xc0, 0x09, 0xc0, 0x13, 0xc0, 0x14, 0x00, 0x33, 0x00, 0x39, 0x00, 0x2f, 0x00, 0x35,
    0x01, 0x00, 0x01, 0x91, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0d, 0x00, 0x00, 0x0a, 0x77, 0x77, 0x77,
    0x2e, 0x77, 0x33, 0x2e, 0x6f, 0x72, 0x67, 0x00, 0x17, 0x00, 0x00, 0xff, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x0a, 0x00, 0x0e, 0x00, 0x0c, 0x00, 0x1d, 0x00, 0x17, 0x00, 0x18, 0x00, 0x19, 0x01, 0x00,
    0x01, 0x01, 0x00, 0x0b, 0x00, 0x02, 0x01, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0e,
    0x00, 0x0c, 0x02, 0x68, 0x32, 0x08, 0x68, 0x74, 0x74, 0x70, 0x2f, 0x31, 0x2e, 0x31, 0x00, 0x05,
    0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x6b, 0x00, 0x69, 0x00, 0x1d, 0x00,
    0x20, 0xb0, 0xe4, 0xda, 0x34, 0xb4, 0x29, 0x8d, 0xd3, 0x5c, 0x70, 0xd3, 0xbe, 0xe8, 0xa7, 0x2a,
    0x6b, 0xe4, 0x11, 0x19, 0x8b, 0x18, 0x9d, 0x83, 0x9a, 0x49, 0x7c, 0x83, 0x7f, 0xa9, 0x03, 0x8c,
    0x3c, 0x00, 0x17, 0x00, 0x41, 0x04, 0x4c, 0x04, 0xa4, 0x71, 0x4c, 0x49, 0x75, 0x55, 0xd1, 0x18,
    0x1e, 0x22, 0x62, 0x19, 0x53, 0x00, 0xde, 0x74, 0x2f, 0xb3, 0xde, 0x13, 0x54, 0xe6, 0x78, 0x07,
    0x94, 0x55, 0x0e, 0xb2, 0x6c, 0xb0, 0x03, 0xee, 0x79, 0xa9, 0x96, 0x1e, 0x0e, 0x98, 0x17, 0x78,
    0x24, 0x44, 0x0c, 0x88, 0x80, 0x06, 0x8b, 0xd4, 0x80, 0xbf, 0x67, 0x7c, 0x37, 0x6a, 0x5b, 0x46,
    0x4c, 0xa7, 0x98, 0x6f, 0xb9, 0x22, 0x00, 0x2b, 0x00, 0x09, 0x08, 0x03, 0x04, 0x03, 0x03, 0x03,
    0x02, 0x03, 0x01, 0x00, 0x0d, 0x00, 0x18, 0x00, 0x16, 0x04, 0x03, 0x05, 0x03, 0x06, 0x03, 0x08,
    0x04, 0x08, 0x05, 0x08, 0x06, 0x04, 0x01, 0x05, 0x01, 0x06, 0x01, 0x02, 0x03, 0x02, 0x01, 0x00,
    0x2d, 0x00, 0x02, 0x01, 0x01, 0x00, 0x1c, 0x00, 0x02, 0x40, 0x01, 0x00, 0x15, 0x00, 0x96, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00
};

static int send_fake_data(const HANDLE w_filter,
                          const PWINDIVERT_ADDRESS addr,
                          const char *pkt,
                          const UINT packetLen,
                          const BOOL is_ipv6,
                          const BOOL is_https,
                          const BYTE set_ttl,
                          const BYTE set_checksum,
                          const BYTE set_seq
                         ) {
    char packet_fake[MAX_PACKET_SIZE];
    WINDIVERT_ADDRESS addr_new;
    PVOID packet_data;
    UINT packet_dataLen;
    UINT packetLen_new;
    PWINDIVERT_IPHDR ppIpHdr;
    PWINDIVERT_IPV6HDR ppIpV6Hdr;
    PWINDIVERT_TCPHDR ppTcpHdr;
    unsigned const char *fake_request_data = is_https ? fake_https_request : fake_http_request;
    UINT fake_request_size = is_https ? sizeof(fake_https_request) : sizeof(fake_http_request) - 1;

    memcpy(&addr_new, addr, sizeof(WINDIVERT_ADDRESS));
    memcpy(packet_fake, pkt, packetLen);

    addr_new.TCPChecksum = 0;
    addr_new.IPChecksum = 0;

    if (!is_ipv6) {
        // IPv4 TCP Data packet
        if (!WinDivertHelperParsePacket(packet_fake, packetLen, &ppIpHdr,
            NULL, NULL, NULL, NULL, &ppTcpHdr, NULL, &packet_data, &packet_dataLen,
            NULL, NULL))
            return 1;
    }
    else {
        // IPv6 TCP Data packet
        if (!WinDivertHelperParsePacket(packet_fake, packetLen, NULL,
            &ppIpV6Hdr, NULL, NULL, NULL, &ppTcpHdr, NULL, &packet_data, &packet_dataLen,
            NULL, NULL))
            return 1;
    }

    if (packetLen + fake_request_size + 1 > MAX_PACKET_SIZE)
        return 2;

    memcpy(packet_data, fake_request_data, fake_request_size);
    packetLen_new = packetLen - packet_dataLen + fake_request_size;

    if (!is_ipv6) {
        ppIpHdr->Length = htons(
            ntohs(ppIpHdr->Length) -
            packet_dataLen + fake_request_size
        );

        if (set_ttl)
            ppIpHdr->TTL = set_ttl;
    }
    else {
        ppIpV6Hdr->Length = htons(
            ntohs(ppIpV6Hdr->Length) -
            packet_dataLen + fake_request_size
        );

        if (set_ttl)
            ppIpV6Hdr->HopLimit = set_ttl;
    }

    if (set_seq) {
        // This is the smallest ACK drift Linux can't handle already, since at least v2.6.18.
        // https://github.com/torvalds/linux/blob/v2.6.18/net/netfilter/nf_conntrack_proto_tcp.c#L395
        ppTcpHdr->AckNum = htonl(ntohl(ppTcpHdr->AckNum) - 66000);
        // This is just random, no specifics about this value.
        ppTcpHdr->SeqNum = htonl(ntohl(ppTcpHdr->SeqNum) - 10000);
    }

    // Recalculate the checksum
    WinDivertHelperCalcChecksums(packet_fake, packetLen_new, &addr_new, 0ULL);

    if (set_checksum) {
        // ...and damage it
        ppTcpHdr->Checksum = htons(ntohs(ppTcpHdr->Checksum) - 1);
    }
    //printf("Pseudo checksum: %d\n", addr_new.TCPChecksum);

    WinDivertSend(
        w_filter, packet_fake,
        packetLen_new,
        NULL, &addr_new
    );
    debug("Fake packet: OK");

    return 0;
}

static int send_fake_request(const HANDLE w_filter,
                                  const PWINDIVERT_ADDRESS addr,
                                  const char *pkt,
                                  const UINT packetLen,
                                  const BOOL is_ipv6,
                                  const BOOL is_https,
                                  const BYTE set_ttl,
                                  const BYTE set_checksum,
                                  const BYTE set_seq
                                 ) {
    if (set_ttl) {
        send_fake_data(w_filter, addr, pkt, packetLen,
                          is_ipv6, is_https,
                          set_ttl, FALSE, FALSE);
    }
    if (set_checksum) {
        send_fake_data(w_filter, addr, pkt, packetLen,
                          is_ipv6, is_https,
                          FALSE, set_checksum, FALSE);
    }
    if (set_seq) {
        send_fake_data(w_filter, addr, pkt, packetLen,
                          is_ipv6, is_https,
                          FALSE, FALSE, set_seq);
    }
    return 0;
}

int send_fake_http_request(const HANDLE w_filter,
                                  const PWINDIVERT_ADDRESS addr,
                                  const char *pkt,
                                  const UINT packetLen,
                                  const BOOL is_ipv6,
                                  const BYTE set_ttl,
                                  const BYTE set_checksum,
                                  const BYTE set_seq
                                 ) {
    return send_fake_request(w_filter, addr, pkt, packetLen,
                          is_ipv6, FALSE,
                          set_ttl, set_checksum, set_seq);
}

int send_fake_https_request(const HANDLE w_filter,
                                   const PWINDIVERT_ADDRESS addr,
                                   const char *pkt,
                                   const UINT packetLen,
                                   const BOOL is_ipv6,
                                   const BYTE set_ttl,
                                   const BYTE set_checksum,
                                   const BYTE set_seq
                                 ) {
    return send_fake_request(w_filter, addr, pkt, packetLen,
                          is_ipv6, TRUE,
                          set_ttl, set_checksum, set_seq);
}
