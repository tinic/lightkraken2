#
# MIT License
#
# Copyright (c) 2023 Tinic Uro
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
cmake_minimum_required(VERSION 3.0...3.16)

# M46x M467 M467SJHAN
project(lightkraken2 C CXX ASM)

set(BASE_ADDRESS 0x08000000)

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

set(THREADX_ARCH cortex_m33)
set(THREADX_TOOLCHAIN gnu)
set(FILEX_CUSTOM_PORT ${PROJECT_SOURCE_DIR}/filex/ports/cortex_m4/gnu)

add_compile_definitions(
    TICK_INT_PRIORITY=8
    TX_TIMER_TICKS_PER_SECOND=1000UL
    TX_MAX_PRIORITIES=32
    TX_DISABLE_PREEMPTION_THRESHOLD
    TX_DISABLE_NOTIFY_CALLBACKS
    TX_DISABLE_REDUNDANT_CLEARING
    TX_NOT_INTERRUPTABLE
    TX_TIMER_PROCESS_IN_ISR
    TX_REACTIVATE_INLINE
    TX_INLINE_THREAD_RESUME_SUSPEND
    TX_SINGLE_MODE_NON_SECURE=1
    __FPU_PRESENT=1U
    ARM_MATH_CM33
    STM32H563xx
    USE_FULL_LL_DRIVER
    USE_HAL_DRIVER
    HSE_VALUE=8000000
    HSI_VALUE=64000000
    LSI_VALUE=32000
    VDD_VALUE=3300
    PREFETCH_ENABLE=1
    HSE_STARTUP_TIMEOUT=100
    LSE_STARTUP_TIMEOUT=5000
    EXTERNAL_CLOCK_VALUE=12288000
    $<$<NOT:$<CONFIG:Release>>:TX_ENABLE_STACK_CHECKING>
    $<$<CONFIG:Release>:TX_DISABLE_STACK_FILLING>
    $<$<CONFIG:Release>:TX_DISABLE_ERROR_CHECKING>
    $<$<CONFIG:Release>:NDEBUG>)

set(STM32HAL_DIR ${PROJECT_SOURCE_DIR}/stm32h5xx_hal_driver)
set(THREADX_DIR ${PROJECT_SOURCE_DIR}/threadx)
set(FILEX_DIR ${PROJECT_SOURCE_DIR}/filex)
set(NETXDUO_DIR ${PROJECT_SOURCE_DIR}/netxduo)
set(LWJSON_DIR ${PROJECT_SOURCE_DIR}/lwjson)
set(FLASHDB_DIR ${PROJECT_SOURCE_DIR}/FlashDB)
set(EMIO_DIR ${PROJECT_SOURCE_DIR}/emio)

set(LD_SCRIPT_RUNTIME ${PROJECT_SOURCE_DIR}/support/STM32H563xx_FLASH.ld)
set(LD_SCRIPT_BOOTLOADER ${PROJECT_SOURCE_DIR}/support/STM32H563xx_FLASH_BOOTLOADER.ld)

set(CC_COMMON_FLAGS
    -mcpu=cortex-m33
    -Wformat=2
    -Wformat-overflow
    -Wformat-truncation
    -Wnull-dereference
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -fdata-sections
    -ffunction-sections
    -Wall
    -Wextra
    -Wpedantic 
    -Wfloat-conversion 
    -Wdouble-promotion
    -Wno-unused-parameter
    -Wno-strict-aliasing 
    -Wno-format
    -Wstack-usage=8192
    -Wuninitialized
    -Wno-psabi
    -funsigned-char
    -fshort-enums
    -fno-common
    -fstack-usage
    -ffast-math
    -ffat-lto-objects
    -fallow-store-data-races)

set(CXX_COMMON_FLAGS 
    ${CC_COMMON_FLAGS}
    -Weffc++
    -Wsign-conversion
    -Wshadow 
    -Wnon-virtual-dtor
    -Wunused
    -Woverloaded-virtual
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wnull-dereference
    -Wstrict-aliasing 
    -Wcast-align
    -Wconversion
    -Wno-volatile
    -std=c++20
    -fno-rtti 
    -fno-exceptions)

