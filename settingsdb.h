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
#ifndef _SETTINGSDB_H_
#define _SETTINGSDB_H_

#include <flashdb.h>

#include "lwjson/lwjson.h"
#include "nx_api.h"
#include "tx_api.h"

#ifndef BOOTLOADER

#define KEY_TYPE_NUMBER "@f"
#define KEY_TYPE_STRING "@s"
#define KEY_TYPE_BOOL "@b"
#define KEY_TYPE_ARRAY_NUMBER "@F"
#define KEY_TYPE_ARRAY_STRING "@S"
#define KEY_TYPE_ARRAY_BOOL "@B"
#define KEY_TYPE_NULL "@n"

#define KEY_TYPE_NUMBER_CHAR 'f'
#define KEY_TYPE_STRING_CHAR 's'
#define KEY_TYPE_BOOL_CHAR 'b'
#define KEY_TYPE_ARRAY_NUMBER_CHAR 'F'
#define KEY_TYPE_ARRAY_STRING_CHAR 'S'
#define KEY_TYPE_ARRAY_BOOL_CHAR 'B'
#define KEY_TYPE_NULL_CHAR 'n'

class SettingsDB {
   public:
    static SettingsDB &instance();

    size_t getString(const char *key, char *value, size_t maxlen, const char *default_value = "");
    bool getBool(const char *key, bool *value, bool default_value = false);
    bool getNumber(const char *key, float *value, float default_value = 0);
    bool getNull(const char *key);
    bool getIP(const char *key, NXD_ADDRESS *value, NXD_ADDRESS *default_value = 0);

    void setString(const char *key, const char *str);
    void setBool(const char *key, bool value);
    void setNumber(const char *key, float value);
    void setNull(const char *key);
    void setIP(const char *key, const NXD_ADDRESS *addr);

    void erase();

    UINT jsonGETRequest(NX_PACKET *packet_ptr);
    UINT jsonPUTRequest(NX_PACKET *packet_ptr);

    static constexpr const char *kUserIPv4 = "user_ipv4_addr";
    static constexpr const char *kUserIPv4NetMask = "user_ipv4_netmask";
    static constexpr const char *kUserIPv6 = "user_ipv6_addr";
    static constexpr const char *kUserIPv6PrefixLen = "user_ipv6_prefix_len";

    static constexpr const char *kUserIPv4_t = "user_ipv4_addr" KEY_TYPE_STRING;
    static constexpr const char *kUserIPv4NetMask_t = "user_ipv4_netmask" KEY_TYPE_STRING;
    static constexpr const char *kUserIPv6_t = "user_ipv6_addr" KEY_TYPE_STRING;
    static constexpr const char *kUserIPv6PrefixLen_t = "user_ipv6_prefix_len" KEY_TYPE_STRING;

    static constexpr const char *kActiveIPv4 = "active_ipv4_addr";
    static constexpr const char *kActiveIPv4NetMask = "active_ipv4_netmask";
    static constexpr const char *kActiveIPv6 = "active_ipv6_addr";
    static constexpr const char *kActiveIPv6PrefixLen = "active_ipv6_prefix_len";

    static constexpr const char *kActiveIPv4_t = "active_ipv4_addr" KEY_TYPE_STRING;
    static constexpr const char *kActiveIPv4NetMask_t = "active_ipv4_netmask" KEY_TYPE_STRING;
    static constexpr const char *kActiveIPv6_t = "active_ipv6_addr" KEY_TYPE_STRING;
    static constexpr const char *kActiveIPv6PrefixLen_t = "active_ipv6_prefix_len" KEY_TYPE_STRING;

   private:
    void init();
    bool initialized = false;

    static void jsonStreamSettingsCallback(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type);
    void jsonStreamSettings(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type);

    static void lock();
    static void unlock();

    struct fdb_kvdb kvdb {};
};

#endif  // #ifndef BOOTLOADER

#endif  // #ifndef _SETTINGSDB_H_
