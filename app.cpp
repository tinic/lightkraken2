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
#include "app.h"

#include <stdio.h>

#ifndef BOOTLOADER
#include <emio/format.hpp>
#endif  // #ifndef BOOTLOADER

#include "./model.h"
#include "./network.h"
#include "./settingsdb.h"
#include "./utils.h"
#include "./webserver.h"
#include "./systick.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_ll_utils.h"

static TX_THREAD thread_startup{};
extern "C" void thread_startup_entry(ULONG thread_input);
void thread_startup_entry(ULONG thread_input) {
    NX_PARAMETER_NOT_USED(thread_input);
    App::instance().start();
    tx_thread_relinquish();
}

extern "C" void tx_application_define(void *first_unused_memory);
void tx_application_define(void *first_unused_memory) {
    App::instance().setup(first_unused_memory);
}

App &App::instance() {
    static App app;
    if (!app.initialized) {
        app.initialized = true;
        app.init();
    }
    return app;
}

void App::start() {

#ifndef BOOTLOADER
    if (HAL_ICACHE_Disable() != 0) {
        while (1) {
        }
    }

    uint32_t uid[3];
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();

    uint32_t flashSize = LL_GetFlashSize();
    uint32_t packageType = LL_GetPackageType();

    if (HAL_ICACHE_Enable() != 0) {
        while (1) {
        }
    }

    Model::instance().exportToDB();
    Model::instance().exportStaticsToDB();

    emio::static_buffer<64> uID{};
    emio::format_to(uID, "{:08x}:{:08x}:{:08x}", uid[0], uid[1], uid[2]).value();
    SettingsDB::instance().setString(SettingsDB::kUID, uID.str().c_str());

    emio::static_buffer<64> packageTypeStr{};
    static const char *packageNames[] = {
        "LQFP64",         // 00
        "VFQFPN68",       // 01
        "LQFP100",        // 02
        "UFBGA176",       // 03
        "LQFP144",        // 04
        "LQFP48",         // 05
        "UFBGA169",       // 06
        "LQFP176",        // 07
        "undefined",      // 08
        "UFQFPN32",       // 09
        "LQFP100_SMPS",   // 0A
        "UFBGA176_SMPS",  // 0B
        "LQFP144_SMPS",   // 0C
        "LQFP176_SMPS",   // 0D
        "UFBGA169_SMPS",  // 0E
        "WLCSP25",        // 0F
        "UFQFPN48",       // 10
        "unknown"         // 11
    };
    emio::format_to(packageTypeStr, "{}", packageNames[packageType >= 0x11 ? 0x11 : packageType]).value();
    SettingsDB::instance().setString(SettingsDB::kPackageType, packageTypeStr.str().c_str());

    emio::static_buffer<64> flashSizeStr{};
    emio::format_to(flashSizeStr, "{}k", flashSize).value();
    SettingsDB::instance().setString(SettingsDB::kFlashSize, flashSizeStr.str().c_str());

    float bootCount = 0;
    SettingsDB::instance().getNumber(SettingsDB::kBootCount, &bootCount);
    bootCount++;
    SettingsDB::instance().setNumber(SettingsDB::kBootCount, bootCount);
#endif  // #ifndef BOOTLOADER

    Systick::instance().start();

    if (!Network::instance().start()) {
        return;
    }

    if (!WebServer::instance().start()) {
        return;
    }
    
    printf(ESCAPE_FG_CYAN "App up.\n");
}

void App::setup(void *first_unused_memory) {
    uint8_t *pointer = (uint8_t *)first_unused_memory;

    const size_t startup_stack_size = 8192;
    tx_thread_create(&thread_startup, (CHAR *)"startup", thread_startup_entry, 0, pointer, startup_stack_size, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
    pointer = pointer + startup_stack_size;

    pointer = Network::instance().setup(pointer);

    pointer = WebServer::instance().setup(pointer);

    printf(ESCAPE_FG_CYAN "Consumed %d bytes of RAM.\n" ESCAPE_RESET, (int)(pointer - (uint8_t *)first_unused_memory));
}

void App::init() {
}