set(CC_DEBUG_FLAGS
    -O0 -g3 -gdwarf-4)

set(CC_RELEASE_FLAGS   
    -flto=auto
    -Os)

set(LD_COMMON_FLAGS
    -u _printf_float
    -specs=nano.specs
    -mcpu=cortex-m33
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
    -lm
    -lnosys
    -Wl,--gc-sections)

set(LD_DEBUG_FLAGS)

set(LD_RELEASE_FLAGS   
    -flto=auto)

set(TARGET_NAME_RUNTIME ${PROJECT_NAME}.elf)
set(HEX_FILE_RUNTIME ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE_RUNTIME ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
set(DUMP_FILE_RUNTIME ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.S)
set(LD_RUNTIME_FLAGS
    -T${LD_SCRIPT_RUNTIME}
    -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map,--cref)

set(TARGET_NAME_BOOTLOADER ${PROJECT_NAME}_bootloader.elf)
set(HEX_FILE_BOOTLOADER ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_bootloader.hex)
set(BIN_FILE_BOOTLOADER ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_bootloader.bin)
set(DUMP_FILE_BOOTLOADER ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_bootloader.S)
set(LD_BOOTLOADER_FLAGS
    -T${LD_SCRIPT_BOOTLOADER}
    -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}_bootloader.map,--cref)

# Common include directories
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/support)
include_directories(${PROJECT_SOURCE_DIR}/support/emac)
include_directories(${STM32HAL_DIR}/Inc)
include_directories(${LWJSON_DIR}/lwjson/src/include)
include_directories(${FLASHDB_DIR}/inc)
include_directories(${FLASHDB_DIR}/port/fal/inc)

set(COMMON_SOURCES
    ${PROJECT_SOURCE_DIR}/main.c
    ${PROJECT_SOURCE_DIR}/app.cpp
    ${PROJECT_SOURCE_DIR}/network.cpp
    ${PROJECT_SOURCE_DIR}/settingsdb.cpp
    ${PROJECT_SOURCE_DIR}/webserver.cpp
    ${PROJECT_SOURCE_DIR}/support/fal_stm32h5xx.c
    ${PROJECT_SOURCE_DIR}/support/lan8742.c
    ${PROJECT_SOURCE_DIR}/support/hal_sys_init.c
    ${PROJECT_SOURCE_DIR}/support/nx_stm32_eth_driver.c
    ${PROJECT_SOURCE_DIR}/support/nx_stm32_phy_driver.c
    ${PROJECT_SOURCE_DIR}/support/startup_stm32h563xx.S
    ${PROJECT_SOURCE_DIR}/support/syscalls.c
    ${PROJECT_SOURCE_DIR}/support/sysmem.c
    ${PROJECT_SOURCE_DIR}/support/system_stm32h5xx.c
    ${PROJECT_SOURCE_DIR}/support/tx_initialize_low_level.S
    ${LWJSON_DIR}/lwjson/src/lwjson/lwjson.c
    ${LWJSON_DIR}/lwjson/src/lwjson/lwjson_stream.c
    ${LWJSON_DIR}/lwjson/src/lwjson/lwjson_debug.c
    ${FLASHDB_DIR}/src/fdb_kvdb.c
    ${FLASHDB_DIR}/src/fdb_utils.c
    ${FLASHDB_DIR}/src/fdb.c
    ${FLASHDB_DIR}/port/fal/src/fal_rtt.c
    ${FLASHDB_DIR}/port/fal/src/fal.c
    ${FLASHDB_DIR}/port/fal/src/fal_partition.c
    ${FLASHDB_DIR}/port/fal/src/fal_flash.c)

add_executable(${TARGET_NAME_RUNTIME} ${COMMON_SOURCES})
add_executable(${TARGET_NAME_BOOTLOADER} ${COMMON_SOURCES})

