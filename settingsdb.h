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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <nameof.hpp>
#pragma GCC diagnostic pop

#include "lwjson/lwjson.h"
#include "nx_api.h"
#include "tx_api.h"

#ifndef BOOTLOADER

class SettingsDB {
   public:
    SettingsDB() {}

    static SettingsDB &instance();

    static constexpr size_t max_array_size_2d = 16;
    static constexpr size_t max_array_size = 32;
    static constexpr size_t max_string_size = 64;
    static constexpr size_t max_object_size = 4096;

    using stringFixed_t = fixed_containers::FixedString<max_string_size>;
    using floatFixedVector_t = fixed_containers::FixedVector<float, max_array_size>;
    using floatFixedVector2D_t = fixed_containers::FixedVector<fixed_containers::FixedVector<float, max_array_size_2d>, max_array_size_2d>;
    using boolFixedVector_t = fixed_containers::FixedVector<bool, max_array_size>;
    using stringFixedVector_t = fixed_containers::FixedVector<fixed_containers::FixedString<max_string_size>, max_array_size>;

    size_t getString(const char *key, char *value, size_t maxlen, const char *default_value = "");
    bool getBool(const char *key, bool *value, bool default_value = false);
    bool getNumber(const char *key, float *value, float default_value = 0);
    bool getNull(const char *key);
    bool getIP(const char *key, NXD_ADDRESS *value, const NXD_ADDRESS *default_value = 0);

    bool getNumberVector(const char *key, floatFixedVector_t &vec);
    bool getNumberVector2D(const char *key, floatFixedVector2D_t &vec);
    bool getBoolVector(const char *key, boolFixedVector_t &vec);
    bool getStringVector(const char *key, stringFixedVector_t &vec);
    bool getObject(const char *key, char *value, size_t *len, size_t max_len);

    void setString(const char *key, const char *str);
    void setBool(const char *key, bool value);
    void setNumber(const char *key, float value);
    void setNull(const char *key);
    void setIP(const char *key, const NXD_ADDRESS *addr);

    void setNumberVector(const char *key, const floatFixedVector_t &vec);
    void setNumberVector2D(const char *key, const floatFixedVector2D_t &vec);
    void setBoolVector(const char *key, const boolFixedVector_t &vec);
    void setStringVector(const char *key, const stringFixedVector_t &vec);
    void setObject(const char *key, const char *value, size_t len);

    void delString(const char *key);
    void delBool(const char *key);
    void delNumber(const char *key);
    void delNull(const char *key);
    void delIP(const char *key);

    void delNumberVector(const char *key);
    void delNumberVector2D(const char *key);
    void delBoolVector(const char *key);
    void delStringVector(const char *key);

    bool hasNumber(const char *key);
    bool hasBool(const char *key);
    bool hasString(const char *key);
    bool hasNull(const char *key);
    bool hasIP(const char *key);

    bool hasNumberVector(const char *key);
    bool hasNumberVector2D(const char *key);
    bool hasStringVector(const char *key);
    bool hasBoolVector(const char *key);

    void erase();

    UINT jsonGETRequest(NX_PACKET *packet_ptr);
    UINT jsonPUTRequest(NX_PACKET *packet_ptr, bool deleteRequest = false);
    UINT jsonDELETERequest(NX_PACKET *packet_ptr);

#define KEY_TYPE_NUMBER "@f"
#define KEY_TYPE_STRING "@s"
#define KEY_TYPE_BOOL "@b"
#define KEY_TYPE_OBJECT "@o"
#define KEY_TYPE_NUMBER_VECTOR "@F"
#define KEY_TYPE_NUMBER_VECTOR_2D "@2"
#define KEY_TYPE_STRING_VECTOR "@S"
#define KEY_TYPE_BOOL_VECTOR "@B"
#define KEY_TYPE_NULL "@n"

#define KEY_TYPE_NUMBER_CHAR 'f'
#define KEY_TYPE_STRING_CHAR 's'
#define KEY_TYPE_BOOL_CHAR 'b'
#define KEY_TYPE_OBJECT_CHAR 'o'
#define KEY_TYPE_NUMBER_VECTOR_CHAR 'F'
#define KEY_TYPE_NUMBER_VECTOR_2D_CHAR '2'
#define KEY_TYPE_STRING_VECTOR_CHAR 'S'
#define KEY_TYPE_BOOL_VECTOR_CHAR 'B'
#define KEY_TYPE_NULL_CHAR 'n'

#define KEY_DEFINE_STRING(KEY_CONSTANT, KEY_STRING)         \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_STRING;

    KEY_DEFINE_STRING(kTag, "tag")
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

#define KEY_DEFINE_NUMBER(KEY_CONSTANT, KEY_STRING)         \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_NUMBER;

