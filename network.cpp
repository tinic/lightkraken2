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

#ifndef BOOTLOADER
#include <emio/format.hpp>
#endif  // #ifndef BOOTLOADER

#include "nx_stm32_eth_driver.h"
#include "settingsdb.h"
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
    for (size_t c = 0; c < id_length; c++) {
        hostname[c + sizeof(hostname_base) - 1] = hex_table[(unique_id >> (32 - ((c + 1) * 4))) & 0xF];
    }

#ifndef BOOTLOADER
    emio::static_buffer<64> macAddr{};
    emio::format_to(macAddr, "{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]).value();
    SettingsDB::instance().setString(SettingsDB::kMacAddress, macAddr.str().c_str());

    SettingsDB::instance().setString(SettingsDB::kHostname, hostname);
    printf("Hostname: '%s'\n", hostname);
#endif  // #ifndef BOOTLOADER
}

#ifndef BOOTLOADER
static void dhcpv6_state_change(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT old_state, UINT new_state) {
    NX_PARAMETER_NOT_USED(dhcpv6_ptr);
    NX_PARAMETER_NOT_USED(new_state);

    switch (new_state) {
        case NX_DHCPV6_STATE_INIT: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_INIT");
        } break;
        case NX_DHCPV6_STATE_SENDING_SOLICIT: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_SOLICIT");
        } break;
        case NX_DHCPV6_STATE_SENDING_REQUEST: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_REQUEST");
        } break;
        case NX_DHCPV6_STATE_SENDING_RENEW: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_RENEW");
        } break;
        case NX_DHCPV6_STATE_SENDING_REBIND: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_REBIND");
        } break;
        case NX_DHCPV6_STATE_SENDING_DECLINE: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_DECLINE");
        } break;
        case NX_DHCPV6_STATE_SENDING_CONFIRM: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_CONFIRM");
        } break;
        case NX_DHCPV6_STATE_SENDING_INFORM_REQUEST: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_INFORM_REQUEST");
        } break;
        case NX_DHCPV6_STATE_SENDING_RELEASE: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_SENDING_RELEASE");
        } break;
        case NX_DHCPV6_STATE_BOUND_TO_ADDRESS: {
            printf("dhcpv6_state_change NX_DHCPV6_STATE_BOUND_TO_ADDRESS");
        } break;
    }
}

static void dhcpv6_server_error(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT op_code, UINT status_code, UINT message_type) {
    NX_PARAMETER_NOT_USED(dhcpv6_ptr);
    NX_PARAMETER_NOT_USED(op_code);
    NX_PARAMETER_NOT_USED(status_code);
    NX_PARAMETER_NOT_USED(message_type);
    printf("dhcpv6_server_error %08x %08x %08x\n", op_code, status_code, message_type);
}
#endif  // #ifndef BOOTLOADER

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

static void client_ip_address_changed(NX_IP *ip_ptr, VOID *user) {
#ifndef BOOTLOADER
    NXD_ADDRESS ipv4{};
    NXD_ADDRESS mask{};
    ipv4.nxd_ip_version = NX_IP_VERSION_V4;
    mask.nxd_ip_version = NX_IP_VERSION_V4;
    if (nx_ip_address_get(ip_ptr, &ipv4.nxd_ip_address.v4, &mask.nxd_ip_address.v4) == NX_SUCCESS) {
        SettingsDB::instance().setIP(SettingsDB::kActiveIPv4, &ipv4);
        SettingsDB::instance().setIP(SettingsDB::kActiveIPv4NetMask, &mask);
        char ipv4str[64];
        SettingsDB::instance().getString(SettingsDB::kActiveIPv4, ipv4str, sizeof(ipv4str));
        char ipv4maskstr[64];
        SettingsDB::instance().getString(SettingsDB::kActiveIPv4NetMask, ipv4maskstr, sizeof(ipv4maskstr));
        printf("IPv4: addr(%s) mask(%s)\n", ipv4str, ipv4maskstr);
    }

    NXD_ADDRESS ipv6{};
    ULONG prefix = 0;
    UINT interface = 0;
    for (size_t c = 0;; c++) {
        if (nxd_ipv6_address_get(ip_ptr, c, &ipv6, &prefix, &interface) == NX_SUCCESS) {
            SettingsDB::instance().setIP(SettingsDB::kActiveIPv6, &ipv6);
            SettingsDB::instance().setNumber(SettingsDB::kActiveIPv6PrefixLen, float(prefix));
            char ipv6str[64];
            SettingsDB::instance().getString(SettingsDB::kActiveIPv6, ipv6str, sizeof(ipv6str));
            printf("IPv6: idx(%d) addr(%s) prefix(%d)\n", int(c), ipv6str, int(prefix));
        } else {
            break;
        }
    }
#endif  // #ifndef BOOTLOADER
}

