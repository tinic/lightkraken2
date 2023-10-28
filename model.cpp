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

#include <emio/buffer.hpp>
#include <emio/format.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fixed_containers/fixed_string.hpp>
#include <fixed_containers/fixed_vector.hpp>
#pragma GCC diagnostic pop

#include "./settingsdb.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <nameof.hpp>
#pragma GCC diagnostic pop

#include <string>
#include <vector>

#ifndef BOOTLOADER

Model &Model::instance() {
    static Model model;
    if (!model.initialized) {
        model.initialized = true;
        model.init();
    }
    return model;
}

void Model::dumpStatics() {
    {
        auto stripOutputStringVector = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma = "";
            for (Model::StripOutputProperties prop : Model::stripOutputProperties) {
                emio::format_to(buf, "{}{{", comma).value();
                emio::format_to(buf, "\"{}\":\"{}\",", "label", NAMEOF_ENUM(prop.type)).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.type), int(prop.type)).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.components), prop.components).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.bitslen), prop.bitslen).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.clock), prop.clock ? "true" : "false").value();
                emio::format_to(buf, "\"{}\":{}", NAMEOF(prop.globalillum), prop.globalillum ? "true" : "false").value();
                emio::format_to(buf, "}}").value();
                comma = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<1024 + 512>(buf.view());
        };
        static constexpr auto data = stripOutputStringVector();
        SettingsDB::instance().setObject(SettingsDB::kStripOutputProperties, data.c_str(), data.size() + 1);
    }

    {
        auto outputConfigPropertiesVector = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (Model::OutputConfigProperties prop : Model::outputConfigProperties) {
                emio::format_to(buf, "{}{{", comma_outer).value();
                emio::format_to(buf, "\"{}\":\"{}\",", NAMEOF(prop.label), prop.label).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.strip_n), prop.strip_n).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.analog_n), prop.analog_n).value();
                const char *comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.strip)).value();
                for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++) {
                    emio::format_to(buf, "{}{}", comma, prop.strip[c] ? 1 : 0).value();
                    comma = ",";
                }
                emio::format_to(buf, "],").value();
                comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.analog)).value();
                for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++) {
                    emio::format_to(buf, "{}{}", comma, prop.analog[c] ? 1 : 0).value();
                    comma = ",";
                }
                emio::format_to(buf, "],").value();
                comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.has_clock)).value();
                for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++) {
                    emio::format_to(buf, "{}{}", comma, prop.has_clock[c] ? 1 : 0).value();
                    comma = ",";
                }
                emio::format_to(buf, "],").value();
                comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.components)).value();
                for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++) {
                    emio::format_to(buf, "{}{}", comma, prop.components[c]).value();
                    comma = ",";
                }
                emio::format_to(buf, "]").value();
                emio::format_to(buf, "}}").value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<1024 + 512>(buf.view());
        };
        static constexpr auto data = outputConfigPropertiesVector();
        SettingsDB::instance().setObject(SettingsDB::kOutputConfigProperties, data.c_str(), data.size() + 1);
    }
    {
        auto outputConfigPinNamesVector = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (Model::OutputConfigPinNames prop : Model::outputConfigPinNames) {
                emio::format_to(buf, "{}{{", comma_outer).value();

                const char *comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.pinlabels.no_clock)).value();
                for (size_t c = 0; c < Model::OutputConfigPinNames::OutputConfigPinCount; c++) {
                    emio::format_to(buf, "{}{{\"{}\":\"{}\",\"{}\":\"{}\"}}", comma, NAMEOF(prop.pinlabels.no_clock[c].l), prop.pinlabels.no_clock[c].l,
                                    NAMEOF(prop.pinlabels.no_clock[c].s), prop.pinlabels.no_clock[c].s)
                        .value();
                    comma = ",";
                }
                emio::format_to(buf, "],").value();

                comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.pinlabels.with_clock)).value();
                for (size_t c = 0; c < Model::OutputConfigPinNames::OutputConfigPinCount; c++) {
                    emio::format_to(buf, "{}{{\"{}\":\"{}\",\"{}\":\"{}\"}}", comma, NAMEOF(prop.pinlabels.with_clock[c].l), prop.pinlabels.with_clock[c].l,
                                    NAMEOF(prop.pinlabels.with_clock[c].s), prop.pinlabels.with_clock[c].s)
                        .value();
                    comma = ",";
                }
                emio::format_to(buf, "]").value();
                emio::format_to(buf, "}}").value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<2048 + 1024>(buf.view());
        };
        static constexpr auto data = outputConfigPinNamesVector();
        SettingsDB::instance().setObject(SettingsDB::kOutputConfigPinNames, data.c_str(), data.size() + 1);
    }
    {
        auto analogOutputTypes = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (size_t c = 0; c < AnalogConfig::OUTPUT_COUNT; c++) {
                emio::format_to(buf, "{}\"{}\"", comma_outer, NAMEOF_ENUM(AnalogConfig::AnalogOutputType(c))).value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<128>(buf.view());
        };
        static constexpr auto data = analogOutputTypes();
        SettingsDB::instance().setObject(SettingsDB::kAnalogOutputTypes, data.c_str(), data.size() + 1);
    }

    {
        auto analogInputTypes = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (size_t c = 0; c < AnalogConfig::INPUT_COUNT; c++) {
                emio::format_to(buf, "{}\"{}\"", comma_outer, NAMEOF_ENUM(AnalogConfig::AnalogInputType(c))).value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<192>(buf.view());
        };
        static constexpr auto data = analogInputTypes();
        SettingsDB::instance().setObject(SettingsDB::kAnalogInputTypes, data.c_str(), data.size() + 1);
    }

    {
        auto stripInputTypes = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (size_t c = 0; c < StripConfig::INPUT_COUNT; c++) {
                emio::format_to(buf, "{}\"{}\"", comma_outer, NAMEOF_ENUM(StripConfig::StripInputType(c))).value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<128>(buf.view());
        };
        static constexpr auto data = stripInputTypes();
        SettingsDB::instance().setObject(SettingsDB::kStripInputTypes, data.c_str(), data.size() + 1);
    }

    {
        auto stripOutptTypes = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (size_t c = 0; c < StripConfig::OUTPUT_COUNT; c++) {
                emio::format_to(buf, "{}\"{}\"", comma_outer, NAMEOF_ENUM(StripConfig::StripOutputType(c))).value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<192>(buf.view());
        };
        static constexpr auto data = stripOutptTypes();
        SettingsDB::instance().setObject(SettingsDB::kStripOutputTypes, data.c_str(), data.size() + 1);
    }

    {
        auto stripStartupMode = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (size_t c = 0; c < StripConfig::STARTUP_COUNT; c++) {
                emio::format_to(buf, "{}\"{}\"", comma_outer, NAMEOF_ENUM(StripConfig::StripStartupMode(c))).value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<128>(buf.view());
        };
        static constexpr auto data = stripStartupMode();
        SettingsDB::instance().setObject(SettingsDB::kStripStartupModes, data.c_str(), data.size() + 1);
    }
    {
        auto outputConfigType = []() consteval {
            emio::static_buffer<SettingsDB::max_object_size> buf{};
            emio::format_to(buf, "[").value();
            const char *comma_outer = "";
            for (size_t c = 0; c < CONFIG_COUNT; c++) {
                emio::format_to(buf, "{}\"{}\"", comma_outer, NAMEOF_ENUM(OutputConfig(c))).value();
                comma_outer = ",";
            }
            emio::format_to(buf, "]").value();
            return fixed_containers::FixedString<128>(buf.view());
        };
        static constexpr auto data = outputConfigType();
        SettingsDB::instance().setObject(SettingsDB::kOutputConfigTypes, data.c_str(), data.size() + 1);
    }
}

void Model::exportToDB() {}

void Model::importFromDB() {}

void Model::init() { dumpStatics(); }

#endif  // #ifndef BOOTLOADER
