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
#include "network.h"

#include "nx_stm32_eth_driver.h"
#include "stm32h5xx_hal.h"

#define NX_PACKET_POOL_SIZE (ETH_MAX_PACKET_SIZE * 32)

Network &Network::instance() {
    static Network network;
    if (!network.initialized) {
        network.initialized = true;
        network.init();
    }
    return network;
}

extern "C" const uint8_t *networkMACAddr(void);

const uint8_t *networkMACAddr(void) { return Network::instance().MACAddr(); }

void Network::init() {
    if (HAL_ICACHE_Disable() != 0) {
        while (1) {
        }
    }

    uint32_t uid[3];
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();

    if (HAL_ICACHE_Enable() != 0) {
        while (1) {
        }
    }

    uint32_t unique_id = murmur3_32(reinterpret_cast<uint8_t *>(&uid[0]), sizeof(uid), 0x66cf8031);
    macaddr[0] = 0x1E;
    macaddr[1] = 0xD5;
    macaddr[2] = uint8_t((unique_id >> 24) & 0xFF);
    macaddr[3] = uint8_t((unique_id >> 16) & 0xFF);
    macaddr[4] = uint8_t((unique_id >> 8) & 0xFF);
    macaddr[5] = uint8_t((unique_id >> 0) & 0xFF);

    memset(hostname, 0, sizeof(hostname));
    strcpy(hostname, hostname_base);
    for (size_t c = 0; c < 8; c++) {
        hostname[c + sizeof(hostname_base) - 1] = hex_table[(unique_id >> (32 - ((c + 1) * 4))) & 0xF];
    }
}

static void client_ip_address_changed(NX_IP *ip_ptr, VOID *user) {
    ULONG ip_address = 0;
    ULONG network_mask = 0;
    nx_ip_address_get(ip_ptr, &ip_address, &network_mask);
    printf("client_ip_address_changed %08x %08x\n", ip_address, network_mask);
}

extern "C" VOID nx_stm32_eth_driver(NX_IP_DRIVER *driver_req_ptr);

uint8_t *Network::setup(uint8_t *pointer) {
    UINT status = 0;

    const size_t ip_stack_size = 2048;
    const size_t auto_ip_stack_size = 1024;
    const size_t mdns_stack_size = 2048;
    const size_t mdns_service_cache_size = 2048;
    const size_t mdns_peer_service_cache_size = 2048;
    const size_t arp_cache_size = 2048;

    nx_system_initialize();

    status = nx_packet_pool_create(&client_pool, (CHAR *)"NetX Main Packet Pool", ETH_MAX_PACKET_SIZE, pointer, NX_PACKET_POOL_SIZE);
    pointer = pointer + NX_PACKET_POOL_SIZE;
    if (status) goto fail;

    status = nx_ip_create(&client_ip, (CHAR *)hostname, IP_ADDRESS(0, 0, 0, 0), 0xFFFFFF00UL, &client_pool, nx_stm32_eth_driver, pointer, ip_stack_size, 1);
    pointer = pointer + ip_stack_size;
    if (status) goto fail;

    status = nx_ip_interface_mtu_set(&client_ip, 0, ETH_MAX_PAYLOAD);
    if (status) goto fail;

    status = nx_auto_ip_create(&auto_ip, (CHAR *)hostname, &client_ip, pointer, auto_ip_stack_size, 1);
    pointer = pointer + auto_ip_stack_size;
    if (status) goto fail;

    status = nx_arp_enable(&client_ip, (void *)pointer, arp_cache_size);
    pointer = pointer + arp_cache_size;
    if (status) goto fail;

    status = nx_icmp_enable(&client_ip);
    if (status) goto fail;

#ifndef BOOTLOADER
    status = nx_udp_enable(&client_ip);
    if (status) goto fail;
#endif  // #ifndef BOOTLOADER

    status = nx_tcp_enable(&client_ip);
    if (status) goto fail;

#ifndef BOOTLOADER
    status = nx_igmp_enable(&client_ip);
    if (status) goto fail;

    status = nxd_ipv6_enable(&client_ip);
    if (status) goto fail;
#endif  // #ifndef BOOTLOADER

    status =
        nx_mdns_create(&mdns, &client_ip, &client_pool, 3, pointer, mdns_stack_size, (UCHAR *)hostname, (VOID *)(pointer + mdns_service_cache_size),
                       mdns_service_cache_size, (VOID *)(pointer + mdns_service_cache_size + mdns_peer_service_cache_size), mdns_peer_service_cache_size, NULL);
    if (status) goto fail;

    pointer = pointer + mdns_stack_size + mdns_service_cache_size + mdns_peer_service_cache_size;

    status = nx_ip_address_change_notify(&client_ip, client_ip_address_changed, 0);
    if (status) goto fail;

    return pointer;

fail:
    while (1) {
    }
}

