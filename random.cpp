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

#include "./random.h"

#include <stdint.h>

#include "stm32h5xx_hal.h"

PseudoRandom &PseudoRandom::instance() {
    static PseudoRandom pseudoRandom;
    if (!pseudoRandom.initialized) {
        pseudoRandom.initialized = true;
        pseudoRandom.init();
    }
    return pseudoRandom;
}

void PseudoRandom::set_seed(uint32_t seed) {
    uint32_t i;
    a = 0xf1ea5eed, b = c = d = seed;
    for (i = 0; i < 20; ++i) {
        (void)get();
    }
}

#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
uint32_t PseudoRandom::get() {
    uint32_t e = a - rot(b, 27);
    a = b ^ rot(c, 17);
    b = c + d;
    c = d + e;
    d = e + a;
    return d;
}

void PseudoRandom::init() {
    if (HAL_ICACHE_Disable() != 0) {
        while (1) {
        }
    }

    set_seed(HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2());

    if (HAL_ICACHE_Enable() != 0) {
        while (1) {
        }
    }
}