extern "C" VOID nx_stm32_eth_driver(NX_IP_DRIVER *driver_req_ptr);

uint8_t *Network::setup(uint8_t *pointer) {
    UINT status = 0;

    const size_t ip_stack_size = 2048;
    const size_t auto_ip_stack_size = 1024;
#ifndef BOOTLOADER
    const size_t mdns_stack_size = 2048;
    const size_t mdns_service_cache_size = 2048;
    const size_t mdns_peer_service_cache_size = 2048;
    const size_t dhcpv6_client_stack_size = 2048;
#endif  // #ifndef BOOTLOADER
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

    status = nx_tcp_enable(&client_ip);
    if (status) goto fail;

#ifndef BOOTLOADER
    status = nx_udp_enable(&client_ip);
    if (status) goto fail;

    status = nx_igmp_enable(&client_ip);
    if (status) goto fail;

    status = nxd_ipv6_enable(&client_ip);
    if (status) goto fail;

    status =
        nx_mdns_create(&mdns, &client_ip, &client_pool, 3, pointer, mdns_stack_size, (UCHAR *)hostname, (VOID *)(pointer + mdns_service_cache_size),
                       mdns_service_cache_size, (VOID *)(pointer + mdns_service_cache_size + mdns_peer_service_cache_size), mdns_peer_service_cache_size, NULL);
    pointer = pointer + mdns_stack_size + mdns_service_cache_size + mdns_peer_service_cache_size;
    if (status) goto fail;

    status = nx_dhcpv6_client_create(&dhcpv6_client, &client_ip, (CHAR *)hostname, &client_pool, pointer, dhcpv6_client_stack_size, dhcpv6_state_change,
                                     dhcpv6_server_error);
    pointer = pointer + dhcpv6_client_stack_size;
    if (status) goto fail;

#endif  // #ifndef BOOTLOADER

    status = nx_ip_address_change_notify(&client_ip, client_ip_address_changed, 0);
    if (status) goto fail;

    return pointer;

fail:
    while (1) {
    }
}