    KEY_DEFINE_NUMBER(kBootCount, "boot_count")
    KEY_DEFINE_NUMBER(kUserIPv6PrefixLen, "user_ipv6_prefix_len")

#define KEY_DEFINE_BOOL(KEY_CONSTANT, KEY_STRING)           \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_BOOL;

#define KEY_DEFINE_STRING_VECTOR(KEY_CONSTANT, KEY_STRING)  \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_STRING_VECTOR;

    KEY_DEFINE_STRING_VECTOR(kActiveIPv6, "active_ipv6_addr")
    KEY_DEFINE_STRING_VECTOR(kStripOutputType, "strip_output_type")
    KEY_DEFINE_STRING_VECTOR(kStripInputType, "strip_input_type")
    KEY_DEFINE_STRING_VECTOR(kStripStartupMode, "strip_startup_mode")
    KEY_DEFINE_STRING_VECTOR(kAnalogOutputType, "analog_output_type")
    KEY_DEFINE_STRING_VECTOR(kAnalogInputType, "analog_input_type")

#define KEY_DEFINE_OBJECT(KEY_CONSTANT, KEY_STRING)         \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_OBJECT;

    KEY_DEFINE_OBJECT(kStripOutputProperties, "strip_output_properties")
    KEY_DEFINE_OBJECT(kOutputConfigProperties, "output_config_properties")
    KEY_DEFINE_OBJECT(kOutputConfigPinNames, "output_config_pin_names")
    KEY_DEFINE_OBJECT(kAnalogOutputTypes, "analog_output_types")
    KEY_DEFINE_OBJECT(kAnalogInputTypes, "analog_input_types")
    KEY_DEFINE_OBJECT(kStripOutputTypes, "strip_output_types")
    KEY_DEFINE_OBJECT(kStripInputTypes, "strip_input_types")
    KEY_DEFINE_OBJECT(kStripStartupModes, "strip_startup_modes")
    KEY_DEFINE_OBJECT(kOutputConfigTypes, "output_config_types")

#define KEY_DEFINE_NUMBER_VECTOR(KEY_CONSTANT, KEY_STRING)  \
    static constexpr const char *KEY_CONSTANT = KEY_STRING; \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_NUMBER_VECTOR;

    KEY_DEFINE_NUMBER_VECTOR(kActiveIPv6PrefixLen, "active_ipv6_prefix_len")
    KEY_DEFINE_NUMBER_VECTOR(kStripCompLimit, "strip_comp_limit")
    KEY_DEFINE_NUMBER_VECTOR(kStripGlobIllum, "strip_glob_illum")
    KEY_DEFINE_NUMBER_VECTOR(kStripLedCount, "strip_led_count")
    KEY_DEFINE_NUMBER_VECTOR(kAnalogPwmLimit, "analog_pwm_limit")

#define KEY_DEFINE_NUMBER_VECTOR_2D(KEY_CONSTANT, KEY_STRING) \
    static constexpr const char *KEY_CONSTANT = KEY_STRING;   \
    static constexpr const char *KEY_CONSTANT##_t = KEY_STRING KEY_TYPE_NUMBER_VECTOR_2D;

    KEY_DEFINE_NUMBER_VECTOR_2D(kStripArtnetUniverse, "strip_artnet_universe")
    KEY_DEFINE_NUMBER_VECTOR_2D(kStripe131Universe, "strip_e131_universe")
    KEY_DEFINE_NUMBER_VECTOR_2D(kAnalogArtnetUniverse, "analog_artnet_universe")
    KEY_DEFINE_NUMBER_VECTOR_2D(kAnalogArtnetChannel, "analog_artnet_channel")
    KEY_DEFINE_NUMBER_VECTOR_2D(kAnaloge131Universe, "analog_e131_universe")
    KEY_DEFINE_NUMBER_VECTOR_2D(kAnaloge131Channel, "analog_e131_channel")

   private:
    void init();
    bool initialized = false;

    static void jsonStreamSettingsCallback(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type);
    void jsonStreamSettings(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type);

    static void lock();
    static void unlock();

    struct fdb_kvdb kvdb {};

    bool in_delete_request = false;
    bool in_array = false;
    int32_t in_array_type = -1;

    fixed_containers::FixedString<max_string_size> array_key_name{};
    floatFixedVector_t scratch_float_vector{};
    floatFixedVector2D_t scratch_float_vector_2d{};
    boolFixedVector_t scratch_bool_vector{};
    stringFixedVector_t scratch_string_vector{};

    std::array<std::array<char, max_string_size>, max_array_size> scratch_string_array{};
    std::array<char, max_object_size> scratch_object{};
};

#endif  // #ifndef BOOTLOADER

#endif  // #ifndef _SETTINGSDB_H_
