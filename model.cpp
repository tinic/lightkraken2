/*
Copyright 2023 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./model.h"
#include "./settingsdb.h"

#include <emio/buffer.hpp>
#include <emio/format.hpp>

#include <fixed_containers/fixed_string.hpp>
#include <fixed_containers/fixed_vector.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <nameof.hpp>
#pragma GCC diagnostic pop

#ifndef BOOTLOADER

Model &Model::instance() {
    static Model model;
    if (!model.initialized) {
        model.initialized = true;
        model.init();
    }
    return model;
}

void Model::dumpStatics()  {

    auto stripOutputStringVector = [] () consteval {
        SettingsDB::objectFixedVector_t vec{};
        for (Model::StripOutputProperties prop : Model::stripOutputProperties ) {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "{{").value();
            emio::format_to(buf, "\"{}\":\"{}\",", "label", NAMEOF_ENUM(prop.type)).value();
            emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.type), int(prop.type)).value();
            emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.components), prop.components).value();
            emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.bitslen), prop.bitslen).value();
            emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.clock), prop.clock ? "true" : "false").value();
            emio::format_to(buf, "\"{}\":{}", NAMEOF(prop.globalillum), prop.globalillum ? "true" : "false").value();
            emio::format_to(buf, "}}").value();
            vec.push_back(buf.view());
        }
        return vec;
    };
    static constexpr auto vec0 = stripOutputStringVector();
    SettingsDB::instance().setObjectVector(SettingsDB::kStripOutputProperties, vec0);

    auto outputConfigPropertiesVector = [] () consteval {
        SettingsDB::objectFixedVector_t vec{};
        for (Model::OutputConfigProperties prop : Model::outputConfigProperties ) {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "{{").value();
            emio::format_to(buf, "\"{}\":\"{}\",", NAMEOF(prop.label), prop.label).value();
            emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.stripn), prop.stripn).value();
            emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.alogn), prop.alogn).value();
            const char *comma = "";
            emio::format_to(buf, "\"{}\":[", NAMEOF(prop.strip)).value();
            for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++ ) {
                emio::format_to(buf, "{}{}", comma, prop.strip[c] ? 1 : 0).value();
                comma = ",";
            }
            emio::format_to(buf, "],").value();
            comma = "";
            emio::format_to(buf, "\"{}\":[", NAMEOF(prop.rgb)).value();
            for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++ ) {
                emio::format_to(buf, "{}{}", comma, prop.rgb[c] ? 1 : 0).value();
                comma = ",";
            }
            emio::format_to(buf, "],").value();
            comma = "";
            emio::format_to(buf, "\"{}\":[", NAMEOF(prop.wclock)).value();
            for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++ ) {
                emio::format_to(buf, "{}{}", comma, prop.wclock[c] ? 1 : 0).value();
                comma = ",";
            }
            emio::format_to(buf, "],").value();
            comma = "";
            emio::format_to(buf, "\"{}\":[", NAMEOF(prop.comp)).value();
            for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++ ) {
                emio::format_to(buf, "{}{}", comma, prop.comp[c]).value();
                comma = ",";
            }
            emio::format_to(buf, "]").value();
            emio::format_to(buf, "}}").value();
            vec.push_back(buf.view());
        }
        return vec;
    };
    static constexpr auto vec1 = outputConfigPropertiesVector();
    SettingsDB::instance().setObjectVector(SettingsDB::kOutputConfigProperties, vec1);

    auto analogOutputTypes = []() consteval {
        SettingsDB::stringFixedVector_t vec;
        for (size_t c = 0; c < AnalogConfig::OUTPUT_COUNT; c++) {
            vec.push_back(NAMEOF_ENUM(AnalogConfig::AnalogOutputType(c)));
        }
        return vec;
    };
    static constexpr auto vec2 = analogOutputTypes();
    SettingsDB::instance().setStringVector(SettingsDB::kAnalogOutputType, vec2);

    auto analogInputTypes = []() consteval {
        SettingsDB::stringFixedVector_t vec;
        for (size_t c = 0; c < AnalogConfig::INPUT_COUNT; c++) {
            vec.push_back(NAMEOF_ENUM(AnalogConfig::AnalogInputType(c)));
        }
        return vec;
    };
    static constexpr auto vec3 = analogInputTypes();
    SettingsDB::instance().setStringVector(SettingsDB::kAnalogInputType, vec3);

    auto stripInputTypes = []() consteval {
        SettingsDB::stringFixedVector_t vec;
        for (size_t c = 0; c < StripConfig::INPUT_COUNT; c++) {
            vec.push_back(NAMEOF_ENUM(StripConfig::StripInputType(c)));
        }
        return vec;
    };
    static constexpr auto vec4 = stripInputTypes();
    SettingsDB::instance().setStringVector(SettingsDB::kStripInputType, vec4);

    auto stripOutptTypes = []() consteval {
        SettingsDB::stringFixedVector_t vec;
        for (size_t c = 0; c < StripConfig::OUTPUT_COUNT; c++) {
            vec.push_back(NAMEOF_ENUM(StripConfig::StripOutputType(c)));
        }
        return vec;
    };
    static constexpr auto vec5 = stripOutptTypes();
    SettingsDB::instance().setStringVector(SettingsDB::kStripOutputType, vec5);

    auto stripStartupMode = []() consteval {
        SettingsDB::stringFixedVector_t vec;
        for (size_t c = 0; c < StripConfig::STARTUP_COUNT; c++) {
            vec.push_back(NAMEOF_ENUM(StripConfig::StripStartupMode(c)));
        }
        return vec;
    };
    static constexpr auto vec7 = stripStartupMode();
    SettingsDB::instance().setStringVector(SettingsDB::kStripStartupMode, vec7);

    auto outputConfigType = []() consteval {
        SettingsDB::stringFixedVector_t vec;
        for (size_t c = 0; c < CONFIG_COUNT; c++) {
            vec.push_back(NAMEOF_ENUM(OutputConfig(c)));
        }
        return vec;
    };
    static constexpr auto vec8 = outputConfigType();
    SettingsDB::instance().setStringVector(SettingsDB::kOutputConfigType, vec8);
}

void Model::init() {
}

#endif  // #ifndef BOOTLOADER
