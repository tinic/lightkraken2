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

#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <magic_enum.hpp>
#pragma GCC diagnostic pop

#include "./settingsdb.h"
#include "./utils.h"

#ifndef BOOTLOADER

Model &Model::instance() {
    static Model model;
    if (!model.initialized) {
        model.initialized = true;
        model.init();
    }
    return model;
}

void Model::exportStaticsToDB() {
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
                emio::format_to(buf, "\"{}\":\"{}\",", NAMEOF(prop.output_config), NAMEOF_ENUM(prop.output_config)).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.strip_n), prop.strip_n).value();
                emio::format_to(buf, "\"{}\":{},", NAMEOF(prop.analog_n), prop.analog_n).value();
                const char *comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.strip)).value();
                for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++) {
                    emio::format_to(buf, "{}{}", comma, prop.strip[c] ? "true" : "false").value();
                    comma = ",";
                }
                emio::format_to(buf, "],").value();
                comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.analog)).value();
                for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++) {
                    emio::format_to(buf, "{}{}", comma, prop.analog[c] ? "true" : "false").value();
                    comma = ",";
                }
                emio::format_to(buf, "],").value();
                comma = "";
                emio::format_to(buf, "\"{}\":[", NAMEOF(prop.has_clock)).value();
                for (size_t c = 0; c < Model::OutputConfigProperties::OutputConfigMaxDevices; c++) {
                    emio::format_to(buf, "{}{}", comma, prop.has_clock[c] ? "true" : "false").value();
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

    SettingsDB::instance().setNumber(SettingsDB::kMaxUniverses, universeN);
    SettingsDB::instance().setNumber(SettingsDB::kMaxStrips, stripN);
    SettingsDB::instance().setNumber(SettingsDB::kMaxAnalog, analogN);
}

