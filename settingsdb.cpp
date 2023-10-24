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
#include "settingsdb.h"

#ifndef BOOTLOADER

#include <stdlib.h>
#include <string.h>

#include <emio/buffer.hpp>
#include <emio/format.hpp>

#include "ipv6.h"
#include "stm32h5xx_hal.h"
#include "webserver.h"

namespace emio {

template <size_t CacheSize = default_cache_size>
class packet_buffer final : public buffer {
   public:
    constexpr explicit packet_buffer(NX_PACKET *packet) noexcept : resp_packet(packet), cache_{} { this->set_write_area(cache_); }

    packet_buffer(const packet_buffer &) = delete;
    packet_buffer(packet_buffer &&) = delete;
    packet_buffer &operator=(const packet_buffer &) = delete;
    packet_buffer &operator=(packet_buffer &&) = delete;
    ~packet_buffer() override = default;

    result<void> flush() noexcept {
        if (this->get_used_count() == 0) {
            return success;
        }
        UINT status = 0;
        if (resp_packet == 0) {
            status = nx_packet_allocate(WebServer::instance().httpServer()->nx_http_server_packet_pool_ptr, &resp_packet, NX_TCP_PACKET, NX_WAIT_FOREVER);
            if (status != NX_SUCCESS) {
                while (1) {
                }
            }
        }
        status = nx_packet_data_append(resp_packet, cache_.data(), this->get_used_count(), WebServer::instance().httpServer()->nx_http_server_packet_pool_ptr,
                                       NX_WAIT_FOREVER);
        if (status != NX_SUCCESS) {
            while (1) {
            }
        }
        status = nx_tcp_socket_send(&(WebServer::instance().httpServer()->nx_http_server_socket), resp_packet, NX_HTTP_SERVER_TIMEOUT_SEND);
        if (status != NX_SUCCESS) {
            while (1) {
            }
        }
        nx_packet_release(resp_packet);
        resp_packet = 0;
        this->set_write_area(cache_);
        return success;
    }

   protected:
    result<std::span<char>> request_write_area(const size_t /*used*/, const size_t size) noexcept override {
        EMIO_TRYV(flush());
        const std::span<char> area{cache_};
        this->set_write_area(area);
        if (size > cache_.size()) {
            return area;
        }
        return area.subspan(0, size);
    }

   private:
    NX_PACKET *resp_packet;
    std::array<char, CacheSize> cache_;
};
}  // namespace emio

SettingsDB &SettingsDB::instance() {
    static SettingsDB settingsDB;
    if (!settingsDB.initialized) {
        settingsDB.initialized = true;
        settingsDB.init();
    }
    return settingsDB;
}

void SettingsDB::erase() {
    nor_flash0.ops.erase(0, FLASH_DB_LENGTH);
    printf("SettingsDB: Database erased!\n");
}

