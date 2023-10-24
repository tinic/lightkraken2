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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fixed_containers/fixed_string.hpp>
#include <fixed_containers/fixed_vector.hpp>
#pragma GCC diagnostic pop

#include "lwjson/lwjson.h"
#include "nx_api.h"
#include "tx_api.h"

#ifndef BOOTLOADER

class SettingsDB {
   public:
    SettingsDB() : float_vector(), bool_vector(), string_vector() {}

    static SettingsDB &instance();

    size_t getString(const char *key, char *value, size_t maxlen, const char *default_value = "");
    bool getBool(const char *key, bool *value, bool default_value = false);
    bool getNumber(const char *key, float *value, float default_value = 0);
    bool getNull(const char *key);
    bool getIP(const char *key, NXD_ADDRESS *value, const NXD_ADDRESS *default_value = 0);

    void setString(const char *key, const char *str);
    void setBool(const char *key, bool value);
    void setNumber(const char *key, float value);
    void setNull(const char *key);
    void setIP(const char *key, const NXD_ADDRESS *addr);

    void erase();

    UINT jsonGETRequest(NX_PACKET *packet_ptr);
    UINT jsonPUTRequest(NX_PACKET *packet_ptr);

    static constexpr size_t max_array_size = 32;
    static constexpr size_t max_string_size = 64;

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

#define KEY_DEFINE_STRING(KEY_CONSTANT, KEY_STRING)         \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_STRING;

    KEY_DEFINE_STRING(kHostname, "hostname")
    KEY_DEFINE_STRING(kMacAddress, "mac_address")
    KEY_DEFINE_STRING(kUID, "uid")
    KEY_DEFINE_STRING(kPackageType, "package_type")
    KEY_DEFINE_STRING(kFlashSize, "flash_size")

    KEY_DEFINE_STRING(kUserIPv4, "user_ipv4_addr")
    KEY_DEFINE_STRING(kUserIPv4NetMask, "user_ipv4_netmask")
    KEY_DEFINE_STRING(kUserIPv6, "user_ipv6_addr")

    KEY_DEFINE_STRING(kActiveIPv4, "active_ipv4_addr")
    KEY_DEFINE_STRING(kActiveIPv4NetMask, "active_ipv4_netmask")
    KEY_DEFINE_STRING(kActiveIPv6, "active_ipv6_addr")

#define KEY_DEFINE_NUMBER(KEY_CONSTANT, KEY_STRING)         \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_NUMBER;

    KEY_DEFINE_NUMBER(kBootCount, "boot_count")
    KEY_DEFINE_NUMBER(kUserIPv6PrefixLen, "user_ipv6_prefix_len")
    KEY_DEFINE_NUMBER(kActiveIPv6PrefixLen, "active_ipv6_prefix_len")

#define KEY_DEFINE_BOOL(KEY_CONSTANT, KEY_STRING)           \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_BOOL;

   private:
    void init();
    bool initialized = false;

    static void jsonStreamSettingsCallback(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type);
    void jsonStreamSettings(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type);

    static void lock();
    static void unlock();

    struct fdb_kvdb kvdb {};

    bool in_array = false;
    int32_t in_array_type = -1;

    fixed_containers::FixedVector<float, max_array_size> float_vector;
    fixed_containers::FixedVector<bool, max_array_size> bool_vector;
    fixed_containers::FixedVector<fixed_containers::FixedString<max_string_size>, max_array_size> string_vector;
};

#endif  // #ifndef BOOTLOADER

#endif  // #ifndef _SETTINGSDB_H_
