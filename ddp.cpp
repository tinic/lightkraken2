
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
#include "./ddp.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>

#include "./artnet.h"
#include "./control.h"
#include "./network.h"

#ifndef BOOTLOADER

class DDPDataPacket : public DDPPacket {
   public:
    DDPDataPacket(){};
    virtual ~DDPDataPacket(){};

   private:
    virtual bool verify() const override { return false; }
};

bool DDPPacket::dispatch(const NXD_ADDRESS *from, const uint8_t *buf, size_t len, bool isBroadcast) {
    (void)from;
    PacketType type = DDPPacket::maybeValid(buf, len);
    if (type == PacketInvalid) {
        return false;
    }
    switch (type) {
        case PacketData: {
            if (!Model::instance().broadcastEnabled && isBroadcast) {
                return false;
            }
            DDPDataPacket dataPacket;
            if (DDPPacket::verify(dataPacket, buf, len)) {
                // TODO
                return true;
            }
        } break;
        case PacketStatusQuery: {
            if (!Model::instance().broadcastEnabled && isBroadcast) {
                return false;
            }
            sendStatusReply();
            return true;
        } break;
        case PacketConfigQuery: {
            if (!Model::instance().broadcastEnabled && isBroadcast) {
                return false;
            }
            sendConfigReply();
            return true;
        } break;
        case PacketInvalid: {
        } break;
    }
    return false;
}

enum {
    DDP_FLAGS1_PUSH = 0x01,
    DDP_FLAGS1_QUERY = 0x02,
    DDP_FLAGS1_REPLY = 0x04,
    DDP_FLAGS1_STORAGE = 0x08,
    DDP_FLAGS1_TIMECODE = 0x10,
    DDP_FLAGS1_VER1 = 0x40,
};

enum { DDP_ID_DISPLAY = 1, DDP_ID_CONFIG = 250, DDP_ID_STATUS = 251 };

DDPPacket::PacketType DDPPacket::maybeValid(const uint8_t *buf, size_t len) {
    if (len < 10) {
        return PacketInvalid;
    }
    if ((buf[0] >> 6) != 1) {
        return PacketInvalid;
    }
    bool timeCode = (buf[0] & DDP_FLAGS1_TIMECODE) != 0;
    size_t dataLen = (buf[8] << 8) | buf[9];
    if (dataLen != (len - 10 - (timeCode ? 4 : 0))) {
        return PacketInvalid;
    }
    if ((buf[0] & DDP_FLAGS1_QUERY) != 0) {
        if (buf[2] == DDP_ID_STATUS) {
            return PacketStatusQuery;
        }
        if (buf[2] == DDP_ID_CONFIG) {
            return PacketConfigQuery;
        }
        return PacketInvalid;
    }
    return PacketData;
}

void DDPPacket::sendStatusReply() {
    struct ddp_hdr_struct {
        uint8_t flags1;
        uint8_t flags2;
        uint8_t type;
        uint8_t id;
        uint32_t offset;
        uint8_t len1;
        uint8_t len2;
        uint8_t data[480 * 3];
    } __attribute__((packed)) reply{};

    reply.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_REPLY | DDP_FLAGS1_PUSH;
    reply.id = DDP_ID_STATUS;
    reply.type = 0;
    reply.offset = 0;
    size_t len = 0;
    // Gen json
    reply.len1 = (len >> 8) & 0xFF;
    reply.len2 = (len >> 0) & 0xFF;
    Network::instance().DDPSend(Network::instance().ipv4Addr(), port, (const uint8_t *)&reply, offsetof(ddp_hdr_struct, data) + len);
}

void DDPPacket::sendConfigReply() {
    struct ddp_hdr_struct {
        uint8_t flags1;
        uint8_t flags2;
        uint8_t type;
        uint8_t id;
        uint32_t offset;
        uint8_t len1;
        uint8_t len2;
        uint8_t data[480 * 3];
    } __attribute__((packed)) reply{};

    reply.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_REPLY | DDP_FLAGS1_PUSH;
    reply.id = DDP_ID_CONFIG;
    reply.type = 0;
    reply.offset = 0;
    size_t len = 0;
    // Gen json
    reply.len1 = (len >> 8) & 0xFF;
    reply.len2 = (len >> 0) & 0xFF;
    Network::instance().DDPSend(Network::instance().ipv4Addr(), port, (const uint8_t *)&reply, offsetof(ddp_hdr_struct, data) + len);
}

bool DDPPacket::verify(DDPPacket &packet, const uint8_t *buf, size_t len) {  // cppcheck-suppress constParameterReference
    PacketType type = DDPPacket::maybeValid(buf, len);
    if (type == PacketInvalid) {
        return false;
    }
    memcpy(packet.packet.data(), buf, std::min(len, packet.packet.size()));
    switch (type) {
        case PacketData:
        case PacketStatusQuery:
        case PacketConfigQuery: {
            return packet.verify();
        } break;
        default:
        case PacketInvalid: {
            return false;
        } break;
    }
    return false;
}

#endif  // #ifndef BOOTLOADER
