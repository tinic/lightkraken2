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
#include "./control.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "./color.h"
#include "./driver.h"
#include "./spi.h"
#include "./strip.h"
#include "./systick.h"
#include "./utils.h"

#ifndef BOOTLOADER

void thread_control_entry(ULONG thread_input) {
    NX_PARAMETER_NOT_USED(thread_input);
    Control::instance().thread();
    tx_thread_relinquish();
}

Control &Control::instance() {
    static Control control;
    if (!control.initialized) {
        control.initialized = true;
        control.init();
    }
    return control;
}

uint8_t *Control::setup(uint8_t *first_unused_memory) {
    uint8_t *pointer = (uint8_t *)first_unused_memory;

    const size_t control_stack_size = 8192;
    tx_thread_create(&thread_control, (CHAR *)"control", thread_control_entry, 0, pointer, control_stack_size, NX_CONTROL_THREAD_PRIORITY,
                     NX_CONTROL_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);
    pointer = pointer + control_stack_size;

    return pointer;
}

bool Control::start() {
    setStartup();
    tx_thread_resume(&thread_control);
    return true;
}

void Control::thread() {
    while (1) {
        update();
        tx_thread_relinquish();
    }
}

void Control::sync() {
    switch (Model::instance().outputConfig()) {
        case Model::DUAL_STRIP: {
            for (size_t c = 0; c < Model::stripN; c++) {
                Strip::get(c).transfer();
            }
        } break;
        case Model::RGB_DUAL_STRIP: {
            Driver::instance().sync(0);
            for (size_t c = 0; c < Model::stripN; c++) {
                Strip::get(c).transfer();
            }
        } break;
        case Model::RGB_STRIP: {
            Driver::instance().sync(0);
            for (size_t c = 1; c < Model::stripN; c++) {
                Strip::get(c).transfer();
            }
        } break;
        case Model::RGBW_STRIP: {
            Driver::instance().sync(0);
            for (size_t c = 1; c < Model::stripN; c++) {
                Strip::get(c).transfer();
            }
        } break;
        case Model::RGB_RGB: {
            for (size_t c = 0; c < Model::analogN; c++) {
                Driver::instance().sync(c);
            }
        } break;
        case Model::RGBWWW: {
            for (size_t c = 0; c < 1; c++) {
                Driver::instance().sync(c);
            }
        } break;
        default: {
        } break;
    }
}

void Control::collectAllActiveArtnetUniverses(std::array<uint16_t, Model::maxUniverses> &universes, size_t &universeCount) {
    universeCount = 0;
    class UniqueCollector {
       public:
        UniqueCollector() { memset(&collected_universes[0], 0xFF, sizeof(collected_universes)); }

        void maybeAcquire(uint16_t universe) {
            for (size_t c = 0; c < Model::maxUniverses; c++) {
                if (collected_universes[c] == universe) {
                    return;
                }
                if (collected_universes[c] == 0xFFFF) {
                    collected_universes[c] = universe;
                    return;
                }
            }
        }

        void fillArray(std::array<uint16_t, Model::maxUniverses> &universes, size_t &universeCount) {
            for (size_t c = 0; c < Model::maxUniverses; c++) {
                if (collected_universes[c] == 0xFFFF) {
                    return;
                }
                universes[universeCount++] = collected_universes[c];
            }
        }

       private:
        uint16_t collected_universes[Model::maxUniverses];
    } uniqueCollector;

    switch (Model::instance().outputConfig()) {
        case Model::DUAL_STRIP: {
            for (size_t c = 0; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().artnetStrip(c, d));
                    }
                }
            }
        } break;
        case Model::RGB_DUAL_STRIP: {
            for (size_t c = 0; c < 3; c++) {
                uniqueCollector.maybeAcquire(Model::instance().analogConfig(0).components[c].artnet.universe);
            }
            for (size_t c = 0; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().artnetStrip(c, d));
                    }
                }
            }
        } break;
        case Model::RGB_STRIP: {
            for (size_t c = 0; c < 3; c++) {
                uniqueCollector.maybeAcquire(Model::instance().analogConfig(0).components[c].artnet.universe);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().artnetStrip(c, d));
                    }
                }
            }
        } break;
        case Model::RGBW_STRIP: {
            for (size_t c = 0; c < 4; c++) {
                uniqueCollector.maybeAcquire(Model::instance().analogConfig(0).components[c].artnet.universe);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().artnetStrip(c, d));
                    }
                }
            }
        } break;
        case Model::RGB_RGB: {
            for (size_t c = 0; c < Model::analogN; c++) {
                for (size_t d = 0; d < 3; d++) {
                    uniqueCollector.maybeAcquire(Model::instance().analogConfig(c).components[d].artnet.universe);
                }
            }
        } break;
        case Model::RGBWWW: {
            for (size_t c = 0; c < 1; c++) {
                for (size_t d = 0; d < 5; d++) {
                    uniqueCollector.maybeAcquire(Model::instance().analogConfig(c).components[d].artnet.universe);
                }
            }
        } break;
        default: {
        } break;
    }

    uniqueCollector.fillArray(universes, universeCount);
}

