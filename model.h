/*
Copyright 2019 Tinic Uro

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

struct Model {
   private:
    uint32_t model_version;

   public:
    static constexpr uint32_t currentModelVersion = 0x1ed50000;

    static constexpr size_t stripN = 2;
    static constexpr size_t analogN = 2;
    static constexpr size_t universeN = 6;
    static constexpr size_t analogCompN = 6;
    static constexpr size_t maxUniverses = stripN * universeN + analogN * analogCompN;

    struct AnalogConfig {
        // clang-format off
        enum AnalogOutputType { 
            RGB, 
            RGBW, 
            RGBWW, 
            RGBWWW,
            OUTPUT_COUNT};

        enum AnalogInputType { 
            RGB8, 
            RGBW8, 
            RGBWW8, 
            RGBWWW8, 
            RGB16_MSB, 
            RGBW8_MSB, 
            RGBWW8_MSB, 
            RGBWWW8_MSB, 
            RGB8_LSB, 
            RGBW8_LSB, 
            RGBWW8_LSB, 
            RGBWWW8_LSB,
            INPUT_COUNT};
        // clang-format on

        AnalogOutputType output_type;
        AnalogInputType input_type;
        float pwm_limit;
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
    } analog_config[analogN];

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
            OUTPUT_COUNT};

        enum StripInputType { 
            RGB8, 
            RGBW8, 
            RGB8_SRGB, 
            RGBW_SRGB, 
            RGB16_MSB, 
            RGBW16_MSB, 
            RGB16_LSB, 
            RGBW16_LSB,
            INPUT_COUNT};

        enum StripStartupMode { 
            COLOR, 
            RAINBOW, 
            TRACER, 
            SOLID_TRACER, 
            NODATA,
            STARTUP_COUNT};
        // clang-format on

        StripOutputType output_type;
        StripInputType input_type;
        StripStartupMode startup_mode;
        float comp_limit;
        float glob_illum;
        uint16_t len;
        uint16_t artnet[universeN];
        uint16_t e131[universeN];
    } strip_config[stripN];

    // clang-format off
    static constexpr struct StripOutputProperties {
        StripConfig::StripOutputType type;
        bool clock;
        bool globalillum;
        uint8_t bitslen;
        uint8_t components;
    } stripOutputProperties[StripConfig::OUTPUT_COUNT] = {
        {StripConfig::WS2812,  false, false,  8, 3},  
        {StripConfig::SK6812,  false, false,  8, 3},  
        {StripConfig::TM1804,  false, false,  8, 3}, 
        {StripConfig::UCS1904, false, false,  8, 3},
        {StripConfig::GS8202,  false, false,  8, 3},  
        {StripConfig::APA102,  true,  true,   8, 3},   
        {StripConfig::APA107,  true,  true,   8, 3},
        {StripConfig::P9813,   true,  false,  8, 3},    
        {StripConfig::SK9822,  true,  true,   8, 3},    
        {StripConfig::HDS107S, true,  false,  8, 3}, 
        {StripConfig::LPD8806, false, false,  8, 3},
        {StripConfig::TLS3001, false, false,  8, 3}, 
        {StripConfig::TM1829,  false, false, 16, 3}, 
        {StripConfig::WS2801,  true,  false,  8, 3},  
        {StripConfig::HD108,   true,  false, 16, 3}};
    // clang-format on

    enum OutputConfig {
        DUAL_STRIP,      // channel0: strip     channel1: strip
        RGB_STRIP,       // channel0: strip     channel1: rgb
        RGB_DUAL_STRIP,  // channel0: single	channel1: single     channel2: rgb
        RGBW_STRIP,      // channel0: single	channel1: rgbw
        RGB_RGB,         // channel0: rgb 	    channel1: rgb
        RGBWWW,          // channel0: rgbwww
        CONFIG_COUNT
    } output_config;

    // clang-format off
    static constexpr struct OutputConfigProperties {
        static constexpr size_t OutputConfigMaxDevices = 3;
        const char *label;
        uint8_t strip_count;
        uint8_t analog_count;
        bool is_strip[OutputConfigMaxDevices];
        bool is_rgb[OutputConfigMaxDevices];
        bool can_have_clock[OutputConfigMaxDevices];
        uint8_t max_comp[OutputConfigMaxDevices];
    } outputConfigProperties[CONFIG_COUNT] = {
        {"2 x RGB Strip",                    2, 0, {true,  true,  false}, {false, false, false}, {true,   true,  false}, {0, 0, 0}},
        {"1 x RGB Strip + 1 x Analog RGB",   1, 1, {true,  false, false}, {false, true,  false}, {false,  true,  false}, {0, 3, 0}},
        {"2 x RGB Strip + 1 x Analog RGB",   2, 1, {true,  true,  false}, {false, false, true }, {false,  false, false}, {0, 3, 0}},
        {"1 x RGB Strip + 1 x Analog RGB+W", 1, 1, {true,  false, false}, {false, true,  false}, {false,  false, false}, {0, 4, 0}},
        {"2 x Analog RGB",                   0, 2, {false, false, false}, {true,  true,  false}, {false,  false, false}, {3, 3, 0}},
        {"1 x Analog RGB+W+WW",              0, 1, {false, false, false}, {true,  false, false}, {false,  false, false}, {6, 0, 0}},
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
        struct OutputConfigPinLabel {
            const char *label0;
            const char *label1;
        };
        static constexpr size_t OutputConfigPinCount = 8;
        OutputConfigPinLabel pinlabel_without_clock[OutputConfigPinCount];
        OutputConfigPinLabel pinlabel_with_clock[OutputConfigPinCount];
    } outputConfigPinNames[CONFIG_COUNT] = {
        { { /*s0*/ 0x02, 0x01, /*s1*/ 0x06, 0x05, /*a0*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /*a1*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
          { /*0*/"GND" , "GND" , /*1*/"---" , "NIL" , /*1*/"DAT0", "DAT" , /*2*/"VCC" , "VCC" , /*3*/"GND" , "GND" , /*4*/"---" , "NIL" , /*5*/"DAT1", "GND" , /*6*/"VCC" , "VCC" },
          { /*0*/"GND" , "GND" , /*1*/"CLK0", "CLK" , /*1*/"DAT0", "DAT" , /*2*/"VCC" , "VCC" , /*3*/"GND" , "GND" , /*4*/"CLk1", "CLK" , /*5*/"DAT1", "GND" , /*6*/"VCC" , "VCC" } },
        { { /*s0*/ 0x05, 0x04, /*s1*/ 0xFF, 0xFF, /*a0*/ 0x00, 0x01, 0x02, 0xFF, 0xFF, 0xFF, /*a1*/ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
          { /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*1*/"GRN" , "GRN" , /*2*/"VCC" , "VCC" , /*3*/"GND" , "GND" , /*4*/"---" , "NIL" , /*5*/"DAT1", "GND" , /*6*/"VCC" , "VCC" },
          { /*0*/"BLU" , "BLU" , /*1*/"RED" , "RED" , /*1*/"GRN" , "GRN" , /*2*/"VCC" , "VCC" , /*3*/"GND" , "GND" , /*4*/"CLk1", "CLK" , /*5*/"DAT1", "GND" , /*6*/"VCC" , "VCC" } },
    };
    // clang-format on