void Model::exportToDB() {
    SettingsDB::floatFixedVector_t nvec{};
    SettingsDB::stringFixedVector_t svec{};
    SettingsDB::floatFixedVector2D_t dvec{};

    if (!SettingsDB::instance().hasBool(SettingsDB::kBroadcastEnabled)) {
        SettingsDB::instance().setBool(SettingsDB::kBroadcastEnabled, broadcastEnabled);
    }

    if (!SettingsDB::instance().hasBool(SettingsDB::kBurstModeEnabled)) {
        SettingsDB::instance().setBool(SettingsDB::kBurstModeEnabled, burstMode);
    }

    if (!SettingsDB::instance().hasString(SettingsDB::kOutputConfig)) {
        auto config = magic_enum::enum_name(output_config);
        SettingsDB::instance().setString(SettingsDB::kOutputConfig, std::string(config).c_str());
    }

    if (!SettingsDB::instance().hasStringVector(SettingsDB::kStripOutputType)) {
        svec.clear();
        for (size_t c = 0; c < stripN; c++) {
            svec.push_back(NAMEOF_ENUM(strip_config[c].output_type));
        }
        SettingsDB::instance().setStringVector(SettingsDB::kStripOutputType, svec);
    }

    if (!SettingsDB::instance().hasStringVector(SettingsDB::kStripInputType)) {
        svec.clear();
        for (size_t c = 0; c < stripN; c++) {
            svec.push_back(NAMEOF_ENUM(strip_config[c].input_type));
        }
        SettingsDB::instance().setStringVector(SettingsDB::kStripInputType, svec);
    }

    if (!SettingsDB::instance().hasNumberVector(SettingsDB::kStripStartupMode)) {
        svec.clear();
        for (size_t c = 0; c < stripN; c++) {
            svec.push_back(NAMEOF_ENUM(strip_config[c].startup_mode));
        }
        SettingsDB::instance().setStringVector(SettingsDB::kStripStartupMode, svec);
    }

    if (!SettingsDB::instance().hasNumberVector(SettingsDB::kStripCompLimit)) {
        nvec.clear();
        for (size_t c = 0; c < stripN; c++) {
            nvec.push_back(float(strip_config[c].comp_limit));
        }
        SettingsDB::instance().setNumberVector(SettingsDB::kStripCompLimit, nvec);
    }

    if (!SettingsDB::instance().hasNumberVector(SettingsDB::kStripLedCount)) {
        nvec.clear();
        for (size_t c = 0; c < stripN; c++) {
            nvec.push_back(float(strip_config[c].led_count));
        }
        SettingsDB::instance().setNumberVector(SettingsDB::kStripLedCount, nvec);
    }

    if (!SettingsDB::instance().hasNumberVector2D(SettingsDB::kStripArtnetUniverse)) {
        dvec.clear();
        for (size_t c = 0; c < stripN; c++) {
            fixed_containers::FixedVector<float, SettingsDB::max_array_size_2d> ivec{};
            for (size_t d = 0; d < universeN; d++) {
                ivec.push_back(float(strip_config[c].artnet[d]));
            }
            dvec.push_back(ivec);
        }
        SettingsDB::instance().setNumberVector2D(SettingsDB::kStripArtnetUniverse, dvec);
    }

    if (!SettingsDB::instance().hasNumberVector2D(SettingsDB::kStripe131Universe)) {
        dvec.clear();
        for (size_t c = 0; c < stripN; c++) {
            fixed_containers::FixedVector<float, SettingsDB::max_array_size_2d> ivec{};
            for (size_t d = 0; d < universeN; d++) {
                ivec.push_back(float(strip_config[c].artnet[d]));
            }
            dvec.push_back(ivec);
        }
        SettingsDB::instance().setNumberVector2D(SettingsDB::kStripe131Universe, dvec);
    }

    //------------------------------------------------

    if (!SettingsDB::instance().hasStringVector(SettingsDB::kAnalogOutputType)) {
        svec.clear();
        for (size_t c = 0; c < analogN; c++) {
            svec.push_back(NAMEOF_ENUM(analog_config[c].output_type));
        }
        SettingsDB::instance().setStringVector(SettingsDB::kAnalogOutputType, svec);
    }

    if (!SettingsDB::instance().hasStringVector(SettingsDB::kAnalogInputType)) {
        svec.clear();
        for (size_t c = 0; c < analogN; c++) {
            svec.push_back(NAMEOF_ENUM(analog_config[c].input_type));
        }
        SettingsDB::instance().setStringVector(SettingsDB::kAnalogInputType, svec);
    }

    if (!SettingsDB::instance().hasNumberVector(SettingsDB::kAnalogPwmLimit)) {
        nvec.clear();
        for (size_t c = 0; c < analogN; c++) {
            nvec.push_back(float(analog_config[c].pwm_limit));
        }
        SettingsDB::instance().setNumberVector(SettingsDB::kAnalogPwmLimit, nvec);
    }

    if (!SettingsDB::instance().hasNumberVector2D(SettingsDB::kAnalogArtnetUniverse)) {
        dvec.clear();
        for (size_t c = 0; c < analogN; c++) {
            fixed_containers::FixedVector<float, SettingsDB::max_array_size_2d> ivec{};
            for (size_t d = 0; d < analogCompN; d++) {
                ivec.push_back(float(analog_config[c].components[d].artnet.universe));
            }
            dvec.push_back(ivec);
        }
        SettingsDB::instance().setNumberVector2D(SettingsDB::kAnalogArtnetUniverse, dvec);
    }

    if (!SettingsDB::instance().hasNumberVector2D(SettingsDB::kAnalogArtnetChannel)) {
        dvec.clear();
        for (size_t c = 0; c < analogN; c++) {
            fixed_containers::FixedVector<float, SettingsDB::max_array_size_2d> ivec{};
            for (size_t d = 0; d < analogCompN; d++) {
                ivec.push_back(float(analog_config[c].components[d].artnet.channel));
            }
            dvec.push_back(ivec);
        }
        SettingsDB::instance().setNumberVector2D(SettingsDB::kAnalogArtnetChannel, dvec);
    }

    if (!SettingsDB::instance().hasNumberVector2D(SettingsDB::kAnaloge131Universe)) {
        dvec.clear();
        for (size_t c = 0; c < analogN; c++) {
            fixed_containers::FixedVector<float, SettingsDB::max_array_size_2d> ivec{};
            for (size_t d = 0; d < analogCompN; d++) {
                ivec.push_back(float(analog_config[c].components[d].e131.universe));
            }
            dvec.push_back(ivec);
        }
        SettingsDB::instance().setNumberVector2D(SettingsDB::kAnaloge131Universe, dvec);
    }

    if (!SettingsDB::instance().hasNumberVector2D(SettingsDB::kAnaloge131Channel)) {
        dvec.clear();
        for (size_t c = 0; c < analogN; c++) {
            fixed_containers::FixedVector<float, SettingsDB::max_array_size_2d> ivec{};
            for (size_t d = 0; d < analogCompN; d++) {
                ivec.push_back(float(analog_config[c].components[d].e131.channel));
            }
            dvec.push_back(ivec);
        }
        SettingsDB::instance().setNumberVector2D(SettingsDB::kAnaloge131Channel, dvec);
    }
}

