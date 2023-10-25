/*
MIT License

Copyright (c) 2023 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdint.h>

#include "nx_api.h"
#include "nx_auto_ip.h"
#include "nxd_dhcp_client.h"
#include "nxd_dhcpv6_client.h"
#include "nxd_mdns.h"
#include "tx_api.h"

class Network {
   public:
    static Network &instance();

    uint8_t *setup(uint8_t *pointer);
    bool start();

    NX_IP *ip() { return &client_ip; };
    NX_PACKET_POOL *pool() { return &client_pool; }

    const char *hostName() const { return hostname; }
    const uint8_t *MACAddr() const { return macaddr; }
    const NXD_ADDRESS *ipv4Addr() const { return &ipv4; }
    const NXD_ADDRESS *ipv4Mask() const { return &ipv4mask; }
    const NXD_ADDRESS *ipv6Addr() const { return &ipv6; }
    uint32_t ipv6Prefix() const { return uint32_t(ipv6prefix); }

    void ClientIPChange(NX_IP *ip_ptr, VOID *user);
    void ArtNetReceive(NX_UDP_SOCKET *socket_ptr);
    bool AddrIsBroadcast(const NXD_ADDRESS *addrToCheck) const;

   private:
    void init();
    bool initialized = false;

    NX_IP client_ip{};
    NX_AUTO_IP auto_ip{};
    NX_DHCP dhcp_client{};
    NX_DHCPV6 dhcpv6_client{};
    NX_PACKET_POOL client_pool{};
    NX_MDNS mdns{};
    NXD_ADDRESS ipv4{};
    NXD_ADDRESS ipv4mask{};
    NXD_ADDRESS ipv6{};
    ULONG ipv6prefix = 0;
    NX_UDP_SOCKET artnet_socket{};

    uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed) const;

#ifndef BOOTLOADER
    static constexpr char hostname_base[] = "lightkraken-";
#else   // #ifndef BOOTLOADER
    static constexpr char hostname_base[] = "lightkraken-bootloader-";
#endif  // #ifndef BOOTLOADER
    static constexpr char hex_table[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    };
    static constexpr size_t id_length = 8;

    char hostname[sizeof(hostname_base) + id_length + 1] = {};
    uint8_t macaddr[6] = {};
};

#endif  // #ifndef _NETWORK_H_
