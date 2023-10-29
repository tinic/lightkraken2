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

#include "webserver.h"

#include "./app.h"
#include "./model.h"
#include "./network.h"
#include "./settingsdb.h"
#include "./systick.h"
#include "./utils.h"
#include "stm32h5xx_hal.h"

typedef UINT (*requestNotifyFunc)(NX_HTTP_SERVER *server_ptr, UINT request_type, CHAR *resource, NX_PACKET *packet_ptr);

WebServer &WebServer::instance() {
    static WebServer webserver;
    if (!webserver.initialized) {
        webserver.initialized = true;
        webserver.init();
    }
    return webserver;
}

void WebServer::init() {
    printf(ESCAPE_FG_CYAN "WebServer up.\n");
}

#ifdef BOOTLOADER
UINT WebServer::postRequestUpload(NX_HTTP_SERVER *server_ptr, UINT request_type, const CHAR *resource, NX_PACKET *packet_ptr) {
    ULONG offset = 0, chunk_length = 0, total_length = 0;
    UCHAR buffer[ETH_MAX_PAYLOAD + 1];  // plus 1 for null termination
    while (nx_http_server_get_entity_header(server_ptr, &packet_ptr, buffer, sizeof(buffer)) == NX_SUCCESS) {
        buffer[chunk_length] = 0;
        if (strstr((const char *)buffer, "application/octet-stream") != NULL) {
            while (nx_http_server_get_entity_content(server_ptr, &packet_ptr, &offset, &chunk_length) == NX_SUCCESS) {
                nx_packet_data_extract_offset(packet_ptr, offset, buffer, chunk_length, &chunk_length);
                total_length += chunk_length;
            }
        }
    }

    nx_http_server_callback_response_send_extended(server_ptr, (CHAR *)NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, NX_NULL, 0, NX_NULL, 0);
    return (NX_HTTP_CALLBACK_COMPLETED);
}
#endif  // #ifdef BOOTLOADER

UINT WebServer::requestNotifyCallback(NX_HTTP_SERVER *server_ptr, UINT request_type, const CHAR *resource, NX_PACKET *packet_ptr) {
    return WebServer::instance().requestNotify(server_ptr, request_type, resource, packet_ptr);
}

UINT WebServer::requestNotify(NX_HTTP_SERVER *server_ptr, UINT request_type, const CHAR *resource, NX_PACKET *packet_ptr) {
    switch (request_type) {
        case NX_HTTP_SERVER_GET_REQUEST: {
            if (strcmp(resource, "/") == 0) {
                const char *redirect = "<html><head><meta http-equiv=\"refresh\" content=\"0; url='./index.html'\"/></head><body></body></html>";
                nx_packet_release(packet_ptr);
                nx_http_server_callback_response_send_extended(server_ptr, (CHAR *)NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, (CHAR *)redirect,
                                                               strlen(redirect), NX_NULL, 0);
                return (NX_HTTP_CALLBACK_COMPLETED);
            }
#ifndef BOOTLOADER
            if (strcmp(resource, "/settings") == 0) {
                return SettingsDB::instance().jsonGETRequest(packet_ptr);
            }
#endif  // #ifndef BOOTLOADER
        } break;
        case NX_HTTP_SERVER_POST_REQUEST:
        case NX_HTTP_SERVER_PUT_REQUEST: {
#ifndef BOOTLOADER
            if (strcmp(resource, "/reset") == 0) {
                Systick::instance().scheduleReset();
                nx_packet_release(packet_ptr);
                nx_http_server_callback_response_send_extended(server_ptr, (CHAR *)NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, NX_NULL, NX_NULL, NX_NULL,
                                                               0);
                return (NX_HTTP_CALLBACK_COMPLETED);
            }
            if (strcmp(resource, "/erase") == 0) {
                SettingsDB::instance().erase();
                nx_packet_release(packet_ptr);
                nx_http_server_callback_response_send_extended(server_ptr, (CHAR *)NX_HTTP_STATUS_OK, sizeof(NX_HTTP_STATUS_OK) - 1, NX_NULL, NX_NULL, NX_NULL,
                                                               0);
                return (NX_HTTP_CALLBACK_COMPLETED);
            }
            if (strcmp(resource, "/settings") == 0) {
                return SettingsDB::instance().jsonPUTRequest(packet_ptr);
            }
#endif  // #ifndef BOOTLOADER
#ifdef BOOTLOADER
            if (strcmp(resource, "/upload") == 0) {
                return postRequestUpload(server_ptr, request_type, resource, packet_ptr);
            }
#endif  // #ifdef BOOTLOADER
            nx_packet_release(packet_ptr);
            nx_http_server_callback_response_send_extended(server_ptr, (CHAR *)NX_HTTP_STATUS_METHOD_NOT_ALLOWED, sizeof(NX_HTTP_STATUS_METHOD_NOT_ALLOWED) - 1,
                                                           NX_NULL, 0, NX_NULL, 0);
            return (NX_HTTP_CALLBACK_COMPLETED);
        } break;
        case NX_HTTP_SERVER_DELETE_REQUEST: {
#ifndef BOOTLOADER
            if (strcmp(resource, "/settings") == 0) {
                return SettingsDB::instance().jsonDELETERequest(packet_ptr);
            }
#endif  // #ifndef BOOTLOADER
            nx_packet_release(packet_ptr);
            nx_http_server_callback_response_send_extended(server_ptr, (CHAR *)NX_HTTP_STATUS_METHOD_NOT_ALLOWED, sizeof(NX_HTTP_STATUS_METHOD_NOT_ALLOWED) - 1,
                                                           NX_NULL, 0, NX_NULL, 0);
            return (NX_HTTP_CALLBACK_COMPLETED);
        } break;
        case NX_HTTP_SERVER_HEAD_REQUEST: {
            nx_packet_release(packet_ptr);
            nx_http_server_callback_response_send_extended(server_ptr, (CHAR *)NX_HTTP_STATUS_METHOD_NOT_ALLOWED, sizeof(NX_HTTP_STATUS_METHOD_NOT_ALLOWED) - 1,
                                                           NX_NULL, 0, NX_NULL, 0);
            return (NX_HTTP_CALLBACK_COMPLETED);
        } break;
    }
    return (NX_SUCCESS);
}

