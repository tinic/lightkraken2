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
#include <stdint.h>
#include <stdlib.h>

#include "./support/hal_sys_init.h"
#include "nx_crypto.h"
#include "nx_crypto_sha2.h"
#include "stm32h5xx_hal.h"
#include "tx_api.h"

static const uint32_t sha256_binary[8] __attribute__((used)) __attribute__((section(".crc_section")));
extern const uint32_t _crc_section_pre_info_start[];
extern const uint32_t _crc_section_post_info_start[];

static void check_firmware_sha256() {
    nx_crypto_initialize();

    // Check for magic header
    if (_crc_section_pre_info_start[0] != 0x1ED51ED5) {
        while (1) {
        };
    }

    // Sanity check that replicated sections pre and post binary are the same
    if (memcmp(_crc_section_pre_info_start, _crc_section_post_info_start, sizeof(uint32_t) *4 ) != 0) {
        while (1) {
        };
    }

    const uint8_t *sha256_start = (const uint8_t *)_crc_section_pre_info_start[2];
    const uint8_t *sha256_end = (const uint8_t *)_crc_section_pre_info_start[3];

    // Check that we were not moved somehow
    if (sha256_start != (const uint8_t *)(sha256_binary)) {
        while (1) {
        };
    }

    // Sanity check size
    if (sha256_end != (const uint8_t *)(sha256_binary) + sizeof(sha256_binary)) {
        while (1) {
        };
    }

    NX_CRYPTO_SHA256 sha256 = {0};
    _nx_crypto_sha256_initialize(&sha256, NX_CRYPTO_HASH_SHA256);

    // Calc sha256 digest of binary, starting from vector table to start of crc section
    const uint8_t *bin_start = (const uint8_t *)_crc_section_pre_info_start[1];
    const uint8_t *bin_end = (const uint8_t *)_crc_section_pre_info_start[2];
    _nx_crypto_sha256_update(&sha256, (UCHAR *)bin_start, bin_end - bin_start);

    UCHAR sha256_digest[32] = {0};
    _nx_crypto_sha256_digest_calculate(&sha256, sha256_digest, NX_CRYPTO_HASH_SHA256);

    if (memcmp(sha256_digest, sha256_binary, sizeof(sha256_binary)) != 0) {
        while (1) {
        };
    }
}

int main() {
    check_firmware_sha256();
    SYS_Init();
    tx_kernel_enter();
    while (1) {
        __WFI();
    }
}
