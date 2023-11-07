#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // #ifdef __cpluplus

void verify_sha256_checksum_self();

void verify_sha256_checksum(const uint32_t *crc_pre_info_start, const uint32_t *crc_post_info_start, const uint32_t *crc_start, const uint32_t *crc_end);

#ifdef __cplusplus
}
#endif  // #ifdef __cpluplus

#endif  // #ifndef _CRC_H_
