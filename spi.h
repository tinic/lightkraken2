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
#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>
#include <stdlib.h>

class SPI {
   public:
    void transfer(const uint8_t *buf, size_t len, uint32_t transferMbps, bool wantsSCLK) {
        if (dmaActive) {
            if (isDMAbusy()) {
                scheduleDMA = true;
                return;
            }
        }

        dmaActive = false;
        if (cbuf != buf || clen != len || mbps != transferMbps || wantsSCLK != sclk) {
            cbuf = buf;
            clen = len;
            mbps = transferMbps;
            PLLQCalcMulDiv();
            sclk = wantsSCLK;
            setupDMATransfer();
        }
        startDMATransfer();
        dmaActive = true;
    };

    void update() {
        if (scheduleDMA) {
            scheduleDMA = false;
            transfer(cbuf, clen, mbps, sclk);
        }
    }

    void setDMAActive(bool state) { dmaActive = false; }

    virtual bool isDMAbusy() const = 0;

   protected:

    void PLLQCalcMulDiv() {
        uint32_t min_diff = 0x7FFFFFFF;
        uint32_t base_freq = HSE_VALUE;
        mul = 1;
        div = 1;
        for (uint32_t c = 4; c < 512; c++) {      // PLL2N
            for (uint32_t d = 1; d < 128; d++) {  // PLL2P
                uint32_t calc_mbps = ((base_freq * c) / d) / 2;
                uint32_t diff = uint32_t(std::abs(int(calc_mbps - mbps)));
                if (diff < min_diff) {
                    min_diff = diff;
                    mul = c;
                    div = d;
                }
            }
        }
        actual_mbps = ( ( base_freq * mul ) / div ) / 2;
    }

    size_t clen = 0;
    const uint8_t *cbuf = 0;

    bool scheduleDMA = false;
    bool dmaActive = false;
    bool sclk = false;
    bool fast = true;

    uint32_t mul = 1;
    uint32_t div = 1;
    uint32_t mbps = 40000000;
    uint32_t actual_mbps = 40000000;

    virtual ~SPI(){};
    bool initialized = false;
    virtual void startDMATransfer() = 0;
    virtual void setupDMATransfer() = 0;
    virtual void init() = 0;
};

class SPI_0 : public SPI {
   public:
    static SPI &instance();
    virtual bool isDMAbusy() const override;
    virtual void startDMATransfer() override;
    virtual void setupDMATransfer() override;

   protected:
    virtual ~SPI_0(){};
    virtual void init() override;
};

class SPI_1 : public SPI {
   public:
    static SPI &instance();
    virtual bool isDMAbusy() const override;
    virtual void startDMATransfer() override;
    virtual void setupDMATransfer() override;

   protected:
    virtual ~SPI_1(){};
    virtual void init() override;
};

#endif  // #ifndef _SPI_H