bool Model::importFromDB() {
    SettingsDB::floatFixedVector_t nvec{};
    SettingsDB::stringFixedVector_t svec{};
    SettingsDB::floatFixedVector2D_t dvec{};

    {
        bool be = false;
        if (SettingsDB::instance().getBool(SettingsDB::kBroadcastEnabled, &be)) {
            broadcastEnabled = be;
        }
    }

    {
        bool bm = false;
        if (SettingsDB::instance().getBool(SettingsDB::kBurstModeEnabled, &bm)) {
            burstMode = bm;
        }
    }

    // ----------------------------

    char outputConfig[SettingsDB::max_string_size] {};
    if (SettingsDB::instance().getString(SettingsDB::kOutputConfig, outputConfig, SettingsDB::max_string_size)) {
        auto value = magic_enum::enum_cast<OutputConfig>(outputConfig, magic_enum::case_insensitive);
        if (value.has_value()) {
            output_config = value.value();
        } else {
            return false;
        }
    }

    // ----------------------------

    if (SettingsDB::instance().getStringVector(SettingsDB::kStripOutputType, svec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                auto value = magic_enum::enum_cast<StripConfig::StripOutputType>(svec[c], magic_enum::case_insensitive);
                if (value.has_value()) {
                    strip_config[c].output_type = value.value();
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getStringVector(SettingsDB::kStripInputType, svec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                auto value = magic_enum::enum_cast<StripConfig::StripInputType>(svec[c], magic_enum::case_insensitive);
                if (value.has_value()) {
                    strip_config[c].input_type = value.value();
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getStringVector(SettingsDB::kStripStartupMode, svec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                auto value = magic_enum::enum_cast<StripConfig::StripStartupMode>(svec[c], magic_enum::case_insensitive);
                if (value.has_value()) {
                    strip_config[c].startup_mode = value.value();
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector(SettingsDB::kStripCompLimit, nvec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                if ((nvec[c] < 0.0f) || (nvec[c] > 2.0f)) {
                    return false;
                }
                strip_config[c].comp_limit = nvec[c];
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector(SettingsDB::kStripGlobIllum, nvec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                if ((nvec[c] < 0.0f) || (nvec[c] > 2.0f)) {
                    return false;
                }
                strip_config[c].glob_illum = nvec[c];
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector(SettingsDB::kStripLedCount, nvec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                if ((nvec[c] < 0.0f) || (nvec[c] > float(maxLEDs))) {
                    return false;
                }
                strip_config[c].led_count = uint16_t(nvec[c]);
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector2D(SettingsDB::kStripArtnetUniverse, dvec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                if (svec[c].size() >= universeN) {
                    for (size_t d = 0; c < universeN; c++) {
                        if ((dvec[c][d] < 0.0f) || (dvec[c][d] > float(maxUniverseID))) {
                            return false;
                        }
                        strip_config[c].artnet[d] = uint16_t(dvec[c][d]);
                    }
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector2D(SettingsDB::kStripe131Universe, dvec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                if (svec[c].size() >= universeN) {
                    for (size_t d = 0; c < universeN; c++) {
                        if ((dvec[c][d] < 0.0f) || (dvec[c][d] > float(maxUniverseID+1))) {
                            return false;
                        }
                        strip_config[c].e131[d] = uint16_t(dvec[c][d]);
                    }
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    // ----------------------------

    if (SettingsDB::instance().getStringVector(SettingsDB::kAnalogOutputType, svec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                auto value = magic_enum::enum_cast<AnalogConfig::AnalogOutputType>(svec[c], magic_enum::case_insensitive);
                if (value.has_value()) {
                    analog_config[c].output_type = value.value();
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getStringVector(SettingsDB::kAnalogInputType, svec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                auto value = magic_enum::enum_cast<AnalogConfig::AnalogInputType>(svec[c]), magic_enum::case_insensitive;
                if (value.has_value()) {
                    analog_config[c].input_type = value.value();
                } else{
                    return false;
                } 
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector(SettingsDB::kAnalogPwmLimit, nvec)) {
        if (svec.size() >= stripN) {
            for (size_t c = 0; c < stripN; c++) {
                if (nvec[c] < 0.0f || nvec[c] > 2.0f) {
                    return false;
                }
                analog_config[c].pwm_limit = nvec[c];
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector2D(SettingsDB::kAnalogArtnetUniverse, dvec)) {
        if (svec.size() >= analogN) {
            for (size_t c = 0; c < analogN; c++) {
                if (svec[c].size() >= analogCompN) {
                    for (size_t d = 0; c < analogCompN; c++) {
                        if ((dvec[c][d] < 0.0f) || (dvec[c][d] > float(maxUniverseID+1))) {
                            return false;
                        }
                        analog_config[c].components[d].artnet.universe = uint16_t(dvec[c][d]);
                    }
                } else{
                    return false;
                } 
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector2D(SettingsDB::kAnalogArtnetChannel, dvec)) {
        if (svec.size() >= analogN) {
            for (size_t c = 0; c < analogN; c++) {
                if (svec[c].size() >= analogCompN) {
                    for (size_t d = 0; c < analogCompN; c++) {
                        if ((dvec[c][d] < 0.0f) || (dvec[c][d] >= float(analogCompN))) {
                            return false;
                        }
                        analog_config[c].components[d].artnet.channel = uint16_t(dvec[c][d]);
                    }
                } else{
                    return false;
                } 
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector2D(SettingsDB::kAnaloge131Universe, dvec)) {
        if (svec.size() >= analogN) {
            for (size_t c = 0; c < analogN; c++) {
                if (svec[c].size() >= analogCompN) {
                    for (size_t d = 0; c < analogCompN; c++) {
                        if ((dvec[c][d] < 0.0f) || (dvec[c][d] > float(maxUniverseID+1))) {
                            return false;
                        }
                        analog_config[c].components[d].e131.universe = uint16_t(dvec[c][d]);
                    }
                } else{
                    return false;
                } 
            }
        } else {
            return false;
        }
    }

    if (SettingsDB::instance().getNumberVector2D(SettingsDB::kAnalogArtnetChannel, dvec)) {
        if (svec.size() >= analogN) {
            for (size_t c = 0; c < analogN; c++) {
                if (svec[c].size() >= analogCompN) {
                    for (size_t d = 0; c < analogCompN; c++) {
                        if ((dvec[c][d] < 0.0f) || (dvec[c][d] >= float(analogCompN))) {
                            return false;
                        }
                        analog_config[c].components[d].e131.channel = uint16_t(dvec[c][d]);
                    }
                } else{
                    return false;
                } 
            }
        } else {
            return false;
        }
    }

    return true;
}

void Model::init() {
    printf(ESCAPE_FG_CYAN "Model up.\n");
}

#endif  // #ifndef BOOTLOADER
