#include "crc.h"

#include <stdint.h>

#include "nx_crypto.h"
#include "nx_crypto_sha2.h"

extern const uint32_t _crc_section_pre_info_start[];
extern const uint32_t _crc_section_post_info_start[];
extern const uint32_t _crc_section_start[];
extern const uint32_t _crc_section_end[];

void verify_sha256_checksum() {
    nx_crypto_initialize();

    // Check for magic header
    if (_crc_section_pre_info_start[0] != 0x1ED51234 || _crc_section_pre_info_start[1] != 0x1ED54321) {
        while (1) {
        };
    }

    // Sanity check that replicated sections pre and post binary are the same
    if (memcmp(_crc_section_pre_info_start, _crc_section_post_info_start, sizeof(uint32_t) * 4) != 0) {
        while (1) {
        };
    }

    NX_CRYPTO_SHA256 sha256{};
    _nx_crypto_sha256_initialize(&sha256, NX_CRYPTO_HASH_SHA256);

    // Calc sha256 digest of binary, starting from vector table to start of crc section
    const uint8_t *bin_start = (const uint8_t *)_crc_section_pre_info_start[2];
    const uint8_t *bin_end = (const uint8_t *)_crc_section_pre_info_start[3];
    _nx_crypto_sha256_update(&sha256, const_cast<UCHAR *>(bin_start), static_cast<UINT>(bin_end - bin_start));

    UCHAR sha256_digest[32]{};
    _nx_crypto_sha256_digest_calculate(&sha256, sha256_digest, NX_CRYPTO_HASH_SHA256);

    if (memcmp(sha256_digest, _crc_section_start, sizeof(sha256_digest)) != 0) {
        while (1) {
        };
    }
}
