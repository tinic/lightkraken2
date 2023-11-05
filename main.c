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
#include "./support/hal_sys_init.h"
#include "stm32h5xx_hal.h"
#include "tx_api.h"

static const uint32_t sha256_binary[8] __attribute__((used)) __attribute__((section(".crc_section")));
extern const void *_section_info_start;

static void check_firmware_sha256() {
#if 0
    const uint32_t *section_info = (const uint32_t *)_section_info_start;

    if (section_info[0] != 0x1ED51ED5) {
        while(1) {};
    }

    const uint8_t *bin_start = (const uint8_t *)section_info[1];
    const uint8_t *bin_end = (const uint8_t *)section_info[2];

    const uint8_t *sha256_start = (const uint8_t *)section_info[2];
    const uint8_t *sha256_end = (const uint8_t *)section_info[3];

    if (sha256_start != (const uint8_t *)(sha256_binary)) {
        while(1) {};
    }

    if (sha256_end != (const uint8_t *)(sha256_binary) + sizeof(sha256_binary)) {
        while(1) {};
    }
#endif  // #if 0
}

int main() {
    check_firmware_sha256();
    SYS_Init();
    tx_kernel_enter();
    while (1) {
        __WFI();
    }
}