void SettingsDB::init() {
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);

    float boot_count = 0;

    const char *ipv4 = "0.0.0.0";
    const char *ipv6 = "::";
    const float ipv6_prefix_length = 0;

    struct fdb_default_kv default_kv;
    static struct fdb_default_kv_node default_kv_table[] = {
        {(char *)(SettingsDB::kBootCount_t), (void *)&boot_count, sizeof(boot_count)},

        {(char *)(SettingsDB::kActiveIPv4_t), (void *)ipv4, strlen(ipv4)},
        {(char *)(SettingsDB::kActiveIPv4NetMask_t), (void *)ipv4, strlen(ipv4)},
        {(char *)(SettingsDB::kActiveIPv6_t), (void *)ipv6, strlen(ipv6)},
        {(char *)(SettingsDB::kActiveIPv6PrefixLen_t), (void *)&ipv6_prefix_length, sizeof(ipv6_prefix_length)},

        {(char *)(SettingsDB::kUserIPv4_t), (void *)ipv4, strlen(ipv4)},
        {(char *)(SettingsDB::kUserIPv4NetMask_t), (void *)ipv4, strlen(ipv4)},
        {(char *)(SettingsDB::kUserIPv6_t), (void *)ipv6, strlen(ipv6)},
        {(char *)(SettingsDB::kUserIPv6PrefixLen_t), (void *)&ipv6_prefix_length, sizeof(ipv6_prefix_length)},
    };
    default_kv.kvs = default_kv_table;
    default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

    fdb_err_t result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
    if (result != FDB_NO_ERR) {
        while (1) {
        }
    }

    struct fdb_blob blob {};
    fdb_kv_get_blob(&kvdb, SettingsDB::kBootCount_t, fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
    boot_count++;
    fdb_kv_set_blob(&kvdb, SettingsDB::kBootCount_t, fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
}

void SettingsDB::lock() { __disable_irq(); }

void SettingsDB::unlock() { __enable_irq(); }

UINT SettingsDB::jsonGETRequest(NX_PACKET *packet_ptr) {
    nx_packet_release(packet_ptr);

    auto toBuffer = [this](emio::buffer &buf) {
        emio::format_to(buf, "{{").value();
        struct fdb_kv_iterator iterator {};
        fdb_kv_iterator_init(&kvdb, &iterator);
        const char *comma = "";
        while (fdb_kv_iterate(&kvdb, &iterator)) {
            fdb_kv_t cur_kv = &(iterator.curr_kv);
            size_t data_size = (size_t)cur_kv->value_len;
            struct fdb_blob blob {};

            size_t name_len = strlen(cur_kv->name);
            if (name_len > 2 && cur_kv->name[name_len - 2] == '@') {
                char name_buf[256];
                strcpy(name_buf, cur_kv->name);
                name_buf[name_len - 2] = 0;
                switch (cur_kv->name[name_len - 1]) {
                    case KEY_TYPE_STRING_CHAR: {
                        char data_buf[256];
                        data_buf[data_size] = 0;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, data_buf, data_size)));
                        emio::format_to(buf, "{}'{}':'{}'", comma, name_buf, data_buf).value();
                        comma = ",";
                    } break;
                    case KEY_TYPE_BOOL_CHAR: {
                        bool value = false;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                        emio::format_to(buf, "{}'{}':{}", comma, name_buf, (value ? "true" : "false")).value();
                        comma = ",";
                    } break;
                    case KEY_TYPE_NUMBER_CHAR: {
                        float value = 0;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                        emio::format_to(buf, "{}'{}':{}", comma, name_buf, value).value();
                        comma = ",";
                    } break;
                    case KEY_TYPE_NULL_CHAR: {
                        char value = 0;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                        emio::format_to(buf, "{}'{}':null", comma, name_buf).value();
                        comma = ",";
                    } break;
                    default:
                        break;
                }
            }
        }
        emio::format_to(buf, "}}").value();
    };

    emio::detail::counting_buffer<1024> cbuf{};
    toBuffer(cbuf);

    UINT status = 0;
    NX_PACKET *resp_packet_ptr = 0;
    const char *jsonContentType = "application/json";
    status = nx_http_server_callback_generate_response_header_extended(WebServer::instance().httpServer(), &resp_packet_ptr, (CHAR *)NX_HTTP_STATUS_OK,
                                                                       sizeof(NX_HTTP_STATUS_OK) - 1, cbuf.count(), (CHAR *)jsonContentType,
                                                                       strlen(jsonContentType), NX_NULL, 0);
    if (status != NX_SUCCESS) {
        while (1) {
        }
    }

    emio::packet_buffer<1024> pbuf(resp_packet_ptr);
    toBuffer(pbuf);
    auto success = pbuf.flush();
    (void)success;

    return (NX_HTTP_CALLBACK_COMPLETED);
}

void SettingsDB::jsonStreamSettingsCallback(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type) { SettingsDB::instance().jsonStreamSettings(jsp, type); }