bool Network::start() {
    UINT status = 0;
    ULONG actual_status = 0;

    volatile bool got_ip = false;
#ifndef BOOTLOADER
    volatile bool got_ipv6 = false;
#endif  // #ifndef BOOTLOADER

    bool try_dhcp = true;
#ifndef BOOTLOADER
    bool try_dhcpv6 = true;
#endif  // #ifndef BOOTLOADER
    bool try_autop = true;
    bool dhcp_status_check = false;

#ifndef BOOTLOADER
    bool try_settings = true;
#endif  // #ifndef BOOTLOADER

    /* Wait for the link to come up.  */
    do {
        status = nx_ip_status_check(&client_ip, NX_IP_LINK_ENABLED, &actual_status, NX_IP_PERIODIC_RATE);
        tx_thread_sleep(NX_IP_PERIODIC_RATE / 10);
    } while (status != NX_SUCCESS);

#ifndef BOOTLOADER

    // Create Link local ipv6 address
    status = nxd_ipv6_address_set(&client_ip, 0, NX_NULL, 10, NULL);
    if (status) {
        return false;
    }

    if (!got_ip && try_settings) {
        NXD_ADDRESS v4addr = {};
        NXD_ADDRESS v4mask = {};
        NXD_ADDRESS v4zero = {};
        SettingsDB::instance().getIP(SettingsDB::kUserIPv4, &v4addr, &v4zero);
        SettingsDB::instance().getIP(SettingsDB::kUserIPv4NetMask, &v4mask, &v4zero);
        if (v4addr.nxd_ip_version == NX_IP_VERSION_V4 && v4addr.nxd_ip_address.v4 != 0 && v4mask.nxd_ip_address.v4 != 0) {
            nx_ip_address_set(&client_ip, v4addr.nxd_ip_address.v4, v4mask.nxd_ip_address.v4);
            got_ip = true;
        }
    }
    if (!got_ipv6 && try_settings) {
        NXD_ADDRESS v6addr = {};
        NXD_ADDRESS v6zero = {};
        float prefix_length = 0;
        SettingsDB::instance().getIP(SettingsDB::kUserIPv6, &v6addr, &v6zero);
        SettingsDB::instance().getNumber(SettingsDB::kUserIPv6PrefixLen, &prefix_length, 0);
        if (v6addr.nxd_ip_version == NX_IP_VERSION_V6 && v6addr.nxd_ip_address.v6[0] != 0 && v6addr.nxd_ip_address.v6[1] != 0 &&
            v6addr.nxd_ip_address.v6[2] != 0 && v6addr.nxd_ip_address.v6[3] != 0 && prefix_length != 0) {
            nxd_ipv6_address_set(&client_ip, 0, &v6addr, ULONG(prefix_length), NULL);
            got_ipv6 = true;
        }
    }
#endif  // #ifndef BOOTLOADER

#ifndef BOOTLOADER

#define DHCPV6_IANA_ID 0x1ED51ED5
#define DHCPV6_T1 NX_DHCPV6_INFINITE_LEASE
#define DHCPV6_T2 NX_DHCPV6_INFINITE_LEASE
#define DHCPV6_RENEW_TIME NX_DHCPV6_INFINITE_LEASE
#define DHCPV6_REBIND_TIME NX_DHCPV6_INFINITE_LEASE

    if (!got_ipv6 && try_dhcpv6) {
        status = nx_dhcpv6_create_client_duid(&dhcpv6_client, NX_DHCPV6_DUID_TYPE_LINK_TIME, NX_DHCPV6_CLIENT_HARDWARE_TYPE_ETHERNET, 0);
        if (status) {
            return false;
        }

        status = nx_dhcpv6_create_client_iana(&dhcpv6_client, DHCPV6_IANA_ID, DHCPV6_T1, DHCPV6_T2);
        if (status) {
            return false;
        }

        status = nx_dhcpv6_start(&dhcpv6_client);
        if (status) {
            return false;
        }

        status = nx_dhcpv6_request_solicit(&dhcpv6_client);
        if (status) {
            return false;
        }

        dhcp_status_check = true;
    }
#endif  // #ifndef BOOTLOADER

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

        dhcp_status_check = true;
    }

    if (dhcp_status_check) {
        status = nx_ip_status_check(&client_ip, NX_IP_ADDRESS_RESOLVED, (ULONG *)&actual_status, NX_IP_PERIODIC_RATE * 60);
        if (status == NX_SUCCESS) {
            got_ip = true;
        } else {
            nx_dhcp_stop(&dhcp_client);
#ifndef BOOTLOADER
            nx_dhcpv6_stop(&dhcpv6_client);
#endif  // #ifndef BOOTLOADER
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

#ifndef BOOTLOADER
    if (got_ip || got_ipv6) {
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
#endif  // #ifndef BOOTLOADER

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