uint8_t *WebServer::setup(uint8_t *pointer) {
    UINT status;

    const size_t http_server_stack_size = 32768;

    fx_system_initialize();

    status = nx_http_server_create(&http_server, (CHAR *)"WebServer", Network::instance().ip(), &ram_disk, pointer, http_server_stack_size,
                                   Network::instance().pool(), NX_NULL, (requestNotifyFunc)requestNotifyCallback);
    pointer = pointer + http_server_stack_size;
    if (status) {
        goto fail;
    }

    static NX_HTTP_SERVER_MIME_MAP map[] = {{(CHAR *)"js", (CHAR *)"text/javascript"},
                                            {(CHAR *)"css", (CHAR *)"text/css"},
                                            {(CHAR *)"json", (CHAR *)"application/json"},
                                            {(CHAR *)"svg", (CHAR *)"image/svg+xml"}};

    nx_http_server_mime_maps_additional_set(&http_server, map, 4);

    return pointer;

fail:
    while (1) {
    }
}

void WebServer::APROMDiskDriver(FX_MEDIA *media_ptr) {
    switch (media_ptr->fx_media_driver_request) {
        case FX_DRIVER_READ: {
            UCHAR *source_buffer = ((UCHAR *)media_ptr->fx_media_driver_info) +
                                   ((media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors) * media_ptr->fx_media_bytes_per_sector);
            _fx_utility_memory_copy(source_buffer, media_ptr->fx_media_driver_buffer,
                                    media_ptr->fx_media_driver_sectors * media_ptr->fx_media_bytes_per_sector);
            media_ptr->fx_media_driver_status = FX_SUCCESS;
            break;
        }

        case FX_DRIVER_FLUSH: {
            media_ptr->fx_media_driver_status = FX_SUCCESS;
            break;
        }

        case FX_DRIVER_ABORT: {
            media_ptr->fx_media_driver_status = FX_SUCCESS;
            break;
        }

        case FX_DRIVER_INIT: {
            media_ptr->fx_media_driver_status = FX_SUCCESS;
            break;
        }

        case FX_DRIVER_UNINIT: {
            media_ptr->fx_media_driver_status = FX_SUCCESS;
            break;
        }

        case FX_DRIVER_BOOT_READ: {
            UCHAR *source_buffer = (UCHAR *)media_ptr->fx_media_driver_info;
            if ((source_buffer[0] != (UCHAR)0xEB) || ((source_buffer[1] != (UCHAR)0x34) && (source_buffer[1] != (UCHAR)0x76)) || /* exFAT jump code.  */
                (source_buffer[2] != (UCHAR)0x90)) {
                media_ptr->fx_media_driver_status = FX_MEDIA_INVALID;
                return;
            }
            UINT bytes_per_sector = _fx_utility_16_unsigned_read(&source_buffer[FX_BYTES_SECTOR]);
#ifdef FX_ENABLE_EXFAT
            if (bytes_per_sector == 0 && (source_buffer[1] == (UCHAR)0x76)) {
                bytes_per_sector = (UINT)(1 << source_buffer[FX_EF_BYTE_PER_SECTOR_SHIFT]);
            }
#endif /* FX_ENABLE_EXFAT */
            if (bytes_per_sector > media_ptr->fx_media_memory_size) {
                media_ptr->fx_media_driver_status = FX_BUFFER_ERROR;
                break;
            }
            _fx_utility_memory_copy(source_buffer, media_ptr->fx_media_driver_buffer, bytes_per_sector);
            media_ptr->fx_media_driver_status = FX_SUCCESS;
            break;
        }
        default: {
            media_ptr->fx_media_driver_status = FX_IO_ERROR;
            break;
        }
    }
}

bool WebServer::start() {
    UINT status;

#ifdef BOOTLOADER
    static __attribute__((section(".text#"))) const
#include "fsbl.h"
#else  // #ifndef BOOTLOADER
    static __attribute__((section(".text#"))) const
#include "fs.h"
#endif  // #ifndef BOOTLOADER

        status = fx_media_open(&ram_disk, (CHAR *)"APROM Disk", APROMDiskDriver, (void *)fs_data, media_memory, sizeof(media_memory));
    if (status) {
        return false;
    }

    status = nx_http_server_start(&http_server);
    if (status) {
        return false;
    }

    return true;
}