void SettingsDB::jsonStreamSettings(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type) {
    if (jsp == NULL) {
        return;
    }

    static bool in_array = false;

    // Top level values only, no nesting
    if (!((jsp->stack_pos == 2) || (jsp->stack_pos == 3 && in_array))) {
        return;
    }

    const char *key_name = jsp->stack[jsp->stack_pos - 1].meta.name;
    const char *data_buf = jsp->data.str.buff;

    switch (type) {
        case LWJSON_STREAM_TYPE_STRING:
            SettingsDB::instance().setString(key_name, data_buf);
            break;
        case LWJSON_STREAM_TYPE_TRUE:
            SettingsDB::instance().setBool(key_name, true);
            break;
        case LWJSON_STREAM_TYPE_FALSE:
            SettingsDB::instance().setBool(key_name, false);
            break;
        case LWJSON_STREAM_TYPE_NULL:
            SettingsDB::instance().setNull(key_name);
            break;
        case LWJSON_STREAM_TYPE_NUMBER: {
            SettingsDB::instance().setNumber(key_name, strtof(data_buf, NULL));
        } break;
        case LWJSON_STREAM_TYPE_NONE:
        case LWJSON_STREAM_TYPE_KEY:
        case LWJSON_STREAM_TYPE_OBJECT:
        case LWJSON_STREAM_TYPE_OBJECT_END:
        case LWJSON_STREAM_TYPE_ARRAY: {
            in_array = true;
        } break;
        case LWJSON_STREAM_TYPE_ARRAY_END: {
            in_array = false;
        } break;
        default:
            // not supported
            break;
    }
}

UINT SettingsDB::jsonPUTRequest(NX_PACKET *packet_ptr) {
    ULONG contentLength = 0;
    UINT status = nx_http_server_packet_content_find(WebServer::instance().httpServer(), &packet_ptr, &contentLength);
    if (status) {
        nx_packet_release(packet_ptr);
        nx_http_server_callback_response_send_extended(WebServer::instance().httpServer(), (CHAR *)NX_HTTP_STATUS_REQUEST_TIMEOUT,
                                                       sizeof(NX_HTTP_STATUS_REQUEST_TIMEOUT) - 1, NX_NULL, 0, NX_NULL, 0);
        return (NX_HTTP_CALLBACK_COMPLETED);
    }
    if (contentLength == 0) {
        nx_packet_release(packet_ptr);
        nx_http_server_callback_response_send_extended(WebServer::instance().httpServer(), (CHAR *)NX_HTTP_STATUS_NO_CONTENT,
                                                       sizeof(NX_HTTP_STATUS_NO_CONTENT) - 1, NX_NULL, 0, NX_NULL, 0);
        return (NX_HTTP_CALLBACK_COMPLETED);
    }
    lwjson_stream_parser_t stream_parser;
    lwjson_stream_init(&stream_parser, jsonStreamSettingsCallback);
    ULONG contentOffset = 0;
    bool done = false;
    do {
        UCHAR *jsonBuf = packet_ptr->nx_packet_prepend_ptr;
        ULONG jsonLen = ULONG(packet_ptr->nx_packet_append_ptr - packet_ptr->nx_packet_prepend_ptr);
        for (size_t c = 0; c < jsonLen; c++) {
            lwjsonr_t res = lwjson_stream_parse(&stream_parser, jsonBuf[c]);
            if (res == lwjsonSTREAMINPROG || res == lwjsonSTREAMWAITFIRSTCHAR || res == lwjsonOK) {
                // NOP
            } else if (res == lwjsonSTREAMDONE) {
                done = true;
                break;
            } else {
                nx_packet_release(packet_ptr);
                nx_http_server_callback_response_send_extended(WebServer::instance().httpServer(), (CHAR *)NX_HTTP_STATUS_BAD_REQUEST,
                                                               sizeof(NX_HTTP_STATUS_BAD_REQUEST) - 1, NX_NULL, 0, NX_NULL, 0);
                return (NX_HTTP_CALLBACK_COMPLETED);
            }
        }
        contentOffset += jsonLen;
        if (!done) {
            done = contentOffset >= contentLength;
        }
        if (!done) {
            nx_packet_release(packet_ptr);
            status = nx_tcp_socket_receive(&(WebServer::instance().httpServer()->nx_http_server_socket), &packet_ptr, NX_HTTP_SERVER_TIMEOUT_RECEIVE);
            if (status) {
                nx_http_server_callback_response_send_extended(WebServer::instance().httpServer(), (CHAR *)NX_HTTP_STATUS_REQUEST_TIMEOUT,
                                                               sizeof(NX_HTTP_STATUS_REQUEST_TIMEOUT) - 1, NX_NULL, 0, NX_NULL, 0);
                return (NX_HTTP_CALLBACK_COMPLETED);
            }
        }
    } while (!done);
    nx_packet_release(packet_ptr);
    nx_http_server_callback_response_send_extended(WebServer::instance().httpServer(), (CHAR *)NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, NX_NULL, 0,
                                                   NX_NULL, 0);
    return (NX_HTTP_CALLBACK_COMPLETED);
}