/*     pinTable: [
        [ 
          [{t:"GND", c:"GND"},{t:"---", c:"NIL"},{t:"DAT0",c:"DAT"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"---", c:"NIL"},{t:"DAT1",c:"DAT"},{t:"VCC",c:"VCC"}],
          [{t:"GND", c:"GND"},{t:"CLK0",c:"CLK"},{t:"DAT0",c:"DAT"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"CLK1",c:"CLK"},{t:"DAT1",c:"DAT"},{t:"VCC",c:"VCC"}]
        ],[
          [{t:"BLU", c:"BLU"},{t:"RED", c:"RED"},{t:"GRN", c:"GRN"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"---", c:"NIL"},{t:"DAT", c:"DAT"},{t:"VCC",c:"VCC"}],
          [{t:"BLU", c:"BLU"},{t:"RED", c:"RED"},{t:"GRN", c:"GRN"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"CLK", c:"CLK"},{t:"DAT", c:"DAT"},{t:"VCC",c:"VCC"}]
        ],[
          [{t:"BLU", c:"BLU"},{t:"RED", c:"RED"},{t:"DAT0",c:"DAT"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"GRN", c:"GRN"},{t:"DAT1",c:"DAT"},{t:"VCC",c:"VCC"}],
          [{t:"BLU", c:"BLU"},{t:"RED", c:"RED"},{t:"DAT0",c:"DAT"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"GRN", c:"GRN"},{t:"DAT1",c:"DAT"},{t:"VCC",c:"VCC"}]
        ],[
          [{t:"BLU", c:"BLU"},{t:"RED", c:"RED"},{t:"GRN", c:"GRN"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"WHT", c:"WHT"},{t:"DAT", c:"DAT"},{t:"VCC",c:"VCC"}],
          [{t:"BLU", c:"BLU"},{t:"RED", c:"RED"},{t:"GRN", c:"GRN"},{t:"VCC",c:"VCC"},{t:"GND", c:"GND"},{t:"WHT", c:"WHT"},{t:"DAT", c:"DAT"},{t:"VCC",c:"VCC"}]
        ],[
          [{t:"BLU0",c:"BLU"},{t:"RED0",c:"RED"},{t:"GRN0",c:"GRN"},{t:"VCC",c:"VCC"},{t:"BLU1",c:"BLU"},{t:"RED1",c:"RED"},{t:"GRN1",c:"GRN"},{t:"VCC",c:"VCC"}],
          [{t:"BLU0",c:"BLU"},{t:"RED0",c:"RED"},{t:"GRN0",c:"GRN"},{t:"VCC",c:"VCC"},{t:"BLU1",c:"BLU"},{t:"RED1",c:"RED"},{t:"GRN1",c:"GRN"},{t:"VCC",c:"VCC"}]
        ],[
          [{t:"BLU0",c:"BLU"},{t:"RED0",c:"RED"},{t:"GRN0",c:"GRN"},{t:"VCC",c:"VCC"},{t:"WHT",c:"WHT"},{t:"WWHT",c:"WHT"},{t:"---",c:"NIL"},{t:"VCC",c:"VCC"}],
          [{t:"BLU0",c:"BLU"},{t:"RED0",c:"RED"},{t:"GRN0",c:"GRN"},{t:"VCC",c:"VCC"},{t:"WHT",c:"WHT"},{t:"WWHT",c:"WHT"},{t:"---",c:"NIL"},{t:"VCC",c:"VCC"}]
        ]
      ]
*/
    static Model &instance();

    StripConfig &stripConfig(size_t index) { return strip_config[index]; }
    AnalogConfig &analogConfig(size_t index) { return analog_config[index]; }

    OutputConfig outputConfig() const { return output_config; }
    void setOutputConfig(OutputConfig outputConfig);

    uint16_t artnetStrip(int32_t strip, int32_t dmx512Index) const {
        strip %= stripN;
        dmx512Index %= universeN;
        return strip_config[strip].artnet[dmx512Index];
    }

    uint16_t e131Strip(int32_t strip, int32_t dmx512Index) const {
        strip %= stripN;
        dmx512Index %= universeN;
        return strip_config[strip].e131[dmx512Index];
    }

   private:
    Model(){};
    void init();
    bool initialized = false;
};

#endif /* _MODEL_H_ */
