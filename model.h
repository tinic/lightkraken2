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
#ifndef _MODEL_H_
#define _MODEL_H_

#include <stdint.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <fixed_containers/fixed_string.hpp>
#include <fixed_containers/fixed_vector.hpp>
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <magic_enum.hpp>
#pragma GCC diagnostic pop

#include "./color.h"

struct Model {
   public:
    static constexpr size_t stripN = 2;
    static constexpr size_t analogN = 2;
    static constexpr size_t universeN = 16;
    static constexpr size_t analogCompN = 6;
    static constexpr size_t maxUniverses = stripN * universeN + analogN * analogCompN;
    static constexpr size_t maxLEDs = 512 * universeN;
    static constexpr size_t maxUniverseID = 65535;

    bool broadcastEnabled = false;
    bool burstMode = false;

    struct AnalogConfig {
        // clang-format off
        enum AnalogOutputType { 
            RGB, 
            RGBW, 
            RGBWWW};

        enum AnalogInputType { 
            RGB8, 
            RGBW8, 
            RGBWWW8, 
            RGB8_SRGB, 
            RGBW8_SRGB, 
            RGBWWW8_SRGB, 
            RGB16_MSB, 
            RGBW16_MSB, 
            RGBWWW16_MSB};
        // clang-format on

        AnalogOutputType output_type;
        AnalogInputType input_type;
        float pwm_limit;
        RGBColorSpace rgbSpace;
        struct Component {
            struct {
                uint16_t universe;
                uint16_t channel;
            } artnet;
            struct {
                uint16_t universe;
                uint16_t channel;
            } e131;
            uint16_t value;
        } components[analogCompN];
    } analog_config[analogN] = {
        // clang-format off
        { AnalogConfig::RGB, AnalogConfig::RGB8, 1.0,
            RGBColorSpace(),
            { { 0,  0, 1,  0, 0 }, 
              { 0,  1, 1,  1, 0 }, 
              { 0,  2, 1,  2, 0 }, 
              { 0,  3, 1,  3, 0 }, 
              { 0,  4, 1,  4, 0 }, 
              { 0,  5, 1,  5, 0 } } },
        { AnalogConfig::RGB, AnalogConfig::RGB8, 1.0, 
            RGBColorSpace(),
            { { 0,  6, 1,  6, 0 }, 
              { 0,  7, 1,  7, 0 }, 
              { 0,  8, 1,  8, 0 }, 
              { 0,  9, 1,  9, 0 }, 
              { 0, 10, 1, 10, 0 }, 
              { 0, 11, 1, 11, 0 } } },
        // clang-format on
    };

    struct StripConfig {
        // clang-format off
        enum StripOutputType { 
            WS2812, 
            SK6812, 
            TM1804, 
            UCS1904, 
            GS8202, 
            APA102, 
            APA107, 
            P9813, 
            SK9822, 
            HDS107S, 
            LPD8806, 
            TLS3001, 
            TM1829, 
            WS2801, 
            HD108,
            WS2816,
            SK6812_RGBW,
            WS2811};

        enum StripInputType { 
            RGB8, 
            RGBW8, 
            RGB8_SRGB, 
            RGBW_SRGB, 
            RGB16_MSB, 
            RGBW16_MSB, 
            RGB16_LSB, 
            RGBW16_LSB};

        enum StripStartupMode { 
            COLOR, 
            RAINBOW, 
            TRACER, 
            SOLID_TRACER, 
            NODATA};

        enum StripNativeType { 
            NATIVE_RGB8, 
            NATIVE_RGBW8, 
            NATIVE_RGB16};
        // clang-format on

        StripOutputType output_type;
        StripInputType input_type;
        StripStartupMode startup_mode;
        float comp_limit;
        float glob_illum;
        uint16_t led_count;
        uint32_t mbps;
        rgb8 color;
        RGBColorSpace rgbSpace;
        uint16_t artnet[universeN];
        uint16_t e131[universeN];
    } strip_config[stripN] = {
        {StripConfig::HD108, StripConfig::RGB8, StripConfig::RAINBOW, 1.0, 1.0, 255, 20000000, rgb8(), RGBColorSpace(), {0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0}},
        {StripConfig::HD108, StripConfig::RGB8, StripConfig::RAINBOW, 1.0, 1.0, 255, 20000000, rgb8(), RGBColorSpace(), {1, 0, 0, 0, 0, 0}, {2, 0, 0, 0, 0, 0}},
    };