size_t SettingsDB::getString(const char *key, char *value, size_t maxlen, const char *default_value) {
    if (!value || !key) {
        return 0;
    }
    char keyS[256]{};
    strncpy(keyS, key, sizeof(keyS) - 3);
    strcat(keyS, KEY_TYPE_STRING);
    struct fdb_blob blob {};
    size_t len = 0;
    if ((len = fdb_kv_get_blob(&kvdb, keyS, fdb_blob_make(&blob, reinterpret_cast<const void *>(value), maxlen))) > 0) {
        value[maxlen - 1] = 0;
        return len;
    }
    if (default_value) {
        strncpy(value, default_value, maxlen);
        value[maxlen - 1] = 0;
        return len;
    }
    return 0;
}

bool SettingsDB::getBool(const char *key, bool *value, bool default_value) {
    if (!value || !key) {
        return 0;
    }
    char keyB[256]{};
    strncpy(keyB, key, sizeof(keyB) - 3);
    strcat(keyB, KEY_TYPE_BOOL);
    struct fdb_blob blob {};
    if (fdb_kv_get_blob(&kvdb, keyB, fdb_blob_make(&blob, reinterpret_cast<const void *>(value), sizeof(bool))) == sizeof(bool)) {
        return true;
    }
    *value = default_value;
    return false;
}

bool SettingsDB::getNumber(const char *key, float *value, float default_value) {
    if (!value || !key) {
        return 0;
    }
    char keyF[256]{};
    strncpy(keyF, key, sizeof(keyF) - 3);
    strcat(keyF, KEY_TYPE_NUMBER);
    struct fdb_blob blob {};
    if (fdb_kv_get_blob(&kvdb, keyF, fdb_blob_make(&blob, reinterpret_cast<const void *>(value), sizeof(float))) == sizeof(float)) {
        return true;
    }
    *value = default_value;
    return false;
}

bool SettingsDB::getNull(const char *key) {
    if (!key) {
        return 0;
    }
    char keyN[256]{};
    strncpy(keyN, key, sizeof(keyN) - 3);
    strcat(keyN, KEY_TYPE_NULL);
    char value = 0;
    struct fdb_blob blob {};
    if (fdb_kv_get_blob(&kvdb, keyN, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(char))) == sizeof(char)) {
        return true;
    }
    return false;
}

bool SettingsDB::getIP(const char *key, NXD_ADDRESS *value, NXD_ADDRESS *default_value) {
    if (!value || !key) {
        return 0;
    }
    char keyS[256]{};
    strncpy(keyS, key, sizeof(keyS) - 3);
    strcat(keyS, KEY_TYPE_STRING);
    struct fdb_blob blob {};
    size_t len = 0;
    char ipStr[64] = {};
    if ((len = fdb_kv_get_blob(&kvdb, keyS, fdb_blob_make(&blob, reinterpret_cast<const void *>(ipStr), sizeof(ipStr)))) > 0) {
        ipStr[sizeof(ipStr) - 1] = 0;
        ipv6_address_full_t ip{};
        if (ipv6_from_str(ipStr, len, &ip)) {
            if ((ip.flags & IPV6_FLAG_IPV4_COMPAT) != 0) {
                value->nxd_ip_address.v4 = (uint32_t(ip.address.components[0]) << 16) | (uint32_t(ip.address.components[1]) << 0);
                value->nxd_ip_version = NX_IP_VERSION_V4;
            } else {
                value->nxd_ip_address.v6[0] = (uint32_t(ip.address.components[0]) << 16) | (uint32_t(ip.address.components[1]) << 0);
                value->nxd_ip_address.v6[1] = (uint32_t(ip.address.components[2]) << 16) | (uint32_t(ip.address.components[4]) << 0);
                value->nxd_ip_address.v6[2] = (uint32_t(ip.address.components[4]) << 16) | (uint32_t(ip.address.components[5]) << 0);
                value->nxd_ip_address.v6[3] = (uint32_t(ip.address.components[6]) << 16) | (uint32_t(ip.address.components[7]) << 0);
                value->nxd_ip_version = NX_IP_VERSION_V6;
            }
            return true;
        }
    }
    if (default_value) {
        *value = *default_value;
    }
    return false;
}

