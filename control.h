/*
Copyright 2023 Tinic Uro

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
#ifndef CONTROL_H
#define CONTROL_H

#include <functional>

#include "./model.h"
#include "./strip.h"
#include "tx_api.h"

#ifndef BOOTLOADER

class Control {
   public:
    static Control &instance();

    uint8_t *setup(uint8_t *pointer);
    bool start();
    void thread();

    void setArtnetUniverseOutputData(uint16_t universe, const uint8_t *data, size_t len, bool nodriver = false);
    void setE131UniverseOutputData(uint16_t universe, const uint8_t *data, size_t len, bool nodriver = false);

    void sync();
    void update();

    bool inStartup() const { return in_startup; }
    void setStartup() { in_startup = true; }
    void clearStartup() { in_startup = false; }

    void setEnableSyncMode(bool state) { syncMode = state; }
    bool syncModeEnabled() const { return syncMode; }

    void interateAllActiveArtnetUniverses(std::function<void(uint16_t universe)> callback);
    void collectAllActiveArtnetUniverses(std::array<uint16_t, Model::maxUniverses> &universes, size_t &universeCount);
    void collectAllActiveE131Universes(std::array<uint16_t, Model::maxUniverses> &universes, size_t &universeCount);

    void setDataReceived() { data_received = true; }
    bool dataReceived() const { return data_received; }
    void scheduleColor() { color_scheduled = true; }

    void setColor();
    void startupModePattern();

   private:
    std::array<uint8_t, Strip::bytesMaxLen> color_buf[Model::stripN] {};

    bool in_startup = true;
    bool color_scheduled = false;
    bool data_received = false;
    bool syncMode = false;
    //    void setColor(size_t strip, size_t index, const rgb8 &color);
    void setArtnetUniverseOutputDataForDriver(size_t channels, size_t components, uint16_t uni, const uint8_t *data, size_t len);
    void setE131UniverseOutputDataForDriver(size_t channels, size_t components, uint16_t uni, const uint8_t *data, size_t len);
    bool initialized = false;
    void init();

    TX_THREAD thread_control {};
};

#endif  // #ifndef BOOTLOADER

#endif  // #ifndef CONTROL_H