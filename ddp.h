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
#ifndef _DDP_H_
#define _DDP_H_

#include <stdint.h>

#include <array>

#include "nx_api.h"

class DDPPacket {
   public:
    static constexpr int32_t port = 4048;
    static constexpr size_t maxDDPPacketSize = 480 * 3 + 14;

    enum PacketType { PacketInvalid = -1, PacketData = 0, PacketStatusQuery = 1, PacketConfigQuery = 2 };

    static bool dispatch(const NXD_ADDRESS *from, const uint8_t *buf, size_t len, bool isBroadcast);

   protected:
    DDPPacket(){};
    virtual ~DDPPacket(){};
    virtual bool verify() const { return false; }

   private:
    static void sendStatusReply();
    static void sendConfigReply();
    static PacketType maybeValid(const uint8_t *buf, size_t len);
    static bool verify(DDPPacket &Packet, const uint8_t *buf, size_t len);
    std::array<uint8_t, maxDDPPacketSize> packet{};
};

#endif  // #ifndef _DDP_H_
