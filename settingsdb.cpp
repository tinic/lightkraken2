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

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <functional>
#include <sstream>
#include <string>

#include "stm32h5xx_hal.h"
#include "webserver.h"

class mtustreambuf : public std::streambuf {
   public:
    mtustreambuf(std::function<void(const char *, size_t)> c) : callback(c) {}
    virtual std::streamsize xsputn(const char *s, std::streamsize n) override {
        for (size_t c = 0; c < size_t(n); c += 256) {
            callback(&s[c], c - std::min(size_t(n), c + 256));
        }
        return n;
    }

   private:
    std::function<void(const char *, size_t)> callback;
};

SettingsDB &SettingsDB::instance() {
    static SettingsDB settingsDB;
    if (!settingsDB.initialized) {
        settingsDB.initialized = true;
        settingsDB.init();
    }
    return settingsDB;
}

void SettingsDB::erase() { nor_flash0.ops.erase(0, FLASH_DB_LENGTH); }

void SettingsDB::init() {
    // erase();

    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);

    uint32_t boot_count = 0;

    NXD_ADDRESS last_ipv4{};
    NXD_ADDRESS last_ipv6{};

    struct fdb_default_kv default_kv;
    static struct fdb_default_kv_node default_kv_table[] = {
        {(char *)"boot_count@i", &boot_count, sizeof(boot_count)},
        {(char *)"last_ipv4@a", &last_ipv4, sizeof(last_ipv4)},
        {(char *)"last_ipv6@a", &last_ipv6, sizeof(last_ipv6)},
    };
    default_kv.kvs = default_kv_table;
    default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

    fdb_err_t result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
    if (result != FDB_NO_ERR) {
        while (1) {
        }
    }

    struct fdb_blob blob {};
    fdb_kv_get_blob(&kvdb, "boot_count@i", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
    boot_count++;
    fdb_kv_set_blob(&kvdb, "boot_count@i", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
}

void SettingsDB::lock() { __disable_irq(); }

void SettingsDB::unlock() { __enable_irq(); }

UINT SettingsDB::jsonGETRequest(NX_PACKET *packet_ptr) {
    nx_packet_release(packet_ptr);
    nx_http_server_callback_response_send_extended(WebServer::instance().httpServer(), (CHAR *)NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, NX_NULL, 0,
                                                   NX_NULL, 0);
    return (NX_HTTP_CALLBACK_COMPLETED);

    size_t jsonSize = 0;
    for (size_t pass = 0; pass < 2; pass++) {
        mtustreambuf streambuf([=](const char *data, size_t size) mutable {
            if (pass == 0) {
                jsonSize += size;
            }
            if (pass == 1) {
                //                nx_http_server_callback_generate_response_header(http_server_ptr,
                //                    &resp_packet_ptr, NX_HTTP_STATUS_OK,
                //                    jsonSize, temp_string, NX_NULL);
            }
        });

        std::ostream out(&streambuf);
        out << "{";
        struct fdb_kv_iterator iterator {};
        fdb_kv_iterator_init(&kvdb, &iterator);
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
                    case 's': {
                        uint8_t data_buf[256];
                        data_buf[data_size] = 0;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, data_buf, data_size)));
                        out << "'" << name_buf << "':'" << data_buf << "',";
                    } break;
                    case 'b': {
                        bool value = false;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                        out << "'" << name_buf << "':" << (value ? "true" : "false") << ",";
                    } break;
                    case 'f': {
                        float value = 0;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                        out << "'" << name_buf << "':" << value << ",";
                    } break;
                    case 'n': {
                        char value = 0;
                        fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                        out << "'" << name_buf << "':null,";
                    } break;
                    default:
                        break;
                }
            }
        }
        out << "}";
        out.flush();
    }
}

void SettingsDB::jsonStreamSettingsCallback(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type) { SettingsDB::instance().jsonStreamSettings(jsp, type); }

