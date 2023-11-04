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
#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

#include "sacn.h"
#include "nx_api.h"
#include "stm32h5xx_hal.h"

class Systick {
   public:
    Systick() {}
    static Systick &instance();

    uint64_t systemTimeRAW() const;
    double systemTime() const { return double(systemTimeRAW()) / double(SystemCoreClock); }

#ifndef BOOTLOADER
    void schedulePollReply(const NXD_ADDRESS *from, uint16_t universe);
#endif  // #ifndef BOOTLOADER

    void handler();

    void scheduleReset(int32_t count = 2000) { 
        sACNPacket::leaveNetworks();
        resetCount = count; 
    };
    void checkReset();

    void start() { started = true; }

   private:
    bool initialized = false;
    void init();

    int32_t resetCount = 0;
    bool started = false;

#ifndef BOOTLOADER
    struct {
        NXD_ADDRESS from;
        uint16_t universe;
        int32_t delay;
    } pollReply[8] {};
#endif  // #ifndef BOOTLOADER
};

#endif  // #ifndef SYSTICK_H