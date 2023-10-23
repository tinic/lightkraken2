
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
#include <memory.h>
#include <stdint.h>

#include "fal.h"
#include "fal_cfg.h"
#include "stm32h5xx_hal.h"

static int init(void) { return 0; }

static int read(long offset, uint8_t *buf, size_t size) {
    if (offset % 4 != 0) {
        while (1) {
        }
    }

    memcpy(buf, (void *)(nor_flash0.addr + offset), size);

    return size;
}

static int write(long offset, const uint8_t *buf, size_t size) {
    if (HAL_ICACHE_Disable() != 0) {
        while (1) {
        }
    }

    HAL_FLASH_Unlock();

    uintptr_t addr = nor_flash0.addr + offset;
    if (addr % 16 != 0) {
        while (1) {
        }
    }

    if (size % 16 != 0) {
        while (1) {
        }
    }

    uint32_t data[4];
    for (size_t i = 0; i < size; i += sizeof(data)) {
        memcpy(&data, &buf[i], sizeof(data));
        if (data[0] != 0xFFFFFFFF || data[1] != 0xFFFFFFFF || data[2] != 0xFFFFFFFF || data[3] != 0xFFFFFFFF) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, addr + i, ((uintptr_t)&data[0])) != 0) {
                while (1) {
                }
            }
        }
    }

    HAL_FLASH_Lock();

    if (HAL_ICACHE_Enable() != 0) {
        while (1) {
        }
    }

    const uint8_t *wrt = (const uint8_t *)buf;
    const uint8_t *org = (const uint8_t *)addr;
    if (memcmp(wrt, org, size) != 0) {
        while (1) {
        }
    }

    return size;
}

static uint32_t GetSector(uint32_t Address) {
    uint32_t sector = 0;

    if ((Address >= FLASH_BASE) && (Address < FLASH_BASE + FLASH_BANK_SIZE)) {
        sector = (Address & ~FLASH_BASE) / FLASH_SECTOR_SIZE;
    } else if ((Address >= FLASH_BASE + FLASH_BANK_SIZE) && (Address < FLASH_BASE + FLASH_SIZE)) {
        sector = ((Address & ~FLASH_BASE) - FLASH_BANK_SIZE) / FLASH_SECTOR_SIZE;
    } else {
        sector = 0xFFFFFFFF; /* Address out of range */
    }

    return sector;
}

static uint32_t GetBank(uint32_t Addr) {
    uint32_t bank = 0;
    if (READ_BIT(FLASH->OPTSR_CUR, FLASH_OPTSR_SWAP_BANK) == 0) {
        /* No Bank swap */
        if (Addr < (FLASH_BASE + FLASH_BANK_SIZE)) {
            bank = FLASH_BANK_1;
        } else {
            bank = FLASH_BANK_2;
        }
    } else {
        /* Bank swap */
        if (Addr < (FLASH_BASE + FLASH_BANK_SIZE)) {
            bank = FLASH_BANK_2;
        } else {
            bank = FLASH_BANK_1;
        }
    }

    return bank;
}

static int erase(long offset, size_t size) {
    uint32_t addr = nor_flash0.addr + offset;
    if (addr % FLASH_DB_BLOCK_SIZE != 0) {
        while (1) {
        }
    }

    if (HAL_ICACHE_Disable() != 0) {
        while (1) {
        }
    }

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef eraseInitStruct = {0};
    eraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInitStruct.Banks = GetBank(addr);
    eraseInitStruct.Sector = GetSector(addr);
    eraseInitStruct.NbSectors = size / FLASH_DB_BLOCK_SIZE;

    uint32_t sectorError;
    if (HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError) != 0) {
        while (1) {
        }
    }

    HAL_FLASH_Lock();

    if (HAL_ICACHE_Enable() != 0) {
        while (1) {
        }
    }

    const uint32_t *empty = (const uint32_t *)addr;
    for (size_t c = 0; c < size / sizeof(uint32_t); c++) {
        if (empty[c] != 0xFFFFFFFF) {
            while (1) {
            }
        }
    }

    return size;
}

const struct fal_flash_dev nor_flash0 = {.name = "norflash0",
                                         .addr = FLASH_DB_START_ADDRESS,
                                         .len = FLASH_DB_LENGTH,
                                         .blk_size = FLASH_DB_BLOCK_SIZE,
                                         .ops = {init, read, write, erase},
                                         .write_gran = FLASH_DB_WRITE_GRAN};
