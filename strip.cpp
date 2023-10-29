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
#include "./strip.h"

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "./color.h"
#include "./model.h"

#define __assume(cond)                        \
    do {                                      \
        if (!(cond)) __builtin_unreachable(); \
    } while (0)

static constexpr size_t ws2816b_error_extent_n = 438;
static constexpr std::array<uint16_t, ws2816b_error_extent_n> make_ws2816b_error_lut() {
    std::array<uint16_t, ws2816b_error_extent_n> lut = {0};
    for (size_t c = 0; c < lut.size(); c++) {
        lut[c] = uint16_t((c * 255) / lut.size());
    }
    return lut;
};

static ColorSpaceConverter converter;

class manchester_bit_buf {
   public:
    manchester_bit_buf(uint8_t *p) {
        buf = p;
        byte = 0;
        bit_pos = 7;
        byte_pos = 0;
        man_pos = 0;
    }

    __attribute__((hot, optimize("O2"))) void push(uint32_t bits, int32_t count) {
        for (int32_t c = 0; c < count; c++) {
            for (int32_t d = 0; d < 2; d++) {
                byte |= ((man_pos & 1) ^ ((bits & (1UL << 31)) ? 1 : 0)) ? (1UL << bit_pos) : 0;
                bit_pos--;
                if (bit_pos < 0) {
                    buf[byte_pos++] = byte;
                    byte = 0;
                    bit_pos = 7;
                }
                man_pos++;
            }
            bits <<= 1;
        }
    }

    void flush() {
        buf[byte_pos++] = byte;
        byte = 0;
        bit_pos = 7;
    }

    size_t len() { return byte_pos; }

   private:
    uint8_t *buf = 0;
    uint8_t byte = 0;
    size_t byte_pos = 0;
    int32_t bit_pos = 0;
    int32_t man_pos = 0;
};

Strip &Strip::get(size_t index) {
    static Strip strips[Model::stripN];
    static bool init = false;
    if (!init) {
        init = true;
        for (size_t c = 0; c < Model::stripN; c++) {
            strips[c].init();
        }
    }
    return strips[index % Model::stripN];
}

bool Strip::ws2812_lut_init = false;
std::array<uint32_t, 256> Strip::ws2812_lut;
bool Strip::hd108_lut_init = false;
std::array<std::array<uint16_t, 256>, 3> Strip::hd108_lut;

void Strip::init() {
    comp_buf.fill(0);
    spi_buf.fill(0);
    transfer_flag = false;
    RGBColorSpace rgbSpace;
    rgbSpace.setsRGB();
    converter.setRGBColorSpace(rgbSpace);
    if (!ws2812_lut_init) {
        ws2812_lut_init = true;
        auto make_ws2812_table = []() constexpr -> std::array<uint32_t, 256> {
            std::array<uint32_t, 256> table = {0};
            for (uint32_t c = 0; c < 256; c++) {
                table[c] =
                    0x88888888 | (((c >> 4) | (c << 6) | (c << 16) | (c << 26)) & 0x04040404) | (((c >> 1) | (c << 9) | (c << 19) | (c << 29)) & 0x40404040);
            }
            return table;
        };
        // Make a RAM copy; gets us a slight perf improvement
        ws2812_lut = make_ws2812_table();
    }
    if (!hd108_lut_init) {
        hd108_lut_init = true;
        auto make_hd108_table = []() constexpr -> std::array<std::array<uint16_t, 256>, 3> {
            std::array<std::array<uint16_t, 256>, 3> lut{};
            double r_const = 1.000;
            double g_const = 0.760;
            double b_const = 0.550;

            double ga_const = exp(-g_const) - 1.0;
            double gai_const = +1.0 / ga_const;
            double gbi_const = -1.0 / g_const;

            double ba_const = exp(-b_const) - 1.0;
            double bai_const = +1.0 / ba_const;
            double bbi_const = -1.0 / b_const;

            for (size_t d = 0; d < 256; d++) {
                double t = double(d) / 255.0;
                // R
                lut[0][d] = uint16_t(pow(t * r_const, 2.4) * 65535.0);
                // G
                lut[1][d] = uint16_t(pow((log((t + gai_const) * ga_const) * gbi_const), 2.4) * 65535.0);
                // B
                lut[2][d] = uint16_t(pow((log((t + bai_const) * ba_const) * bbi_const), 2.4) * 65535.0);
            }
            return lut;
        };
        // Make a RAM copy; gets us a slight perf improvement
        hd108_lut = make_hd108_table();
    }
}

void Strip::setRGBColorSpace(const RGBColorSpace &colorSpace) { converter.setRGBColorSpace(colorSpace); }