static void dhcp_state_change(NX_DHCP *dhcp_ptr, UCHAR new_state) {
    NX_PARAMETER_NOT_USED(dhcp_ptr);

    switch (new_state) {
        case NX_DHCP_STATE_NOT_STARTED:
            printf("dhcp_state_change NX_DHCP_STATE_NOT_STARTED\n");
            break;
        case NX_DHCP_STATE_BOOT:
            printf("dhcp_state_change NX_DHCP_STATE_BOOT\n");
            break;
        case NX_DHCP_STATE_INIT:
            printf("dhcp_state_change NX_DHCP_STATE_INIT\n");
            break;
        case NX_DHCP_STATE_SELECTING:
            printf("dhcp_state_change NX_DHCP_STATE_SELECTING\n");
            break;
        case NX_DHCP_STATE_REQUESTING:
            printf("dhcp_state_change NX_DHCP_STATE_REQUESTING\n");
            break;
        case NX_DHCP_STATE_BOUND:
            printf("dhcp_state_change NX_DHCP_STATE_BOUND\n");
            break;
        case NX_DHCP_STATE_RENEWING:
            printf("dhcp_state_change NX_DHCP_STATE_RENEWING\n");
            break;
        case NX_DHCP_STATE_REBINDING:
            printf("dhcp_state_change NX_DHCP_STATE_REBINDING\n");
            break;
        case NX_DHCP_STATE_FORCERENEW:
            printf("dhcp_state_change NX_DHCP_STATE_FORCERENEW\n");
            break;
        case NX_DHCP_STATE_ADDRESS_PROBING:
            printf("dhcp_state_change NX_DHCP_STATE_ADDRESS_PROBING\n");
            break;
    }
}

bool Network::start() {
    UINT status = 0;
    ULONG actual_status = 0;

    volatile bool got_ip = false;

    bool try_dhcp = true;
    bool try_autop = true;
    bool try_settings = true;

    /* Wait for the link to come up.  */
    do {
        status = nx_ip_status_check(&client_ip, NX_IP_LINK_ENABLED, &actual_status, NX_IP_PERIODIC_RATE);
        HAL_Delay(10);
    } while (status != NX_SUCCESS);

    if (!got_ip && try_settings) {
        if (1) {
            nx_ip_address_set(&client_ip, IP_ADDRESS(192, 168, 1, 147), IP_ADDRESS(255, 255, 255, 0));
            got_ip = true;
        }
    }

    if (!got_ip && try_dhcp) {
        /* Create the DHCP instance.  */
        status = nx_dhcp_create(&dhcp_client, &client_ip, (CHAR *)hostname);
        if (status) {
            return false;
        }

        /* Register state change variable.  */
        status = nx_dhcp_state_change_notify(&dhcp_client, dhcp_state_change);
        if (status) {
            return false;
        }

        /* Start the DHCP Client.  */
        status = nx_dhcp_start(&dhcp_client);
        if (status) {
            return false;
        }

        status = nx_ip_status_check(&client_ip, NX_IP_ADDRESS_RESOLVED, (ULONG *)&actual_status, NX_IP_PERIODIC_RATE * 60);
        if (status == NX_SUCCESS) {
            got_ip = true;
        } else {
            nx_dhcp_stop(&dhcp_client);
            if (status == NX_NOT_SUCCESSFUL) {
                printf("No DHCP address available.\n");
            }
        }
    }

    if (!got_ip && try_autop) {
        status = nx_auto_ip_start(&auto_ip, 0);
        if (status) {
            return false;
        }

        status = nx_ip_status_check(&client_ip, NX_IP_ADDRESS_RESOLVED, (ULONG *)&actual_status, NX_IP_PERIODIC_RATE * 60);
        if (status == NX_SUCCESS) {
            got_ip = true;
        } else {
            nx_auto_ip_stop(&auto_ip);
            if (status == NX_NOT_SUCCESSFUL) {
                printf("No AutoIP address available.\n");
            }
            return false;
        }
    }

    if (got_ip) {
        status = nx_mdns_enable(&mdns, 0);
        if (status) {
            return false;
        }

        status = nx_mdns_service_add(&mdns, (UCHAR *)hostname, (UCHAR *)"_http._tcp", NULL, NULL, 120, 0, 0, 80, NX_TRUE, 0);
        if (status) {
            return false;
        }

        status = nx_mdns_service_add(&mdns, (UCHAR *)hostname, (UCHAR *)"_artnet._udp", NULL, NULL, 120, 0, 0, 6454, NX_TRUE, 0);
        if (status) {
            return false;
        }

        status = nx_mdns_service_add(&mdns, (UCHAR *)hostname, (UCHAR *)"_sACN._udp", NULL, NULL, 120, 0, 0, 5568, NX_TRUE, 0);
        if (status) {
            return false;
        }
    }

    return true;
}

uint32_t Network::murmur3_32(const uint8_t *key, size_t len, uint32_t seed) const {
    uint32_t h = seed;
    if (len > 3) {
        size_t i = len >> 2;
        do {
            uint32_t k;
            memcpy(&k, key, sizeof(uint32_t));
            key += sizeof(uint32_t);
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
            h = (h << 13) | (h >> 19);
            h = h * 5 + 0xe6546b64;
        } while (--i);
    }
    if (len & 3) {
        size_t i = len & 3;
        uint32_t k = 0;
        do {
            k <<= 8;
            k |= key[i - 1];
        } while (--i);
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
    }
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}