# Generate mkfat tool locally. This is fragile...
IF (${CMAKE_HOST_SYSTEM_NAME} STREQUAL  "Windows")
    set(MKFAT_BINARY ${PROJECT_SOURCE_DIR}/mkfat/build/Debug/mkfat.exe)
    set(MKFAT_CMAKE_COMMAND cmake -Wno-deprecated -G "Visual Studio 16 2019" .. )
ELSE (${CMAKE_HOST_SYSTEM_NAME} STREQUAL  "Windows")
    set(MKFAT_BINARY ${PROJECT_SOURCE_DIR}/mkfat/build/mkfat)
    set(MKFAT_CMAKE_COMMAND cmake -Wno-deprecated -G "Ninja" .. )
ENDIF (${CMAKE_HOST_SYSTEM_NAME} STREQUAL  "Windows")

file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/mkfat/build)
add_custom_command(
    OUTPUT ${MKFAT_BINARY}
    COMMAND ${MKFAT_CMAKE_COMMAND}
    COMMAND cmake --build . 
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/mkfat/build
    DEPENDS ${PROJECT_SOURCE_DIR}/mkfat/main.c)
# Done generating mkfat tool...

# Generate fs.h
file(GLOB VUE3_FILES
       ${PROJECT_SOURCE_DIR}/vue3/src/*.*)
add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/fs.h
    COMMAND yarn build 
    COMMAND ${MKFAT_BINARY} ${PROJECT_SOURCE_DIR}/vue3/dist 512 128 ${PROJECT_BINARY_DIR}/fs.h ${PROJECT_BINARY_DIR}/fs.img
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/vue3
    DEPENDS ${VUE3_FILES} ${MKFAT_BINARY})
target_sources(${TARGET_NAME_RUNTIME} PRIVATE fs.h)

# Generate fsbl.h
file(GLOB FSBL_FILES
       ${PROJECT_SOURCE_DIR}/fsbl/*.*)
add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/fsbl.h
    COMMAND ${MKFAT_BINARY} ${PROJECT_SOURCE_DIR}/fsbl 64 64 ${PROJECT_BINARY_DIR}/fsbl.h ${PROJECT_BINARY_DIR}/fsbl.img
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/fsbl
    DEPENDS ${FSBL_FILES} ${MKFAT_BINARY})
target_sources(${TARGET_NAME_BOOTLOADER} PRIVATE fsbl.h)

# STM32HAL
add_library(stm32hal)
target_sources(stm32hal PRIVATE
    ${STM32HAL_DIR}/Src/stm32h5xx_hal.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_adc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_adc_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_cec.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_comp.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_cordic.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_cortex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_crc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_crc_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_cryp.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_cryp_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_dac.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_dac_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_dcache.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_dcmi.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_dma.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_dma_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_dts.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_eth.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_eth_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_exti.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_fdcan.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_flash.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_flash_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_fmac.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_gpio.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_gtzc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_hash.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_hcd.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_i2c.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_i2c_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_i3c.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_icache.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_irda.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_iwdg.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_lptim.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_mmc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_mmc_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_nand.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_nor.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_opamp.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_opamp_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_otfdec.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_pcd.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_pcd_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_pka.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_pssi.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_pwr.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_pwr_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_ramcfg.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_rcc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_rcc_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_rng.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_rng_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_rtc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_rtc_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_sai.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_sai_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_sd.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_sd_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_sdram.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_smartcard.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_smartcard_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_smbus.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_smbus_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_spi.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_spi_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_sram.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_tim.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_tim_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_uart.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_uart_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_usart.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_usart_ex.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_wwdg.c
    ${STM32HAL_DIR}/Src/stm32h5xx_hal_xspi.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_adc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_comp.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_cordic.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_crc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_crs.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_dac.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_dlyb.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_dma.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_exti.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_fmac.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_fmc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_gpio.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_i2c.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_i3c.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_icache.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_lptim.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_lpuart.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_opamp.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_pka.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_pwr.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_rcc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_rng.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_rtc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_sdmmc.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_spi.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_tim.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_ucpd.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_usart.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_usb.c
    ${STM32HAL_DIR}/Src/stm32h5xx_ll_utils.c)
target_link_libraries(${TARGET_NAME_RUNTIME} stm32hal)
target_link_libraries(${TARGET_NAME_BOOTLOADER} stm32hal)
target_compile_options(stm32hal PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:${CC_COMMON_FLAGS}>")
target_compile_options(stm32hal PRIVATE "$<$<COMPILE_LANGUAGE:C>:${CC_COMMON_FLAGS}>")
target_compile_options(stm32hal PRIVATE "$<$<CONFIG:DEBUG>:${CC_DEBUG_FLAGS}>")
target_compile_options(stm32hal PRIVATE "$<$<CONFIG:RELEASE>:${CC_RELEASE_FLAGS}>")

# emio
add_subdirectory(${EMIO_DIR})
target_link_libraries(${TARGET_NAME_RUNTIME} emio::emio)
target_link_libraries(${TARGET_NAME_BOOTLOADER} emio::emio)

# ThreadX
add_subdirectory(${THREADX_DIR})
target_link_libraries(${TARGET_NAME_RUNTIME} threadx)
target_link_libraries(${TARGET_NAME_BOOTLOADER} threadx)

target_compile_options(threadx PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:${CC_COMMON_FLAGS}>")
target_compile_options(threadx PRIVATE "$<$<COMPILE_LANGUAGE:C>:${CC_COMMON_FLAGS}>")
target_compile_options(threadx PRIVATE "$<$<CONFIG:DEBUG>:${CC_DEBUG_FLAGS}>")
target_compile_options(threadx PRIVATE "$<$<CONFIG:RELEASE>:${CC_RELEASE_FLAGS}>")

# FileX
add_subdirectory(${FILEX_DIR})
target_link_libraries(${TARGET_NAME_RUNTIME} filex)
target_link_libraries(${TARGET_NAME_BOOTLOADER} filex)

target_compile_options(filex PRIVATE "$<$<COMPILE_LANGUAGE:C>:${CC_COMMON_FLAGS}>")
target_compile_options(filex PRIVATE "$<$<CONFIG:DEBUG>:${CC_DEBUG_FLAGS}>")
target_compile_options(filex PRIVATE "$<$<CONFIG:RELEASE>:${CC_RELEASE_FLAGS}>")

# NetXDuo
add_subdirectory(${NETXDUO_DIR})
target_link_libraries(${TARGET_NAME_RUNTIME} netxduo)
target_link_libraries(${TARGET_NAME_BOOTLOADER} netxduo)

target_compile_options(netxduo PRIVATE "$<$<COMPILE_LANGUAGE:C>:${CC_COMMON_FLAGS}>")
target_compile_options(netxduo PRIVATE "$<$<CONFIG:DEBUG>:${CC_DEBUG_FLAGS}>")
target_compile_options(netxduo PRIVATE "$<$<CONFIG:RELEASE>:${CC_RELEASE_FLAGS}>")

# Runtime
target_include_directories(${TARGET_NAME_RUNTIME} PRIVATE ${CMAKE_BINARY_DIR})

target_compile_options(${TARGET_NAME_RUNTIME} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:${CC_COMMON_FLAGS}>")
target_compile_options(${TARGET_NAME_RUNTIME} PRIVATE "$<$<COMPILE_LANGUAGE:C>:${CC_COMMON_FLAGS}>")
target_compile_options(${TARGET_NAME_RUNTIME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CXX_COMMON_FLAGS}>")
target_compile_options(${TARGET_NAME_RUNTIME} PRIVATE "$<$<CONFIG:DEBUG>:${CC_DEBUG_FLAGS}>")
target_compile_options(${TARGET_NAME_RUNTIME} PRIVATE "$<$<CONFIG:RELEASE>:${CC_RELEASE_FLAGS}>")

target_link_options(${TARGET_NAME_RUNTIME} PRIVATE ${LD_COMMON_FLAGS})
target_link_options(${TARGET_NAME_RUNTIME} PRIVATE ${LD_RUNTIME_FLAGS})
target_link_options(${TARGET_NAME_RUNTIME} PRIVATE "$<$<CONFIG:DEBUG>:${LD_DEBUG_FLAGS}>")
target_link_options(${TARGET_NAME_RUNTIME} PRIVATE "$<$<CONFIG:RELEASE>:${LD_RELEASE_FLAGS}>")

# Bootloader
target_include_directories(${TARGET_NAME_BOOTLOADER} PRIVATE ${CMAKE_BINARY_DIR})
target_compile_definitions(${TARGET_NAME_BOOTLOADER} PRIVATE BOOTLOADER)

target_compile_options(${TARGET_NAME_BOOTLOADER} PRIVATE "$<$<COMPILE_LANGUAGE:ASM>:${CC_COMMON_FLAGS}>")
target_compile_options(${TARGET_NAME_BOOTLOADER} PRIVATE "$<$<COMPILE_LANGUAGE:C>:${CC_COMMON_FLAGS}>")
target_compile_options(${TARGET_NAME_BOOTLOADER} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CXX_COMMON_FLAGS}>")
target_compile_options(${TARGET_NAME_BOOTLOADER} PRIVATE "$<$<CONFIG:DEBUG>:${CC_DEBUG_FLAGS}>")
target_compile_options(${TARGET_NAME_BOOTLOADER} PRIVATE "$<$<CONFIG:RELEASE>:${CC_RELEASE_FLAGS}>")

target_link_options(${TARGET_NAME_BOOTLOADER} PRIVATE ${LD_COMMON_FLAGS})
target_link_options(${TARGET_NAME_BOOTLOADER} PRIVATE ${LD_BOOTLOADER_FLAGS})
target_link_options(${TARGET_NAME_BOOTLOADER} PRIVATE "$<$<CONFIG:DEBUG>:${LD_DEBUG_FLAGS}>")
target_link_options(${TARGET_NAME_BOOTLOADER} PRIVATE "$<$<CONFIG:RELEASE>:${LD_RELEASE_FLAGS}>")

# Generate version.h
find_package(Git)

execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_SHORT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_REV_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} show -s --format=%ad HEAD --date=iso-strict WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE GIT_COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${CMAKE_BINARY_DIR}/version.h" @ONLY)

add_custom_command(TARGET ${TARGET_NAME_RUNTIME} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -D $<TARGET_FILE:${TARGET_NAME_RUNTIME}> > ${DUMP_FILE_RUNTIME}
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME_RUNTIME}> ${BIN_FILE_RUNTIME}
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature $<TARGET_FILE:${TARGET_NAME_RUNTIME}> ${HEX_FILE_RUNTIME}
    COMMAND ${CMAKE_SIZE} ${TARGET_NAME_RUNTIME}
    COMMENT ("Building ${HEX_FILE_RUNTIME}\\nBuilding ${BIN_FILE_RUNTIME}"))

add_custom_command(TARGET ${TARGET_NAME_BOOTLOADER} POST_BUILD
    COMMAND ${CMAKE_OBJDUMP} -D $<TARGET_FILE:${TARGET_NAME_BOOTLOADER}> > ${DUMP_FILE_BOOTLOADER}
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME_BOOTLOADER}> ${BIN_FILE_BOOTLOADER}
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature $<TARGET_FILE:${TARGET_NAME_BOOTLOADER}> ${HEX_FILE_BOOTLOADER}
    COMMAND ${CMAKE_SIZE} ${TARGET_NAME_BOOTLOADER}
    COMMENT ("Building ${HEX_FILE_BOOTLOADER}\\nBuilding ${BIN_FILE_BOOTLOADER}"))

set(PROGRAM_CMD "./openocd -f ./stlink-dap.cfg -f ./stm32h5x.cfg -c \"adapter speed 8000\" -c \"program ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin ${BASE_ADDRESS} verify reset exit\"")
install(CODE "execute_process(COMMAND ${PROGRAM_CMD} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/openocd)")