void SettingsDB::jsonStreamSettings(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type) {
    if (jsp == NULL) {
        return;
    }

    // Top level values only, no nesting
    if (jsp->stack_pos != 2) {
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
        case LWJSON_STREAM_TYPE_ARRAY:
        case LWJSON_STREAM_TYPE_ARRAY_END:
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
    SettingsDB::instance().dump();
    nx_http_server_callback_response_send_extended(WebServer::instance().httpServer(), (CHAR *)NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, NX_NULL, 0,
                                                   NX_NULL, 0);
    return (NX_HTTP_CALLBACK_COMPLETED);
}

void SettingsDB::dump() {
    struct fdb_kv_iterator iterator {};

    printf("Dumping database:\n");

    fdb_kv_iterator_init(&kvdb, &iterator);
    while (fdb_kv_iterate(&kvdb, &iterator)) {
        fdb_kv_t cur_kv = &(iterator.curr_kv);
        size_t data_size = (size_t)cur_kv->value_len;
        struct fdb_blob blob {};

        size_t name_len = strlen(cur_kv->name);
        if (name_len > 2 && cur_kv->name[name_len - 2] == '@') {
            switch (cur_kv->name[name_len - 1]) {
                case 's': {
                    uint8_t data_buf[256];
                    data_buf[data_size] = 0;
                    fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, data_buf, data_size)));
                    printf("String: <%s> <%s>\n", cur_kv->name, data_buf);
                } break;
                case 'b': {
                    bool value = false;
                    fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                    printf("Bool:   <%s> <%s>\n", cur_kv->name, value ? "true" : "false");
                } break;
                case 'f': {
                    float value = 0;
                    fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                    printf("Float:  <%s> <%f>\n", cur_kv->name, double(value));
                } break;
                case 'n': {
                    char value = 0;
                    fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                    printf("Null:   <%s>\n", cur_kv->name);
                } break;
                case 'i': {
                    int32_t value = 0;
                    fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                    printf("Int:    <%s> <%d>\n", cur_kv->name, value);
                } break;
                case 'a': {
                    NXD_ADDRESS value{};
                    fdb_blob_read(reinterpret_cast<fdb_db_t>(&kvdb), fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, &value, sizeof(value))));
                    if (value.nxd_ip_version == NX_IP_VERSION_V4) {
                        printf("Addr:   <%s> <%d.%d.%d.%d>\n", cur_kv->name, (value.nxd_ip_address.v4 >> 24) & 0xFF, (value.nxd_ip_address.v4 >> 16) & 0xFF,
                               (value.nxd_ip_address.v4 >> 8) & 0xFF, (value.nxd_ip_address.v4 >> 0) & 0xFF);
                    } else if (value.nxd_ip_version == NX_IP_VERSION_V6) {
                        printf("Addr:   <%s> <%08x::%08x::%08x::%08x>\n", cur_kv->name, value.nxd_ip_address.v6[0], value.nxd_ip_address.v6[1],
                               value.nxd_ip_address.v6[2], value.nxd_ip_address.v6[3]);
                    } else {
                        printf("Addr:   <%s> <invalid>\n", cur_kv->name);
                    }
                } break;
                default:
                    break;
            }
        }
    }
    printf("Dumping database done.\n");
}

size_t SettingsDB::getString(const char *key, char *value, size_t maxlen) {
    char keyS[256]{};
    strncpy(keyS, key, sizeof(keyS) - 3);
    strcat(keyS, "@s");
    struct fdb_blob blob {};
    size_t len = 0;
    if ((len = fdb_kv_get_blob(&kvdb, keyS, fdb_blob_make(&blob, reinterpret_cast<const void *>(value), maxlen))) > 0) {
        value[maxlen - 1] = 0;
        return len;
    }
    return 0;
}

bool SettingsDB::getBool(const char *key, bool *value) {
    char keyB[256]{};
    strncpy(keyB, key, sizeof(keyB) - 3);
    strcat(keyB, "@b");
    struct fdb_blob blob {};
    if (fdb_kv_get_blob(&kvdb, keyB, fdb_blob_make(&blob, reinterpret_cast<const void *>(value), sizeof(bool))) == sizeof(bool)) {
        return true;
    }
    return false;
}

bool SettingsDB::getNumber(const char *key, float *value) {
    char keyF[256]{};
    strncpy(keyF, key, sizeof(keyF) - 3);
    strcat(keyF, "@f");
    struct fdb_blob blob {};
    if (fdb_kv_get_blob(&kvdb, keyF, fdb_blob_make(&blob, reinterpret_cast<const void *>(value), sizeof(float))) == sizeof(float)) {
        return true;
    }
    return false;
}

bool SettingsDB::getNull(const char *key) {
    char keyN[256]{};
    strncpy(keyN, key, sizeof(keyN) - 3);
    strcat(keyN, "@n");
    char value = 0;
    struct fdb_blob blob {};
    if (fdb_kv_get_blob(&kvdb, keyN, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(char))) == sizeof(char)) {
        return true;
    }
    return false;
}

void SettingsDB::setString(const char *key, const char *str) {
    printf("setString <%s> <%s>\n", key, str);
    char keyS[256]{};
    strncpy(keyS, key, sizeof(keyS) - 3);
    strcat(keyS, "@s");
    fdb_kv_set(&kvdb, keyS, str);
}

void SettingsDB::setBool(const char *key, bool value) {
    printf("setBool <%s> <%s>\n", key, value ? "true" : "false");
    char keyB[256]{};
    strncpy(keyB, key, sizeof(keyB) - 3);
    strcat(keyB, "@b");
    struct fdb_blob blob {};
    fdb_kv_set_blob(&kvdb, keyB, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(value)));
}

void SettingsDB::setNumber(const char *key, float value) {
    printf("setNumber <%s> <%f>\n", key, double(value));
    char keyF[256]{};
    strncpy(keyF, key, sizeof(keyF) - 3);
    strcat(keyF, "@f");
    struct fdb_blob blob {};
    fdb_kv_set_blob(&kvdb, keyF, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(value)));
}

void SettingsDB::setNull(const char *key) {
    printf("setNull <%s>\n", key);
    char keyN[256]{};
    strncpy(keyN, key, sizeof(keyN) - 3);
    strcat(keyN, "@n");
    char value = 0;
    struct fdb_blob blob {};
    fdb_kv_set_blob(&kvdb, keyN, fdb_blob_make(&blob, reinterpret_cast<const void *>(&value), sizeof(value)));
}