    // clang-format off
    static constexpr struct StripInputProperties {
        StripConfig::StripInputType type;
        uint8_t bytes_per_comp;
        uint8_t bytes_per_pixel;
        uint8_t comp_per_pixel;
    } stripInputProperties[magic_enum::enum_count<StripConfig::StripInputType>()] = {
        { StripConfig::RGB8,       3, 1, 3 },
        { StripConfig::RGBW8,      4, 1, 4 },
        { StripConfig::RGB8_SRGB,  3, 1, 3 },
        { StripConfig::RGBW_SRGB,  4, 1, 4 },
        { StripConfig::RGB16_MSB,  6, 2, 3 },
        { StripConfig::RGBW16_MSB, 8, 2, 4 },
        { StripConfig::RGB16_LSB,  6, 2, 3 },
        { StripConfig::RGBW16_LSB, 8, 2, 4 }
    };
    // clang-format on

    // clang-format off
    static constexpr struct StripOutputProperties {
        StripConfig::StripOutputType type;
        StripConfig::StripNativeType native_type;
        uint8_t bits_per_comp;
        uint8_t bytes_per_pixel;
        uint8_t comp_per_pixel;
        fixed_containers::FixedVector<size_t, 4> rgbw_order;
        bool has_clock;
        bool globalillum;
        float spi_mpbs_factor;
        uint32_t min_mbps;
        uint32_t max_mbps;
        uint32_t default_mpbs;
    } stripOutputProperties[magic_enum::enum_count<StripConfig::StripOutputType>()] = {
        {StripConfig::WS2812,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    }, false, false, 4.0f, 700000,   900000,  800000 },  
        {StripConfig::SK6812,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    }, false, false, 4.0f, 700000,   900000,  800000 },  
        {StripConfig::TM1804,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    }, false, false, 4.0f, 700000,   900000,  800000 }, 
        {StripConfig::UCS1904,     StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    }, false, false, 4.0f, 700000,   900000,  800000 },
        {StripConfig::GS8202,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    }, false, false, 4.0f, 700000,   900000,  800000 },  
        {StripConfig::APA102,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 2, 1, 0    },  true,  true, 1.0f,   1000, 20000000, 2000000 },   
        {StripConfig::APA107,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 2, 1, 0    },  true,  true, 1.0f,   1000, 30000000, 3000000 },
        {StripConfig::P9813,       StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    },  true, false, 1.0f, 700000,   900000,  800000 },    
        {StripConfig::SK9822,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    },  true,  true, 1.0f,   1000, 15000000, 1000000 },    
        {StripConfig::HDS107S,     StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    },  true, false, 1.0f,   1000, 40000000, 2000000 }, 
        {StripConfig::LPD8806,     StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    },  true, false, 4.0f, 700000, 20000000, 2000000 },
        {StripConfig::TLS3001,     StripConfig::NATIVE_RGB8,   8, 4, 3, { 0, 1, 2    }, false, false, 2.0f, 500000,  1000000,  750000 }, 
        {StripConfig::TM1829,      StripConfig::NATIVE_RGB8,  16, 4, 3, { 2, 1, 0    }, false, false, 4.0f, 800000,   800000,  800000 }, 
        {StripConfig::WS2801,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    },  true, false, 1.0f, 700000,   900000,  800000 },  
        {StripConfig::HD108,       StripConfig::NATIVE_RGB16, 16, 6, 3, { 0, 1, 2    },  true, false, 1.0f, 700000, 20000000, 1000000 },
        {StripConfig::WS2816,      StripConfig::NATIVE_RGB16, 16, 6, 3, { 1, 0, 2    }, false, false, 4.0f, 700000,   900000,  800000 },
        {StripConfig::SK6812_RGBW, StripConfig::NATIVE_RGBW8,  8, 4, 4, { 1, 0, 2, 3 }, false, false, 4.0f, 700000,   900000,  800000 },
        {StripConfig::WS2811,      StripConfig::NATIVE_RGB8,   8, 4, 3, { 1, 0, 2    }, false, false, 4.0f, 700000,   900000,  800000 }};
    // clang-format on

    enum OutputConfig {
        DUAL_STRIP,      // channel0: strip     channel1: strip
        RGB_STRIP,       // channel0: strip     channel1: rgb
        RGB_DUAL_STRIP,  // channel0: single	channel1: single     channel2: rgb
        RGBW_STRIP,      // channel0: single	channel1: rgbw
        RGB_RGB,         // channel0: rgb 	    channel1: rgb
        RGBWWW           // channel0: rgbwww
    } output_config = DUAL_STRIP;

    // clang-format off
    static constexpr struct OutputConfigProperties {
        static constexpr size_t OutputConfigMaxDevices = 3;
        const char *label;
        OutputConfig output_config;
        uint8_t strip_n;
        uint8_t analog_n;
        bool strip[OutputConfigMaxDevices];
        bool analog[OutputConfigMaxDevices];
        bool has_clock[OutputConfigMaxDevices];
        uint8_t components[OutputConfigMaxDevices];
    } outputConfigProperties[magic_enum::enum_count<OutputConfig>()] = {
        {"2 x RGB Strip",                    DUAL_STRIP,     2, 0, {true,  true,  false}, {false, false, false}, {true,   true,  false}, {0, 0, 0}},
        {"1 x RGB Strip + 1 x Analog RGB",   RGB_STRIP,      1, 1, {true,  false, false}, {false, true,  false}, {false,  true,  false}, {0, 3, 0}},
        {"2 x RGB Strip + 1 x Analog RGB",   RGB_DUAL_STRIP, 2, 1, {true,  true,  false}, {false, false, true }, {false,  false, false}, {0, 3, 0}},
        {"1 x RGB Strip + 1 x Analog RGB+W", RGBW_STRIP,     1, 1, {true,  false, false}, {false, true,  false}, {false,  false, false}, {0, 4, 0}},
        {"2 x Analog RGB",                   RGB_RGB,        0, 2, {false, false, false}, {true,  true,  false}, {false,  false, false}, {3, 3, 0}},
        {"1 x Analog RGB+W+WW",              RGBWWW,         0, 1, {false, false, false}, {true,  false, false}, {false,  false, false}, {6, 0, 0}},
    };
    // clang-format on

    // clang-format off
    static constexpr struct OutputConfigPinNames {
        struct OutputConfigPinAssign {
            uint8_t dat0;
            uint8_t clk0;

            uint8_t dat1;
            uint8_t clk1;

            uint8_t red0;
            uint8_t grn0;
            uint8_t blu0;
            uint8_t wht0;
            uint8_t wwt0;
            uint8_t whw0;

            uint8_t red1;
            uint8_t grn1;
            uint8_t blu1;
            uint8_t wht1;
            uint8_t wwt1;
            uint8_t whw1;
        };
        OutputConfigPinAssign pin_assign;
        static constexpr size_t OutputConfigPinCount = 8;
        struct PinLabels {
            struct OutputConfigPinLabelNoClock {
                const char *l;
                const char *s;
            } no_clock[OutputConfigPinCount];
            struct OutputConfigPinLabelWithClock {
                const char *l;
                const char *s;
            } with_clock[OutputConfigPinCount];
        } pinlabels;
    } outputConfigPinNames[magic_enum::enum_count<OutputConfig>()] = {
        { { /*s0*/ 0x02, 0x01, /*s1*/ 0x06, 0x05, /*a0*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*a1*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
          {{ /*0*/"GND" , "GND" , /*1*/"---" , "NIL" , /*2*/"DAT0", "DAT" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"---" , "NIL" , /*6*/"DAT1", "DAT" , /*7*/"VCC" , "VCC" },
           { /*0*/"GND" , "GND" , /*1*/"CLK0", "CLK" , /*2*/"DAT0", "DAT" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"CLK1", "CLK" , /*6*/"DAT1", "DAT" , /*7*/"VCC" , "VCC" }} },
        { { /*s0*/ 0x06, 0x05, /*s1*/ 0xFF, 0xFF, /*a0*/ 0x00, 0x01, 0x02, 0xFF, 0xFF, 0xFF, /*a1*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
          {{ /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*2*/"GRN" , "GRN" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"---" , "NIL" , /*6*/"DAT1", "DAT" , /*7*/"VCC" , "VCC" },
           { /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*2*/"GRN" , "GRN" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"CLk1", "CLK" , /*6*/"DAT1", "DAT" , /*7*/"VCC" , "VCC" }} },
        { { /*s0*/ 0x01, 0xFF, /*s1*/ 0x06, 0xFF, /*a0*/ 0x00, 0x01, 0x05, 0xFF, 0xFF, 0xFF, /*a1*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
          {{ /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*2*/"DAT0", "DAT" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"GRN" , "GRN" , /*6*/"DAT1", "DAT" , /*7*/"VCC" , "VCC" },
           { /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*2*/"DAT0", "DAT" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"GRN" , "GRN" , /*6*/"DAT1", "DAT" , /*7*/"VCC" , "VCC" }} },
        { { /*s0*/ 0x06, 0xFF, /*s1*/ 0xFF, 0xFF, /*a0*/ 0x00, 0x01, 0x02, 0x05, 0xFF, 0xFF, /*a1*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
          {{ /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*2*/"GRN" , "GRN" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"WHT" , "WHT" , /*6*/"DAT" , "DAT" , /*7*/"VCC" , "VCC" },
           { /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*2*/"GRN" , "GRN" , /*3*/"VCC" , "VCC" , /*4*/"GND" , "GND" , /*5*/"WHT" , "WHT" , /*6*/"DAT" , "DAT" , /*7*/"VCC" , "VCC" }} },
        { { /*s0*/ 0xFF, 0xFF, /*s1*/ 0xFF, 0xFF, /*a0*/ 0x00, 0x01, 0x02, 0xFF, 0xFF, 0xFF, /*a1*/ 0x04, 0x05, 0x06, 0xFF, 0xFF, 0xFF },
          {{ /*0*/"BLU0", "BLU" , /*1*/"RED0", "RED" , /*2*/"GRN0", "GRN" , /*3*/"VCC" , "VCC" , /*4*/"BLU1", "BLU" , /*5*/"RED1", "RED" , /*6*/"GRN1", "GRN" , /*7*/"VCC" , "VCC" },
           { /*0*/"BLU0", "BLU" , /*1*/"RED0", "RED" , /*2*/"GRN0", "GRN" , /*3*/"VCC" , "VCC" , /*4*/"BLU1", "BLU" , /*5*/"RED1", "RED" , /*6*/"GRN1", "GRN" , /*7*/"VCC" , "VCC" }} },
        { { /*s0*/ 0xFF, 0xFF, /*s1*/ 0xFF, 0xFF, /*a0*/ 0x00, 0x01, 0x02, 0x04, 0x05, 0x06, /*a1*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
          {{ /*0*/"BLU0", "BLU" , /*1*/"RED0", "RED" , /*2*/"GRN0", "GRN" , /*3*/"VCC" , "VCC" , /*4*/"WHT0", "WHT" , /*5*/"WHT1", "WHT" , /*6*/"WHT2", "WHT" , /*7*/"VCC" , "VCC" },
           { /*0*/"BLU0", "BLU" , /*1*/"RED0", "RED" , /*2*/"GRN0", "GRN" , /*3*/"VCC" , "VCC" , /*4*/"WHT0", "WHT" , /*5*/"WHT1", "WHT" , /*6*/"WHT2", "WHT" , /*7*/"VCC" , "VCC" }} },
    };
    // clang-format on

    static Model &instance();

    StripConfig &stripConfig(size_t index) { return strip_config[index]; }
    AnalogConfig &analogConfig(size_t index) { return analog_config[index]; }

    OutputConfig outputConfig() const { return output_config; }
    void setOutputConfig(OutputConfig outputConfig);

    uint16_t artnetStrip(size_t strip, size_t dmx512Index) const {
        strip %= stripN;
        dmx512Index %= universeN;
        return strip_config[strip].artnet[dmx512Index];
    }

    uint16_t e131Strip(size_t strip, size_t dmx512Index) const {
        strip %= stripN;
        dmx512Index %= universeN;
        return strip_config[strip].e131[dmx512Index];
    }

    bool importFromDB();
    void exportToDB();
    void exportStaticsToDB();
    void applyToControl();

   private:
    Model(){};
    void init();
    bool initialized = false;
};

#endif /* _MODEL_H_ */