size_t Strip::getBytesPerPixel() const {
    switch (output_type) {
        case Model::StripConfig::StripOutputType::SK6812_RGBW: {
            return 4;
        } break;
        default:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::GS8202:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::UCS1904:
        case Model::StripConfig::StripOutputType::TLS3001:
        case Model::StripConfig::StripOutputType::LPD8806:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::TM1829:
        case Model::StripConfig::StripOutputType::APA102:
        case Model::StripConfig::StripOutputType::WS2801: {
            return 3;
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::WS2816: {
            return 6;
        } break;
    }
    return 0;
}

Strip::NativeType Strip::nativeType() const {
    switch (output_type) {
        case Model::StripConfig::StripOutputType::SK6812_RGBW: {
            return NATIVE_RGBW8;
        } break;
        default:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::GS8202:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::TM1829:
        case Model::StripConfig::StripOutputType::UCS1904:
        case Model::StripConfig::StripOutputType::LPD8806:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::APA102:
        case Model::StripConfig::StripOutputType::WS2801: {
            return NATIVE_RGB8;
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::WS2816: {
            return NATIVE_RGB16;
        } break;
    }
    return NATIVE_RGB8;
}

size_t Strip::getPixelLen() const {
    const size_t pixsize = getBytesPerPixel();
    return bytes_len / pixsize;
}

size_t Strip::getMaxPixelLen() const {
    const size_t pixsize = getBytesPerPixel();
    const size_t pixpad = size_t(dmxMaxLen / pixsize);
    return pixpad * Model::universeN;
}

void Strip::setPixelLen(size_t len) {
    const size_t pixsize = getBytesPerPixel();
    const size_t c_len = len * pixsize;
    setBytesLen(c_len);
}

size_t Strip::getMaxBytesLen() const {
    const size_t pixsize = getBytesPerPixel();
    const size_t pixpad = size_t(dmxMaxLen / pixsize) * pixsize;
    return pixpad * Model::universeN;
}

void Strip::setBytesLen(size_t len) {
    bytes_len = std::min(getMaxBytesLen(), size_t(len));
    memset(&comp_buf.data()[bytes_len], 0, comp_buf.size() - bytes_len);
}

bool Strip::isUniverseActive(size_t uniN, Model::StripConfig::StripInputType input_type) const {
    const size_t pixsize = getBytesPerInputPixel(input_type);
    const size_t pixpad = size_t(dmxMaxLen / pixsize);
    if (uniN * pixpad < getPixelLen()) {
        return true;
    }
    return false;
}

size_t Strip::getBytesPerInputPixel(Model::StripConfig::StripInputType input_type) const {
    switch (input_type) {
        default:
        case Model::StripConfig::StripInputType::RGB8_SRGB:
        case Model::StripConfig::StripInputType::RGB8: {
            return 3;
        } break;
        case Model::StripConfig::StripInputType::RGBW_SRGB:
        case Model::StripConfig::StripInputType::RGBW8: {
            return 4;
        } break;
        case Model::StripConfig::StripInputType::RGB16_LSB:
        case Model::StripConfig::StripInputType::RGB16_MSB: {
            return 6;
        } break;
        case Model::StripConfig::StripInputType::RGBW16_LSB:
        case Model::StripConfig::StripInputType::RGBW16_MSB: {
            return 8;
        }
    }
    return 0;
}

size_t Strip::getComponentsPerInputPixel(Model::StripConfig::StripInputType input_type) const {
    switch (input_type) {
        default:
        case Model::StripConfig::StripInputType::RGB16_LSB:
        case Model::StripConfig::StripInputType::RGB16_MSB:
        case Model::StripConfig::StripInputType::RGB8_SRGB:
        case Model::StripConfig::StripInputType::RGB8: {
            return 3;
        } break;
        case Model::StripConfig::StripInputType::RGBW16_LSB:
        case Model::StripConfig::StripInputType::RGBW16_MSB:
        case Model::StripConfig::StripInputType::RGBW_SRGB:
        case Model::StripConfig::StripInputType::RGBW8: {
            return 4;
        } break;
    }
    return 0;
}

size_t Strip::getComponentBytes(Model::StripConfig::StripInputType input_type) const {
    switch (input_type) {
        default:
        case Model::StripConfig::StripInputType::RGBW16_LSB:
        case Model::StripConfig::StripInputType::RGBW16_MSB:
        case Model::StripConfig::StripInputType::RGB16_LSB:
        case Model::StripConfig::StripInputType::RGB16_MSB: {
            return 2;
        } break;
        case Model::StripConfig::StripInputType::RGB8_SRGB:
        case Model::StripConfig::StripInputType::RGB8:
        case Model::StripConfig::StripInputType::RGBW_SRGB:
        case Model::StripConfig::StripInputType::RGBW8: {
            return 1;
        } break;
    }
    return 0;
}

void Strip::setData(const uint8_t *data, const size_t len, const Model::StripConfig::StripInputType input_type) {
    auto transfer = [=, this](const std::vector<int> &order) {
        const size_t input_size = getBytesPerInputPixel(input_type);
        const size_t input_pad = size_t(dmxMaxLen / input_size) * order.size() * getComponentBytes(input_type);
        size_t left = len;
        const uint8_t *ptr = data;
        for (size_t c = 0; c <= ((len - 1) / input_pad); c++) {
            size_t block = std::min(left, input_pad);
            setUniverseData(c, ptr, block, input_type);
            left -= block;
            ptr += block;
        }
    };

    switch (output_type) {
        default:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::WS2816:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::GS8202:
        case Model::StripConfig::StripOutputType::UCS1904: {
            const std::vector<int> order = {1, 0, 2};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::APA107:
        case Model::StripConfig::StripOutputType::APA102:
        case Model::StripConfig::StripOutputType::TM1829: {
            const std::vector<int> order = {2, 1, 0};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::TLS3001: {
            const std::vector<int> order = {0, 1, 2};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::SK6812_RGBW: {
            const std::vector<int> order = {1, 0, 2, 3};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::LPD8806: {
            const std::vector<int> order = {2, 0, 1};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::WS2801: {
            const std::vector<int> order = {1, 0, 2};
            transfer(order);
        } break;
    }
}

__attribute__((hot, optimize("O3"), optimize("unroll-loops"))) void Strip::setUniverseData(const size_t uniN, const uint8_t *data, const size_t len,
                                                                                           const Model::StripConfig::StripInputType input_type) {
    __assume(uniN < Model::universeN);
    __assume(len <= 512);
    __assume(input_type < Model::StripConfig::INPUT_COUNT);

    if (uniN >= Model::universeN) {
        return;
    }

    if (!isUniverseActive(uniN, input_type)) {
        return;
    }

    auto transfer = [=, this](const std::vector<size_t> &order) {
        const uint32_t limit_8bit = uint32_t(std::clamp(comp_limit, 0.0f, 1.0f) * 255.f);
        const uint32_t limit_16bit = uint32_t(std::clamp(comp_limit, 0.0f, 1.0f) * 65535.f);
        const size_t input_size = getBytesPerInputPixel(input_type);
        const size_t pixel_pad = std::min(getComponentsPerInputPixel(input_type), order.size());
        const size_t input_pad = size_t(dmxMaxLen / input_size) * order.size() * getComponentBytes(input_type);
        const size_t pixel_loop_n = std::min(len, input_pad);
        const size_t order_size = order.size();

        __assume(pixel_loop_n > 0);

        __assume(limit_8bit <= 255);
        __assume(limit_16bit <= 65535);

        __assume(input_size > 0);
        __assume(pixel_pad > 0);
        __assume(input_size <= 8);
        __assume(pixel_pad <= 4);
        __assume(input_pad > 0);
        __assume(input_pad < dmxMaxLen);

        auto fix_for_ws2816b = [=](const uint16_t v) {
            static constexpr auto lut = make_ws2816b_error_lut();
            if (v < lut.size()) {
                return lut[v];
            }
            return v;
        };

        switch (input_type) {
            default:
            case Model::StripConfig::StripInputType::RGB8: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += order_size) {
                            for (size_t d = 0; d < pixel_pad; d++) {
                                buf[n + order[d]] = uint8_t(std::min(limit_8bit, uint32_t(data[c + d])));
                            }
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += 4) {
                            uint32_t r = std::min(limit_8bit, uint32_t(data[c + 0]));
                            uint32_t g = std::min(limit_8bit, uint32_t(data[c + 1]));
                            uint32_t b = std::min(limit_8bit, uint32_t(data[c + 2]));
                            uint32_t m = std::min(r, std::min(g, b));
                            buf[n + order[0]] = uint8_t(r - m);
                            buf[n + order[1]] = uint8_t(g - m);
                            buf[n + order[2]] = uint8_t(b - m);
                            buf[n + order[3]] = uint8_t(m);
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    uint32_t v = uint32_t(data[c + i]);
                                    v = (v << 8) | v;
                                    return v;
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                write_buf(0, fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(1)))));
                                write_buf(1, fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(0)))));
                                write_buf(2, fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(2)))));
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    uint32_t v = uint32_t(data[c + i]);
                                    return v;
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                uint32_t r = hd108_lut[0][read_buf(0)];
                                uint32_t g = hd108_lut[1][read_buf(1)];
                                uint32_t b = hd108_lut[2][read_buf(2)];

                                write_buf(0, uint16_t(std::min(limit_16bit, r)));
                                write_buf(1, uint16_t(std::min(limit_16bit, g)));
                                write_buf(2, uint16_t(std::min(limit_16bit, b)));
                            }
                            return;
                        }
                    } break;
                }
            } break;
            case Model::StripConfig::StripInputType::RGBW8: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += 3) {
                            uint32_t r = uint32_t(data[c + 0]);
                            uint32_t g = uint32_t(data[c + 1]);
                            uint32_t b = uint32_t(data[c + 2]);
                            uint32_t w = uint32_t(data[c + 3]);

                            buf[n + order[0]] = uint8_t(std::min(r + w, limit_8bit));
                            buf[n + order[1]] = uint8_t(std::min(g + w, limit_8bit));
                            buf[n + order[2]] = uint8_t(std::min(b + w, limit_8bit));
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += order_size) {
                            for (size_t d = 0; d < pixel_pad; d++) {
                                buf[n + order[d]] = uint8_t(std::min(limit_8bit, uint32_t(data[c + d])));
                            }
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    uint32_t v = uint32_t(data[c + i]);
                                    v = (v << 8) | v;
                                    return v;
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                uint32_t r = read_buf(0);
                                uint32_t g = read_buf(1);
                                uint32_t b = read_buf(2);
                                uint32_t w = read_buf(3);

                                r = fix_for_ws2816b(uint16_t(std::min(limit_16bit, r + w)));
                                g = fix_for_ws2816b(uint16_t(std::min(limit_16bit, g + w)));
                                b = fix_for_ws2816b(uint16_t(std::min(limit_16bit, b + w)));

                                write_buf(0, uint16_t(g));
                                write_buf(1, uint16_t(r));
                                write_buf(2, uint16_t(b));
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    uint32_t v = uint32_t(data[c + i]);
                                    return v;
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                uint32_t r = read_buf(0);
                                uint32_t g = read_buf(1);
                                uint32_t b = read_buf(2);
                                uint32_t w = read_buf(3);

                                r = uint8_t(std::min(limit_8bit, r + w));
                                g = uint8_t(std::min(limit_8bit, g + w));
                                b = uint8_t(std::min(limit_8bit, b + w));

                                r = hd108_lut[0][r];
                                g = hd108_lut[1][g];
                                b = hd108_lut[2][b];

                                write_buf(0, uint16_t(r));
                                write_buf(1, uint16_t(g));
                                write_buf(2, uint16_t(b));
                            }
                            return;
                        }
                    } break;
                }
            } break;
            case Model::StripConfig::StripInputType::RGB8_SRGB: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += 3) {
                            uint8_t sr = data[c + 0];
                            uint8_t sg = data[c + 1];
                            uint8_t sb = data[c + 2];

                            uint16_t lr = 0;
                            uint16_t lg = 0;
                            uint16_t lb = 0;

                            converter.sRGB8toLEDPWM(sr, sg, sb, 255, lr, lg, lb);

                            lr = std::min(uint16_t(limit_8bit), lr);
                            lg = std::min(uint16_t(limit_8bit), lg);
                            lb = std::min(uint16_t(limit_8bit), lb);

                            buf[n + order[0]] = uint8_t(lr);
                            buf[n + order[1]] = uint8_t(lg);
                            buf[n + order[2]] = uint8_t(lb);
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += 3) {
                            uint8_t sr = data[c + 0];
                            uint8_t sg = data[c + 1];
                            uint8_t sb = data[c + 2];

                            uint16_t lr = 0;
                            uint16_t lg = 0;
                            uint16_t lb = 0;

                            converter.sRGB8toLEDPWM(sr, sg, sb, 255, lr, lg, lb);

                            uint16_t lm = std::min(lr, std::min(lg, lb));

                            lr = std::min(uint16_t(limit_8bit), lr);
                            lg = std::min(uint16_t(limit_8bit), lg);
                            lb = std::min(uint16_t(limit_8bit), lb);
                            lm = std::min(uint16_t(limit_8bit), lm);

                            buf[n + order[0]] = uint8_t(lr - lm);
                            buf[n + order[1]] = uint8_t(lg - lm);
                            buf[n + order[2]] = uint8_t(lb - lm);
                            buf[n + order[3]] = uint8_t(lm);
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint16_t *buf = reinterpret_cast<uint16_t *>(uintptr_t(&comp_buf[input_pad * uniN]));
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += 3) {
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(&buf[(n + i) * 2]) = __builtin_bswap16(uint16_t(p));
                                };

                                uint8_t sr = data[c + 0];
                                uint8_t sg = data[c + 1];
                                uint8_t sb = data[c + 2];

                                uint16_t lr = 0;
                                uint16_t lg = 0;
                                uint16_t lb = 0;

                                converter.sRGB8toLEDPWM(sr, sg, sb, 65535, lr, lg, lb);

                                lr = fix_for_ws2816b(std::min(uint16_t(limit_16bit), lr));
                                lg = fix_for_ws2816b(std::min(uint16_t(limit_16bit), lg));
                                lb = fix_for_ws2816b(std::min(uint16_t(limit_16bit), lb));

                                write_buf(0, lg);
                                write_buf(1, lr);
                                write_buf(2, lb);
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint16_t *buf = reinterpret_cast<uint16_t *>(uintptr_t(&comp_buf[input_pad * uniN]));
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 3, n += 3) {
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(&buf[(n + i) * 2]) = __builtin_bswap16(uint16_t(p));
                                };

                                uint8_t sr = data[c + 0];
                                uint8_t sg = data[c + 1];
                                uint8_t sb = data[c + 2];

                                uint16_t lr = 0;
                                uint16_t lg = 0;
                                uint16_t lb = 0;

                                converter.sRGB8toLEDPWM(sr, sg, sb, 65535, lr, lg, lb);

                                // TODO: HD108 lut

                                lr = std::min(uint16_t(limit_16bit), lr);
                                lg = std::min(uint16_t(limit_16bit), lg);
                                lb = std::min(uint16_t(limit_16bit), lb);

                                write_buf(0, lr);
                                write_buf(1, lg);
                                write_buf(2, lb);
                            }
                            return;
                        }
                    } break;
                }
            } break;
            case Model::StripConfig::StripInputType::RGBW_SRGB: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += 3) {
                            uint8_t sr = uint8_t(data[c + 0]);
                            uint8_t sg = uint8_t(data[c + 1]);
                            uint8_t sb = uint8_t(data[c + 2]);
                            uint8_t lw = uint8_t(data[c + 3]);

                            uint16_t lr = 0;
                            uint16_t lg = 0;
                            uint16_t lb = 0;

                            converter.sRGB8toLEDPWM(sr, sg, sb, 255, lr, lg, lb);

                            buf[n + order[0]] = uint8_t(std::min(uint32_t(lr + uint16_t(lw)), limit_8bit));
                            buf[n + order[1]] = uint8_t(std::min(uint32_t(lg + uint16_t(lw)), limit_8bit));
                            buf[n + order[2]] = uint8_t(std::min(uint32_t(lb + uint16_t(lw)), limit_8bit));
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += 4) {
                            uint8_t sr = uint8_t(data[c + 0]);
                            uint8_t sg = uint8_t(data[c + 1]);
                            uint8_t sb = uint8_t(data[c + 2]);
                            uint8_t lw = uint8_t(data[c + 3]);

                            uint16_t lr = 0;
                            uint16_t lg = 0;
                            uint16_t lb = 0;

                            converter.sRGB8toLEDPWM(sr, sg, sb, 255, lr, lg, lb);

                            buf[n + order[0]] = uint8_t(std::min(uint32_t(lr), limit_8bit));
                            buf[n + order[1]] = uint8_t(std::min(uint32_t(lg), limit_8bit));
                            buf[n + order[2]] = uint8_t(std::min(uint32_t(lb), limit_8bit));
                            buf[n + order[3]] = uint8_t(std::min(uint32_t(lw), limit_8bit));
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint16_t *buf = reinterpret_cast<uint16_t *>(uintptr_t(&comp_buf[input_pad * uniN]));
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += 3) {
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(&buf[(n + i) * 2]) = __builtin_bswap16(uint16_t(p));
                                };

                                uint8_t sr = uint8_t(data[c + 0]);
                                uint8_t sg = uint8_t(data[c + 1]);
                                uint8_t sb = uint8_t(data[c + 2]);

                                uint16_t lr = 0;
                                uint16_t lg = 0;
                                uint16_t lb = 0;
                                uint16_t lw = uint16_t(data[c + 3]);

                                converter.sRGB8toLEDPWM(sr, sg, sb, 65535, lr, lg, lb);

                                lw = (lw << 8) | lw;

                                lr = fix_for_ws2816b(uint16_t(std::min(limit_8bit, uint32_t(lr) + uint32_t(lw))));
                                lg = fix_for_ws2816b(uint16_t(std::min(limit_8bit, uint32_t(lg) + uint32_t(lw))));
                                lb = fix_for_ws2816b(uint16_t(std::min(limit_8bit, uint32_t(lb) + uint32_t(lw))));

                                write_buf(0, lg);
                                write_buf(1, lr);
                                write_buf(2, lb);
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint16_t *buf = reinterpret_cast<uint16_t *>(uintptr_t(&comp_buf[input_pad * uniN]));
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 4, n += 3) {
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(&buf[(n + i) * 2]) = __builtin_bswap16(uint16_t(p));
                                };

                                uint8_t sr = uint8_t(data[c + 0]);
                                uint8_t sg = uint8_t(data[c + 1]);
                                uint8_t sb = uint8_t(data[c + 2]);

                                uint16_t lr = 0;
                                uint16_t lg = 0;
                                uint16_t lb = 0;
                                uint16_t lw = uint8_t(data[c + 3]);

                                converter.sRGB8toLEDPWM(sr, sg, sb, 65535, lr, lg, lb);

                                lw = (lw << 8) | lw;

                                // TODO: HD108 lut

                                lr = uint16_t(std::min(limit_8bit, uint32_t(lr) + uint32_t(lw)));
                                lg = uint16_t(std::min(limit_8bit, uint32_t(lg) + uint32_t(lw)));
                                lb = uint16_t(std::min(limit_8bit, uint32_t(lb) + uint32_t(lw)));

                                write_buf(0, lr);
                                write_buf(1, lg);
                                write_buf(2, lb);
                            }
                            return;
                        }
                    } break;
                }
            } break;
            case Model::StripConfig::StripInputType::RGB16_LSB: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += order_size) {
                            for (size_t d = 0; d < pixel_pad; d++) {
                                buf[n + order[d]] = uint8_t(std::min(limit_8bit, uint32_t(data[c + d * 2 + 1])));
                            }
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += 4) {
                            auto read_buf = [=](const size_t i) { return uint32_t(data[c + i * 2 + 1]); };
                            auto write_buf = [=](const size_t i, const uint8_t p) { buf[n + order[i]] = p; };

                            uint32_t r = std::min(limit_8bit, read_buf(0));
                            uint32_t g = std::min(limit_8bit, read_buf(1));
                            uint32_t b = std::min(limit_8bit, read_buf(2));

                            uint32_t m = std::min(r, std::min(g, b));

                            write_buf(0, uint8_t(r - m));
                            write_buf(1, uint8_t(g - m));
                            write_buf(2, uint8_t(b - m));
                            write_buf(3, uint8_t(m));
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += 3) {
                                auto read_buf = [=](const size_t i) { return uint32_t(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))); };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                write_buf(0, uint16_t(fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(1))))));
                                write_buf(1, uint16_t(fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(0))))));
                                write_buf(2, uint16_t(fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(2))))));
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += 3) {
                                auto read_buf = [=](const size_t i) { return uint32_t(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))); };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                write_buf(0, uint16_t(std::min(limit_16bit, read_buf(0))));
                                write_buf(1, uint16_t(std::min(limit_16bit, read_buf(1))));
                                write_buf(2, uint16_t(std::min(limit_16bit, read_buf(2))));
                            }
                            return;
                        }
                    } break;
                }
            } break;
            case Model::StripConfig::StripInputType::RGB16_MSB: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += order_size) {
                            for (size_t d = 0; d < pixel_pad; d++) {
                                buf[n + order[d]] = uint8_t(std::min(limit_8bit, uint32_t(data[c + d * 2 + 0])));
                            }
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += 4) {
                            auto read_buf = [=](const size_t i) { return uint32_t(data[c + i * 2]); };
                            auto write_buf = [=](const size_t i, const uint8_t p) { buf[n + order[i]] = p; };

                            uint32_t r = std::min(limit_8bit, read_buf(0));
                            uint32_t g = std::min(limit_8bit, read_buf(1));
                            uint32_t b = std::min(limit_8bit, read_buf(2));

                            uint32_t m = std::min(r, std::min(g, b));

                            write_buf(0, uint8_t(r - m));
                            write_buf(1, uint8_t(g - m));
                            write_buf(2, uint8_t(b - m));
                            write_buf(3, uint8_t(m));
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    return uint32_t(__builtin_bswap16(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))));
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                write_buf(0, fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(1)))));
                                write_buf(1, fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(0)))));
                                write_buf(2, fix_for_ws2816b(uint16_t(std::min(limit_16bit, read_buf(2)))));
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 6, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    return uint32_t(__builtin_bswap16(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))));
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                write_buf(0, uint16_t(std::min(limit_16bit, read_buf(0))));
                                write_buf(1, uint16_t(std::min(limit_16bit, read_buf(1))));
                                write_buf(2, uint16_t(std::min(limit_16bit, read_buf(2))));
                            }
                            return;
                        }
                    } break;
                }
            } break;
            case Model::StripConfig::StripInputType::RGBW16_LSB: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += 3) {
                            auto read_buf = [=](const size_t i) { return uint32_t(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2 + 1]))); };
                            auto write_buf = [=](const size_t i, const uint8_t p) { buf[n + order[i]] = p; };

                            uint32_t r = read_buf(0);
                            uint32_t g = read_buf(1);
                            uint32_t b = read_buf(2);
                            uint32_t w = read_buf(3);

                            write_buf(0, uint8_t(std::min(limit_8bit, r + w)));
                            write_buf(1, uint8_t(std::min(limit_8bit, g + w)));
                            write_buf(2, uint8_t(std::min(limit_8bit, b + w)));
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += order_size) {
                            for (size_t d = 0; d < pixel_pad; d++) {
                                buf[n + order[d]] = uint8_t(std::min(limit_8bit, uint32_t(data[c + d * 2 + 1])));
                            }
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += 3) {
                                auto read_buf = [=](const size_t i) { return uint32_t(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))); };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                uint32_t r = read_buf(0);
                                uint32_t g = read_buf(1);
                                uint32_t b = read_buf(2);
                                uint32_t w = read_buf(3);

                                write_buf(0, fix_for_ws2816b(uint16_t(std::min(limit_16bit, g + w))));
                                write_buf(1, fix_for_ws2816b(uint16_t(std::min(limit_16bit, r + w))));
                                write_buf(2, fix_for_ws2816b(uint16_t(std::min(limit_16bit, b + w))));
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += 3) {
                                auto read_buf = [=](const size_t i) { return uint32_t(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))); };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                uint32_t r = read_buf(0);
                                uint32_t g = read_buf(1);
                                uint32_t b = read_buf(2);
                                uint32_t w = read_buf(3);

                                write_buf(0, uint16_t(std::min(limit_16bit, r + w)));
                                write_buf(1, uint16_t(std::min(limit_16bit, g + w)));
                                write_buf(2, uint16_t(std::min(limit_16bit, b + w)));
                            }
                            return;
                        }
                    } break;
                }
            } break;
            case Model::StripConfig::StripInputType::RGBW16_MSB: {
                switch (nativeType()) {
                    default: {
                    } break;
                    case NATIVE_RGB8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += 3) {
                            auto read_buf = [=](const size_t i) { return uint32_t(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2 + 0]))); };

                            uint32_t r = read_buf(0);
                            uint32_t g = read_buf(1);
                            uint32_t b = read_buf(2);
                            uint32_t w = read_buf(3);

                            buf[n + order[0]] = uint8_t(std::min(limit_8bit, r + w));
                            buf[n + order[1]] = uint8_t(std::min(limit_8bit, g + w));
                            buf[n + order[2]] = uint8_t(std::min(limit_8bit, b + w));
                        }
                    } break;
                    case NATIVE_RGBW8: {
                        uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                        for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += order_size) {
                            for (size_t d = 0; d < pixel_pad; d++) {
                                buf[n + order[d]] = uint8_t(std::min(limit_8bit, uint32_t(data[c + d * 2 + 0])));
                            }
                        }
                    } break;
                    case NATIVE_RGB16: {
                        if (output_type == Model::StripConfig::StripOutputType::WS2816) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    return uint32_t(__builtin_bswap16(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))));
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                uint32_t r = read_buf(0);
                                uint32_t g = read_buf(1);
                                uint32_t b = read_buf(2);
                                uint32_t w = read_buf(3);

                                r = fix_for_ws2816b(uint16_t(std::min(limit_16bit, r + w)));
                                g = fix_for_ws2816b(uint16_t(std::min(limit_16bit, g + w)));
                                b = fix_for_ws2816b(uint16_t(std::min(limit_16bit, b + w)));

                                write_buf(0, uint16_t(g));
                                write_buf(1, uint16_t(r));
                                write_buf(2, uint16_t(b));
                            }
                            return;
                        }
                        if (output_type == Model::StripConfig::StripOutputType::HD108) {
                            uint8_t *buf = reinterpret_cast<uint8_t *>(&comp_buf[input_pad * uniN]);
                            for (size_t c = 0, n = 0; c < pixel_loop_n; c += 8, n += 3) {
                                auto read_buf = [=](const size_t i) {
                                    return uint32_t(__builtin_bswap16(*reinterpret_cast<const uint16_t *>(uintptr_t(&data[c + i * 2]))));
                                };
                                auto write_buf = [=](const size_t i, const uint16_t p) {
                                    *reinterpret_cast<uint16_t *>(uintptr_t(&buf[(n + i) * 2])) = __builtin_bswap16(uint16_t(p));
                                };

                                uint32_t r = read_buf(0);
                                uint32_t g = read_buf(1);
                                uint32_t b = read_buf(2);
                                uint32_t w = read_buf(3);

                                r = uint16_t(std::min(limit_16bit, r + w));
                                g = uint16_t(std::min(limit_16bit, g + w));
                                b = uint16_t(std::min(limit_16bit, b + w));

                                write_buf(0, uint16_t(g));
                                write_buf(1, uint16_t(r));
                                write_buf(2, uint16_t(b));
                            }
                            return;
                        }
                    } break;
                }
            } break;
        }
    };

    switch (output_type) {
        default:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::WS2816:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::GS8202:
        case Model::StripConfig::StripOutputType::UCS1904: {
            const std::vector<size_t> order = {1, 0, 2};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::APA107:
        case Model::StripConfig::StripOutputType::APA102:
        case Model::StripConfig::StripOutputType::TM1829: {
            const std::vector<size_t> order = {2, 1, 0};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::TLS3001: {
            const std::vector<size_t> order = {0, 1, 2};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::SK6812_RGBW: {
            const std::vector<size_t> order = {1, 0, 2, 3};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::LPD8806: {
            const std::vector<size_t> order = {2, 0, 1};
            transfer(order);
        } break;
        case Model::StripConfig::StripOutputType::WS2801: {
            const std::vector<size_t> order = {1, 0, 2};
            transfer(order);
        } break;
    }
}

void Strip::transfer() {
    size_t len = 0;
    if (Model::instance().burstMode && output_type != Model::StripConfig::StripOutputType::TLS3001) {
        const uint8_t *buf = prepareHead(len);
        if (dmaTransferFunc) {
            dmaTransferFunc((uint8_t *)(buf), uint16_t(len));
        }
        prepareTail();
    } else {
        const uint8_t *buf = prepare(len);
        if (dmaTransferFunc) {
            dmaTransferFunc((uint8_t *)(buf), uint16_t(len));
        }
    }
}

const uint8_t *Strip::prepareHead(size_t &len) {
    switch (output_type) {
        case Model::StripConfig::StripOutputType::TLS3001: {
            return 0;
        } break;
        default:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::SK6812_RGBW:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::WS2816:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::UCS1904:
        case Model::StripConfig::StripOutputType::TM1829:
        case Model::StripConfig::StripOutputType::GS8202: {
            len = std::min(spi_buf.size(), (bytes_len + bytesLatchLen) * 4);
            ws2812_alike_convert(0, std::min(bytes_len + bytesLatchLen, size_t(burstHeadLen)));
            return spi_buf.data();
        } break;
        case Model::StripConfig::StripOutputType::LPD8806: {
            len = std::min(spi_buf.size(), (bytes_len + 1));
            lpd8806_alike_convert(0, std::min(bytes_len + 1, size_t(burstHeadLen)));
            return spi_buf.data();
        } break;
        case Model::StripConfig::StripOutputType::WS2801: {
            len = std::min(spi_buf.size(), bytes_len);
            ws2801_alike_convert(0, std::min(bytes_len, size_t(burstHeadLen)));
            return spi_buf.data();
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::APA107:
        case Model::StripConfig::StripOutputType::APA102: {
            size_t out_len = bytes_len + bytes_len / 3;
            size_t ext_len = 32 + ((bytes_len / 2) + 7) / 8;
            len = std::min(spi_buf.size(), (out_len + ext_len));
            apa102_alike_convert(0, std::min(out_len + ext_len, size_t(burstHeadLen)));
            return spi_buf.data();
        } break;
    }
}

void Strip::prepareTail() {
    switch (output_type) {
        case Model::StripConfig::StripOutputType::TLS3001: {
        } break;
        default:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::SK6812_RGBW:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::WS2816:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::UCS1904:
        case Model::StripConfig::StripOutputType::TM1829:
        case Model::StripConfig::StripOutputType::GS8202: {
            ws2812_alike_convert(std::min(bytes_len + bytesLatchLen, size_t(burstHeadLen)), bytes_len + bytesLatchLen);
        } break;
        case Model::StripConfig::StripOutputType::LPD8806: {
            lpd8806_alike_convert(std::min(bytes_len + 1, size_t(burstHeadLen)), (bytes_len + 1) - 1);
        } break;
        case Model::StripConfig::StripOutputType::WS2801: {
            ws2801_alike_convert(std::min(bytes_len, size_t(burstHeadLen)), bytes_len - 1);
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::APA107:
        case Model::StripConfig::StripOutputType::APA102: {
            size_t out_len = bytes_len + bytes_len / 3;
            size_t ext_len = 32 + ((bytes_len / 2) + 7) / 8;
            apa102_alike_convert(std::min(out_len + ext_len, size_t(burstHeadLen)), (out_len + ext_len) - 1);
        } break;
    }
}

bool Strip::needsClock() const {
    switch (output_type) {
        default:
        case Model::StripConfig::StripOutputType::TLS3001:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::SK6812_RGBW:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::WS2816:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::UCS1904:
        case Model::StripConfig::StripOutputType::TM1829:
        case Model::StripConfig::StripOutputType::GS8202: {
            return false;
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::LPD8806:
        case Model::StripConfig::StripOutputType::WS2801:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::APA107:
        case Model::StripConfig::StripOutputType::APA102: {
            return true;
        } break;
    }
}

const uint8_t *Strip::prepare(size_t &len) {
    switch (output_type) {
        case Model::StripConfig::StripOutputType::TLS3001: {
            tls3001_alike_convert(len);
            return spi_buf.data();
        } break;
        default:
        case Model::StripConfig::StripOutputType::SK6812:
        case Model::StripConfig::StripOutputType::SK6812_RGBW:
        case Model::StripConfig::StripOutputType::WS2812:
        case Model::StripConfig::StripOutputType::WS2816:
        case Model::StripConfig::StripOutputType::TM1804:
        case Model::StripConfig::StripOutputType::UCS1904:
        case Model::StripConfig::StripOutputType::TM1829:
        case Model::StripConfig::StripOutputType::GS8202: {
            len = std::min(spi_buf.size(), (bytes_len + bytesLatchLen) * 4);
            ws2812_alike_convert(0, bytes_len + bytesLatchLen);
            return spi_buf.data();
        } break;
        case Model::StripConfig::StripOutputType::LPD8806: {
            len = std::min(spi_buf.size(), (bytes_len + 3));
            lpd8806_alike_convert(0, (bytes_len + 3) - 1);
            return spi_buf.data();
        } break;
        case Model::StripConfig::StripOutputType::WS2801: {
            len = std::min(spi_buf.size(), (bytes_len + 3));
            ws2801_alike_convert(0, (bytes_len + 3));
            return spi_buf.data();
        } break;
        case Model::StripConfig::StripOutputType::HD108:
        case Model::StripConfig::StripOutputType::SK9822:
        case Model::StripConfig::StripOutputType::HDS107S:
        case Model::StripConfig::StripOutputType::P9813:
        case Model::StripConfig::StripOutputType::APA107:
        case Model::StripConfig::StripOutputType::APA102: {
            size_t out_len = bytes_len + bytes_len / 3;
            size_t ext_len = 32 + ((out_len / 2) + 7) / 8;
            len = std::min(spi_buf.size(), (out_len + ext_len));
            apa102_alike_convert(0, (out_len + ext_len) - 1);
            return spi_buf.data();
        } break;
    }
}

__attribute__((hot, flatten, optimize("O3"))) void Strip::lpd8806_alike_convert(size_t start, size_t end) {
    uint8_t *dst = spi_buf.data() + start;
    *dst++ = 0x00;
    switch (nativeType()) {
        default: {
        } break;
        case NATIVE_RGBW8:
        case NATIVE_RGB8: {
            for (size_t c = std::max(start, size_t(1)); c <= std::min(end, 1 + bytes_len - 1); c++) {
                *dst++ = 0x80 | (comp_buf[c - 1] >> 1);
            }
        } break;
    }
}

__attribute__((hot, flatten, optimize("O3"))) void Strip::ws2801_alike_convert(size_t start, size_t end) {
    uint8_t *dst = spi_buf.data() + start;
    switch (nativeType()) {
        default: {
        } break;
        case NATIVE_RGBW8:
        case NATIVE_RGB8: {
            for (size_t c = start; c <= std::min(end, bytes_len - 1); c++) {
                *dst++ = comp_buf[c];
            }
        } break;
    }
}

__attribute__((hot, flatten, optimize("O3"), optimize("unroll-loops"))) void Strip::apa102_alike_convert(size_t start, size_t end) {
    // Align to 4 bytes
    start &= ~3UL;

    uint8_t *dst = spi_buf.data() + start;
    size_t out_len = bytes_len + (bytes_len / 3);

    // start frame
    size_t head_len = 32;
    for (size_t c = start; c <= std::min(end, size_t(head_len - 1)); c++) {
        *dst++ = 0x00;
    }

    size_t offset = 0;
    // adjust offset
    for (size_t c = head_len; c < start; c += 4, offset += 3) {
    }

    size_t loop_start = std::max(start, size_t(head_len));
    size_t loop_end = std::min(end, head_len + out_len - 1);

    switch (nativeType()) {
        default: {
        } break;
        case NATIVE_RGB16: {
            uint8_t illum5 = uint8_t(float(0x1f) * std::clamp(glob_illum, 0.0f, 1.0f));
            uint16_t illum16 = 0b1000'0000'0000'0000 | (illum5 << 10) | (illum5 << 5) | illum5;
            for (size_t c = loop_start; c <= loop_end; c += 4, offset += 6) {
                *dst++ = uint8_t(illum16 >> 8);
                *dst++ = uint8_t(illum16 & 0xFF);
                *dst++ = comp_buf[offset + 0];
                *dst++ = comp_buf[offset + 1];
                *dst++ = comp_buf[offset + 2];
                *dst++ = comp_buf[offset + 3];
                *dst++ = comp_buf[offset + 4];
                *dst++ = comp_buf[offset + 5];
            }
        } break;
        case NATIVE_RGB8: {
            uint8_t illum = 0b11100000 | uint8_t(float(0x1f) * std::clamp(glob_illum, 0.0f, 1.0f));
            for (size_t c = loop_start; c <= loop_end; c += 4, offset += 3) {
                *dst++ = illum;
                *dst++ = comp_buf[offset + 0];
                *dst++ = comp_buf[offset + 1];
                *dst++ = comp_buf[offset + 2];
            }
        } break;
    }
    // latch words
    for (size_t c = std::max(start, head_len + out_len); c <= end; c++) {
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x00;
    }
}

__attribute__((hot, flatten, optimize("O3"), optimize("unroll-loops"))) void Strip::ws2812_alike_convert(const size_t start, const size_t end) {
    uint32_t *dst = reinterpret_cast<uint32_t *>(uintptr_t(spi_buf.data() + start * 4));
    size_t head_len = bytesLatchLen / 2;
    for (size_t c = start; c < std::min(end, size_t(head_len)); c++) {
        *dst++ = 0x00;
    }

    switch (nativeType()) {
        default: {
        } break;
        case NATIVE_RGB16:
        case NATIVE_RGBW8:
        case NATIVE_RGB8: {
            const uint8_t *src = &comp_buf[std::max(start, size_t(head_len)) - head_len];
            const int32_t len = int32_t(std::min(end, head_len + bytes_len)) - int32_t(std::max(start, size_t(head_len)));
            for (int32_t c = 0; c <= len; c++) {
                dst[c] = ws2812_lut[src[c]];
            }
            dst += len;
        } break;
    }
    for (size_t c = std::max(start, head_len + bytes_len); c <= end; c++) {
        *dst++ = 0x00;
    }
}

__attribute__((hot, flatten, optimize("O3"), optimize("unroll-loops"))) void Strip::tls3001_alike_convert(size_t &len) {
    uint8_t *dst = spi_buf.data();
    uint32_t reset = 0b11111111'11111110'10000000'00000000;  // 19 bits
    uint32_t syncw = 0b11111111'11111110'00100000'00000000;  // 30 bits
    uint32_t start = 0b11111111'11111110'01000000'00000000;  // 19 bits
    manchester_bit_buf buf(dst);
    if (!strip_reset) {
        strip_reset = true;
        buf.push(reset, 19);
        buf.push(0, 4000);
        buf.push(syncw, 30);
        buf.push(0, 12 * int32_t(bytes_len / 3));
    } else {
        buf.push(start, 19);
        switch (nativeType()) {
            default: {
            } break;
            case NATIVE_RGBW8:
            case NATIVE_RGB8: {
                for (size_t c = 0; c < bytes_len; c++) {
                    uint32_t p = uint32_t(comp_buf[c]);
                    buf.push((p << 19) | (p << 11), 13);
                }
            } break;
        }
        buf.push(0, 100);
        buf.push(start, 19);
    }
    buf.flush();

    len = buf.len();
}
