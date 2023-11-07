#include "crc.h"

#include <stdint.h>

#include "nx_crypto.h"
#include "nx_crypto_sha2.h"

// Defined in linker script(s)
extern const uint32_t _crc_section_pre_info_start[];
extern const uint32_t _crc_section_post_info_start[];
extern const uint32_t _crc_section_start[];
extern const uint32_t _crc_section_end[];

void verify_sha256_checksum_self() { verify_sha256_checksum(_crc_section_pre_info_start, _crc_section_post_info_start, _crc_section_start, _crc_section_end); }

void verify_sha256_checksum(const uint32_t *crc_pre_info_start, const uint32_t *crc_post_info_start, const uint32_t *crc_start, const uint32_t *crc_end) {
    // Check for magic header
    if (crc_pre_info_start[0] != 0x1ED51234 || crc_pre_info_start[1] != 0x1ED54321) {
        while (1) {
        };
    }

    // Sanity check that replicated sections pre and post binary are the same
    if (memcmp(crc_pre_info_start, crc_post_info_start, sizeof(uint32_t) * 4) != 0) {
        while (1) {
        };
    }

    nx_crypto_initialize();

    NX_CRYPTO_SHA256 sha256{};
    _nx_crypto_sha256_initialize(&sha256, NX_CRYPTO_HASH_SHA256);

    // Calc sha256 digest of binary, starting from vector table to start of crc section
    const uint8_t *bin_start = (const uint8_t *)crc_pre_info_start[2];
    const uint8_t *bin_end = (const uint8_t *)crc_pre_info_start[3];
    _nx_crypto_sha256_update(&sha256, const_cast<UCHAR *>(bin_start), static_cast<UINT>(bin_end - bin_start));

    UCHAR sha256_digest[32]{};
    _nx_crypto_sha256_digest_calculate(&sha256, sha256_digest, NX_CRYPTO_HASH_SHA256);

    // Sanity check in case linker script breaks
    if (size_t(crc_end - crc_start) != sizeof(sha256_digest)) {
        while (1) {
        };
    }

    if (memcmp(sha256_digest, crc_start, sizeof(sha256_digest)) != 0) {
        while (1) {
        };
    }
}