void Control::collectAllActiveE131Universes(std::array<uint16_t, Model::maxUniverses> &universes, size_t &universeCount) {
    universeCount = 0;
    class UniqueCollector {
       public:
        UniqueCollector() { memset(&collected_universes[0], 0xFF, sizeof(collected_universes)); }

        void maybeAcquire(uint16_t universe) {
            for (size_t c = 0; c < Model::maxUniverses; c++) {
                if (collected_universes[c] == universe) {
                    return;
                }
                if (collected_universes[c] == 0xFFFF) {
                    collected_universes[c] = universe;
                    return;
                }
            }
        }

        void fillArray(std::array<uint16_t, Model::maxUniverses> &universes, size_t &universeCount) {
            for (size_t c = 0; c < Model::maxUniverses; c++) {
                if (collected_universes[c] == 0xFFFF) {
                    return;
                }
                universes[universeCount++] = collected_universes[c];
            }
        }

       private:
        uint16_t collected_universes[Model::maxUniverses];
    } uniqueCollector;

    switch (Model::instance().outputConfig()) {
        case Model::DUAL_STRIP: {
            for (size_t c = 0; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().e131Strip(c, d));
                    }
                }
            }
        } break;
        case Model::RGB_DUAL_STRIP: {
            for (size_t c = 0; c < 3; c++) {
                uniqueCollector.maybeAcquire(Model::instance().analogConfig(0).components[c].e131.universe);
            }
            for (size_t c = 0; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().e131Strip(c, d));
                    }
                }
            }
        } break;
        case Model::RGB_STRIP: {
            for (size_t c = 0; c < 3; c++) {
                uniqueCollector.maybeAcquire(Model::instance().analogConfig(0).components[c].e131.universe);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().e131Strip(c, d));
                    }
                }
            }
        } break;
        case Model::RGBW_STRIP: {
            for (size_t c = 0; c < 4; c++) {
                uniqueCollector.maybeAcquire(Model::instance().analogConfig(0).components[c].e131.universe);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Strip::get(c).isUniverseActive(d, Model::instance().stripConfig(c).input_type)) {
                        uniqueCollector.maybeAcquire(Model::instance().e131Strip(c, d));
                    }
                }
            }
        } break;
        case Model::RGB_RGB: {
            for (size_t c = 0; c < Model::analogN; c++) {
                for (size_t d = 0; d < 3; d++) {
                    uniqueCollector.maybeAcquire(Model::instance().analogConfig(c).components[d].e131.universe);
                }
            }
        } break;
        case Model::RGBWWW: {
            for (size_t c = 0; c < 1; c++) {
                for (size_t d = 0; d < 5; d++) {
                    uniqueCollector.maybeAcquire(Model::instance().analogConfig(c).components[d].e131.universe);
                }
            }
        } break;
        default: {
        } break;
    }
    uniqueCollector.fillArray(universes, universeCount);
}

