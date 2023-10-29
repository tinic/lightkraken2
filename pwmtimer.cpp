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
#include <stdio.h>

#include "./pwmtimer.h"

PwmTimer &PwmTimer0::instance() {
    static PwmTimer0 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer0::init() {
}

void PwmTimer0::setPulse(uint16_t pulse) {
}
    
PwmTimer &PwmTimer1::instance() {
    static PwmTimer1 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }        
    return timer;
}

void PwmTimer1::init() {
}

void PwmTimer1::setPulse(uint16_t pulse) {
}

PwmTimer &PwmTimer2::instance() {
    static PwmTimer2 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer2::init() {
}

void PwmTimer2::setPulse(uint16_t pulse) {
}

PwmTimer &PwmTimer3::instance() {
    static PwmTimer3 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer3::init() {
}

void PwmTimer3::setPulse(uint16_t pulse) {
}

PwmTimer &PwmTimer4::instance() {
    static PwmTimer4 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer4::init() {
}

void PwmTimer4::setPulse(uint16_t pulse) {
}

PwmTimer &PwmTimer5::instance() {
    static PwmTimer5 timer;
    if (!timer.initialized) {
        timer.initialized = true;
        timer.init();
    }
    return timer;
}

void PwmTimer5::init() {
}

void PwmTimer5::setPulse(uint16_t pulse) {
}