void SettingsDB::setString(const char *key, const char *str) {
    if (!str || !key) {
        return;
    }
    char checkStr[256]{};
    if (getString(key, checkStr, 256)) {
        if (strcmp(str, checkStr) == 0) {
            return;
        }
    }
    char keyS[256]{};
    strncpy(keyS, key, sizeof(keyS) - 3);
    strcat(keyS, KEY_TYPE_STRING);
    fdb_kv_set(&kvdb, keyS, str);
}

void SettingsDB::setBool(const char *key, bool value) {
    if (!key) {
        return;
    }
    bool checkBool = false;
    if (getBool(key, &checkBool)) {
        if (value == checkBool) {
            return;
        }
    }
    char keyB[256]{};
    strncpy(keyB, key, sizeof(keyB) - 3);
    strcat(keyB, KEY_TYPE_BOOL);
    struct fdb_blob blob {};
    fdb_kv_set_blob(&kvdb, keyB, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(value)));
}

void SettingsDB::setNumber(const char *key, float value) {
    if (!key) {
        return;
    }
    float checkNumber = false;
    if (getNumber(key, &checkNumber)) {
        if (value == checkNumber) {
            return;
        }
    }
    char keyF[256]{};
    strncpy(keyF, key, sizeof(keyF) - 3);
    strcat(keyF, KEY_TYPE_NUMBER);
    struct fdb_blob blob {};
    fdb_kv_set_blob(&kvdb, keyF, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(value)));
}

void SettingsDB::setNull(const char *key) {
    if (!key) {
        return;
    }
    if (getNull(key)) {
        return;
    }
    char keyN[256]{};
    strncpy(keyN, key, sizeof(keyN) - 3);
    strcat(keyN, KEY_TYPE_NULL);
    char value = 0;
    struct fdb_blob blob {};
    fdb_kv_set_blob(&kvdb, keyN, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(value)));
}

void SettingsDB::setIP(const char *key, const NXD_ADDRESS *value) {
    if (!value || !key) {
        return;
    }

    char keyS[256]{};
    strncpy(keyS, key, sizeof(keyS) - 3);
    strcat(keyS, KEY_TYPE_STRING);
    struct fdb_blob blob {};

    char ip_str[64] = {};
    ipv6_address_full_t ip{};
    if (value->nxd_ip_version == NX_IP_VERSION_V4) {
        ip.address.components[0] = uint16_t((value->nxd_ip_address.v4 >> 16) & 0xFFFF);
        ip.address.components[1] = uint16_t((value->nxd_ip_address.v4 >> 0) & 0xFFFF);
        ip.flags = IPV6_FLAG_IPV4_COMPAT;
    } else {
        ip.address.components[0] = uint16_t((value->nxd_ip_address.v6[0] >> 16) & 0xFFFF);
        ip.address.components[1] = uint16_t((value->nxd_ip_address.v6[0] >> 0) & 0xFFFF);
        ip.address.components[2] = uint16_t((value->nxd_ip_address.v6[1] >> 16) & 0xFFFF);
        ip.address.components[3] = uint16_t((value->nxd_ip_address.v6[1] >> 0) & 0xFFFF);
        ip.address.components[4] = uint16_t((value->nxd_ip_address.v6[2] >> 16) & 0xFFFF);
        ip.address.components[5] = uint16_t((value->nxd_ip_address.v6[2] >> 0) & 0xFFFF);
        ip.address.components[6] = uint16_t((value->nxd_ip_address.v6[3] >> 16) & 0xFFFF);
        ip.address.components[7] = uint16_t((value->nxd_ip_address.v6[3] >> 0) & 0xFFFF);
    }

    ipv6_to_str(&ip, ip_str, sizeof(ip_str));
    char checkStr[64]{};
    if (getString(key, checkStr, 64)) {
        if (strcmp(ip_str, checkStr) == 0) {
            return;
        }
    }
    fdb_kv_set_blob(&kvdb, keyS, fdb_blob_make(&blob, reinterpret_cast<const void *>(ip_str), sizeof(ip_str)));
}

#endif  // #ifndef BOOTLOADER