void Control::interateAllActiveArtnetUniverses(std::function<void(uint16_t universe)> callback) {
    size_t universeCount = 0;
    std::array<uint16_t, Model::maxUniverses> universes;
    collectAllActiveArtnetUniverses(universes, universeCount);

    for (size_t c = 0; c < universeCount; c++) {
        callback(universes[c]);
    }
}

void Control::setArtnetUniverseOutputDataForDriver(size_t terminals, size_t components, uint16_t uni, const uint8_t *data, size_t len) {
    clearStartup();

    rgbww rgb[Driver::terminalN];
    for (size_t c = 0; c < terminals; c++) {
        rgb[c] = Driver::instance().srgbwwCIE(c);
    }
    for (size_t c = 0; c < terminals; c++) {
        switch (components) {
            case 5: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[4].artnet.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[4].artnet.universe == uni) {
                        rgb[c].ww = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 4: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[3].artnet.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[3].artnet.universe == uni) {
                        rgb[c].w = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 3: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[2].artnet.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[2].artnet.universe == uni) {
                        rgb[c].b = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 2: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[1].artnet.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[1].artnet.universe == uni) {
                        rgb[c].g = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 1: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[0].artnet.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[0].artnet.universe == uni) {
                        rgb[c].r = data[channel];
                    }
                }
            }
                [[fallthrough]];
            default:
            case 0:
                break;
        }
    }
    for (size_t c = 0; c < terminals; c++) {
        Driver::instance().setRGBWW(c, rgb[c]);
        if (!syncMode) {
            Driver::instance().sync(c);
        }
    }
}

void Control::setE131UniverseOutputDataForDriver(size_t terminals, size_t components, uint16_t uni, const uint8_t *data, size_t len) {
    clearStartup();

    rgbww rgb[Driver::terminalN];
    for (size_t c = 0; c < terminals; c++) {
        rgb[c] = Driver::instance().srgbwwCIE(c);
    }
    for (size_t c = 0; c < terminals; c++) {
        switch (components) {
            case 5: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[4].e131.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[4].e131.universe == uni) {
                        rgb[c].ww = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 4: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[3].e131.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[3].e131.universe == uni) {
                        rgb[c].w = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 3: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[2].e131.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[2].e131.universe == uni) {
                        rgb[c].b = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 2: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[1].e131.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[1].e131.universe == uni) {
                        rgb[c].g = data[channel];
                    }
                }
            }
                [[fallthrough]];
            case 1: {
                size_t channel = size_t(std::clamp(Model::instance().analogConfig(c).components[0].e131.channel - 1, 0, 511));
                if (len > channel) {
                    if (Model::instance().analogConfig(c).components[0].e131.universe == uni) {
                        rgb[c].r = data[channel];
                    }
                }
            }
                [[fallthrough]];
            default:
            case 0:
                break;
        }
    }
    for (size_t c = 0; c < terminals; c++) {
        Driver::instance().setRGBWW(c, rgb[c]);
        if (!syncMode) {
            Driver::instance().sync(c);
        }
    }
}

void Control::setArtnetUniverseOutputData(uint16_t uni, const uint8_t *data, size_t len, bool nodriver) {
    clearStartup();

    switch (Model::instance().outputConfig()) {
        case Model::DUAL_STRIP: {
            for (size_t c = 0; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().artnetStrip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGB_DUAL_STRIP: {
            if (!nodriver) {
                setArtnetUniverseOutputDataForDriver(1, 3, uni, data, len);
            }
            for (size_t c = 0; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().artnetStrip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGB_STRIP: {
            if (!nodriver) {
                setArtnetUniverseOutputDataForDriver(1, 3, uni, data, len);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().artnetStrip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGBW_STRIP: {
            if (!nodriver) {
                setArtnetUniverseOutputDataForDriver(1, 4, uni, data, len);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().artnetStrip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGB_RGB: {
            if (!nodriver) {
                setArtnetUniverseOutputDataForDriver(Model::analogN, 3, uni, data, len);
            }
        } break;
        case Model::RGBWWW: {
            if (!nodriver) {
                setArtnetUniverseOutputDataForDriver(Model::analogN, 5, uni, data, len);
            }
        } break;
        default: {
        } break;
    }
}

void Control::setE131UniverseOutputData(uint16_t uni, const uint8_t *data, size_t len, bool nodriver) {
    clearStartup();

    switch (Model::instance().outputConfig()) {
        case Model::DUAL_STRIP: {
            for (size_t c = 0; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().e131Strip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGB_DUAL_STRIP: {
            if (!nodriver) {
                setE131UniverseOutputDataForDriver(1, 3, uni, data, len);
            }
            for (size_t c = 0; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().e131Strip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGB_STRIP: {
            if (!nodriver) {
                setE131UniverseOutputDataForDriver(1, 3, uni, data, len);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().e131Strip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGBW_STRIP: {
            if (!nodriver) {
                setE131UniverseOutputDataForDriver(1, 4, uni, data, len);
            }
            for (size_t c = 1; c < Model::stripN; c++) {
                bool set = false;
                for (size_t d = 0; d < Model::universeN; d++) {
                    if (Model::instance().e131Strip(c, d) == uni) {
                        Strip::get(c).setUniverseData(d, data, len, Model::instance().stripConfig(c).input_type);
                        set = true;
                    }
                }
                if (set) {
                    setDataReceived();
                }
                if (set && !syncMode) {
                    Strip::get(c).transfer();
                }
            }
        } break;
        case Model::RGB_RGB: {
            if (!nodriver) {
                setE131UniverseOutputDataForDriver(Model::analogN, 3, uni, data, len);
            }
        } break;
        case Model::RGBWWW: {
            if (!nodriver) {
                setE131UniverseOutputDataForDriver(Model::analogN, 5, uni, data, len);
            }
        } break;
        default: {
        } break;
    }
}

void Control::setColor() {
    for (size_t c = 0; c < Model::stripN; c++) {
        size_t cpp = Strip::get(c).getBytesPerPixel();
        size_t len = 0;
        switch (cpp) {
            case 3: {
                for (size_t d = 0; d <= color_buf[c].size() - 3; d += 3) {
                    color_buf[c][d + 0] = (Model::instance().stripConfig(c).color.r) & 0xFF;
                    color_buf[c][d + 1] = (Model::instance().stripConfig(c).color.g) & 0xFF;
                    color_buf[c][d + 2] = (Model::instance().stripConfig(c).color.b) & 0xFF;
                    len += 3;
                }
                Strip::get(c).setData(color_buf[c].data(), len, Model::StripConfig::StripInputType::RGB8);
            } break;
            case 4: {
                for (size_t d = 0; d <= color_buf[c].size() - 4; d += 4) {
                    color_buf[c][d + 0] = (Model::instance().stripConfig(c).color.r) & 0xFF;
                    color_buf[c][d + 1] = (Model::instance().stripConfig(c).color.g) & 0xFF;
                    color_buf[c][d + 2] = (Model::instance().stripConfig(c).color.b) & 0xFF;
                    color_buf[c][d + 3] = (Model::instance().stripConfig(c).color.x) & 0xFF;
                    len += 4;
                }
                Strip::get(c).setData(color_buf[c].data(), len, Model::StripConfig::StripInputType::RGBW8);
            } break;
            case 6: {
                for (size_t d = 0; d <= color_buf[c].size() - 6; d += 6) {
                    color_buf[c][d + 0] = color_buf[c][d + 1] = (Model::instance().stripConfig(c).color.r) & 0xFF;
                    color_buf[c][d + 2] = color_buf[c][d + 3] = (Model::instance().stripConfig(c).color.g) & 0xFF;
                    color_buf[c][d + 4] = color_buf[c][d + 5] = (Model::instance().stripConfig(c).color.b) & 0xFF;
                    len += 6;
                }
                Strip::get(c).setData(color_buf[c].data(), len, Model::StripConfig::StripInputType::RGB16_MSB);
            } break;
        }
    }
}

void Control::update() {
    if (inStartup()) {
        startupModePattern();
        sync();
    } else if (color_scheduled) {
        color_scheduled = false;
        setColor();
        switch (Model::instance().outputConfig()) {
            case Model::DUAL_STRIP: {
                for (size_t c = 0; c < Model::stripN; c++) {
                    Strip::get(c).transfer();
                }
            } break;
            case Model::RGB_DUAL_STRIP: {
                for (size_t c = 0; c < Model::stripN; c++) {
                    Strip::get(c).transfer();
                }
            } break;
            case Model::RGB_STRIP: {
                for (size_t c = 1; c < Model::stripN; c++) {
                    Strip::get(c).transfer();
                }
            } break;
            case Model::RGBW_STRIP: {
                for (size_t c = 1; c < Model::stripN; c++) {
                    Strip::get(c).transfer();
                }
            } break;
            case Model::RGB_RGB: {
            } break;
            case Model::RGBWWW: {
            } break;
            default: {
            } break;
        }
    }

    switch (Model::instance().outputConfig()) {
        case Model::DUAL_STRIP: {
            SPI_0::instance().update();
            SPI_1::instance().update();
        } break;
        case Model::RGB_DUAL_STRIP: {
            SPI_0::instance().update();
            SPI_1::instance().update();
        } break;
        case Model::RGB_STRIP: {
            SPI_0::instance().update();
        } break;
        case Model::RGBW_STRIP: {
            SPI_0::instance().update();
        } break;
        case Model::RGB_RGB: {
        } break;
        case Model::RGBWWW: {
        } break;
        default: {
        } break;
    }
}

void Control::init() {
    Strip::get(0).dmaTransferFunc = [](const uint8_t *data, size_t len) {
        SPI_0::instance().transfer(data, len, Strip::get(0).transferMpbs(), Strip::get(0).needsClock());
    };
    Strip::get(0).dmaBusyFunc = []() { return SPI_0::instance().isDMAbusy(); };

    Strip::get(1).dmaTransferFunc = [](const uint8_t *data, size_t len) {
        SPI_1::instance().transfer(data, len, Strip::get(0).transferMpbs(), Strip::get(1).needsClock());
    };
    Strip::get(1).dmaBusyFunc = []() { return SPI_1::instance().isDMAbusy(); };

    printf(ESCAPE_FG_CYAN "Control up.\n");
}

void Control::startupModePattern() {
    auto effect = [this](size_t strip) {
        switch (Model::instance().stripConfig(strip).startup_mode) {
            case Model::StripConfig::COLOR: {
                size_t l = Strip::get(strip).getPixelLen();
                size_t cpp = Strip::get(strip).getBytesPerPixel();
                for (size_t c = 0; c < l; c++) {
                    switch (cpp) {
                        case 3: {
                            color_buf[strip][c * 3 + 0] = Model::instance().stripConfig(strip).color.r;
                            color_buf[strip][c * 3 + 1] = Model::instance().stripConfig(strip).color.g;
                            color_buf[strip][c * 3 + 2] = Model::instance().stripConfig(strip).color.b;
                        } break;
                        case 4: {
                            color_buf[strip][c * 4 + 0] = Model::instance().stripConfig(strip).color.r;
                            color_buf[strip][c * 4 + 1] = Model::instance().stripConfig(strip).color.g;
                            color_buf[strip][c * 4 + 2] = Model::instance().stripConfig(strip).color.b;
                            color_buf[strip][c * 4 + 3] = Model::instance().stripConfig(strip).color.x;
                        } break;
                        case 6: {
                            color_buf[strip][c * 6 + 0] = color_buf[strip][c * 6 + 1] = (Model::instance().stripConfig(strip).color.r) & 0xFF;
                            color_buf[strip][c * 6 + 2] = color_buf[strip][c * 6 + 3] = (Model::instance().stripConfig(strip).color.g) & 0xFF;
                            color_buf[strip][c * 6 + 4] = color_buf[strip][c * 6 + 5] = (Model::instance().stripConfig(strip).color.b) & 0xFF;
                        } break;
                    }
                }
                switch (cpp) {
                    default:
                    case 3:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB8);
                        break;
                    case 4:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGBW8);
                        break;
                    case 6:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB16_MSB);
                        break;
                }
            } break;
            case Model::StripConfig::RAINBOW: {
                float h = 1.0f - fmod(float(Systick::instance().systemTime()) / 10.0f, 1.0f);
                size_t l = Strip::get(strip).getPixelLen();
                size_t cpp = Strip::get(strip).getBytesPerPixel();
                for (size_t c = 0; c < l; c++) {
                    hsv col_hsv(fmod(h + float(c) * (1.0f / 255.0f), 1.0f), 1.0f, 1.0f);
                    rgb col_rgb(col_hsv);
                    rgb8 col_rgb8(col_rgb);
                    switch (cpp) {
                        case 3: {
                            color_buf[strip][c * 3 + 0] = col_rgb8.red();
                            color_buf[strip][c * 3 + 1] = col_rgb8.green();
                            color_buf[strip][c * 3 + 2] = col_rgb8.blue();
                        } break;
                        case 4: {
                            color_buf[strip][c * 4 + 0] = col_rgb8.red();
                            color_buf[strip][c * 4 + 1] = col_rgb8.green();
                            color_buf[strip][c * 4 + 2] = col_rgb8.blue();
                            color_buf[strip][c * 4 + 3] = 0;
                        } break;
                        case 6: {
                            color_buf[strip][c * 6 + 0] = color_buf[strip][c * 6 + 1] = col_rgb8.red();
                            color_buf[strip][c * 6 + 2] = color_buf[strip][c * 6 + 3] = col_rgb8.green();
                            color_buf[strip][c * 6 + 4] = color_buf[strip][c * 6 + 5] = col_rgb8.blue();
                        } break;
                    }
                }
                switch (cpp) {
                    default:
                    case 3:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB8);
                        break;
                    case 4:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGBW8);
                        break;
                    case 6:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB16_MSB);
                        break;
                }
            } break;
            case Model::StripConfig::TRACER: {
                float h = 1.0f - fmod(float(Systick::instance().systemTime()) / 5.0f, 1.0f);
                size_t l = Strip::get(strip).getPixelLen();
                size_t cpp = Strip::get(strip).getBytesPerPixel();
                for (size_t c = 0; c < l; c++) {
                    size_t i = std::clamp(size_t(float(l) * fmod(h + (float(c) / float(l)), 1.0f)), size_t(0), l);
                    if (i == 0) {
                        switch (cpp) {
                            case 3: {
                                color_buf[strip][c * 3 + 0] = 0xFF;
                                color_buf[strip][c * 3 + 1] = 0xFF;
                                color_buf[strip][c * 3 + 2] = 0xFF;
                            } break;
                            case 4: {
                                color_buf[strip][c * 4 + 0] = 0xFF;
                                color_buf[strip][c * 4 + 1] = 0xFF;
                                color_buf[strip][c * 4 + 2] = 0xFF;
                                color_buf[strip][c * 4 + 3] = 0xFF;
                            } break;
                            case 6: {
                                color_buf[strip][c * 6 + 0] = 0xFF;
                                color_buf[strip][c * 6 + 1] = 0xFF;
                                color_buf[strip][c * 6 + 2] = 0xFF;
                                color_buf[strip][c * 6 + 4] = 0xFF;
                                color_buf[strip][c * 6 + 5] = 0xFF;
                                color_buf[strip][c * 6 + 6] = 0xFF;
                            } break;
                        }
                    } else {
                        switch (cpp) {
                            case 3: {
                                color_buf[strip][c * 3 + 0] = 0x00;
                                color_buf[strip][c * 3 + 1] = 0x00;
                                color_buf[strip][c * 3 + 2] = 0x00;
                            } break;
                            case 4: {
                                color_buf[strip][c * 4 + 0] = 0x00;
                                color_buf[strip][c * 4 + 1] = 0x00;
                                color_buf[strip][c * 4 + 2] = 0x00;
                                color_buf[strip][c * 4 + 3] = 0x00;
                            } break;
                            case 6: {
                                color_buf[strip][c * 6 + 0] = 0x00;
                                color_buf[strip][c * 6 + 1] = 0x00;
                                color_buf[strip][c * 6 + 2] = 0x00;
                                color_buf[strip][c * 6 + 3] = 0x00;
                                color_buf[strip][c * 6 + 4] = 0x00;
                                color_buf[strip][c * 6 + 5] = 0x00;
                            } break;
                        }
                    }
                }
                switch (cpp) {
                    default:
                    case 3:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB8);
                        break;
                    case 4:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGBW8);
                        break;
                    case 6:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB16_MSB);
                        break;
                }
            } break;
            case Model::StripConfig::SOLID_TRACER: {
                float h = 1.0f - fmod(float(Systick::instance().systemTime()) / 5.0f, 1.0f);
                size_t l = Strip::get(strip).getPixelLen() + 1;
                size_t cpp = Strip::get(strip).getBytesPerPixel();
                for (size_t c = 0; c < l; c++) {
                    size_t i = std::clamp(size_t(float(l) * fmod(h + (float(c) / float(l)), 1.0f)), size_t(0), l);
                    if (c < i) {
                        switch (cpp) {
                            case 3: {
                                color_buf[strip][c * 3 + 0] = 0xFF;
                                color_buf[strip][c * 3 + 1] = 0xFF;
                                color_buf[strip][c * 3 + 2] = 0xFF;
                            } break;
                            case 4: {
                                color_buf[strip][c * 4 + 0] = 0xFF;
                                color_buf[strip][c * 4 + 1] = 0xFF;
                                color_buf[strip][c * 4 + 2] = 0xFF;
                                color_buf[strip][c * 4 + 3] = 0xFF;
                            } break;
                            case 6: {
                                color_buf[strip][c * 6 + 0] = 0xFF;
                                color_buf[strip][c * 6 + 1] = 0xFF;
                                color_buf[strip][c * 6 + 2] = 0xFF;
                                color_buf[strip][c * 6 + 4] = 0xFF;
                                color_buf[strip][c * 6 + 5] = 0xFF;
                                color_buf[strip][c * 6 + 6] = 0xFF;
                            } break;
                        }
                    } else {
                        switch (cpp) {
                            case 3: {
                                color_buf[strip][c * 3 + 0] = 0x00;
                                color_buf[strip][c * 3 + 1] = 0x00;
                                color_buf[strip][c * 3 + 2] = 0x00;
                            } break;
                            case 4: {
                                color_buf[strip][c * 4 + 0] = 0x00;
                                color_buf[strip][c * 4 + 1] = 0x00;
                                color_buf[strip][c * 4 + 2] = 0x00;
                                color_buf[strip][c * 4 + 3] = 0x00;
                            } break;
                            case 6: {
                                color_buf[strip][c * 6 + 0] = 0x00;
                                color_buf[strip][c * 6 + 1] = 0x00;
                                color_buf[strip][c * 6 + 2] = 0x00;
                                color_buf[strip][c * 6 + 3] = 0x00;
                                color_buf[strip][c * 6 + 4] = 0x00;
                                color_buf[strip][c * 6 + 5] = 0x00;
                            } break;
                        }
                    }
                }
                switch (cpp) {
                    default:
                    case 3:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB8);
                        break;
                    case 4:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGBW8);
                        break;
                    case 6:
                        Strip::get(strip).setData(color_buf[strip].data(), l * cpp, Model::StripConfig::StripInputType::RGB16_MSB);
                        break;
                }
            } break;
            case Model::StripConfig::NODATA: {
            } break;
            default: {
            } break;
        }
    };

    switch (Model::instance().outputConfig()) {
        case Model::RGB_DUAL_STRIP:
        case Model::DUAL_STRIP: {
            for (size_t c = 0; c < Model::stripN; c++) {
                effect(c);
            }
        } break;
        case Model::RGB_STRIP:
        case Model::RGBW_STRIP: {
            for (size_t c = 1; c < Model::stripN; c++) {
                effect(c);
            }
        } break;
        default: {
        } break;
    }
}

#endif  // #ifndef BOOTLOADER
