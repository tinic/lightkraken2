
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
#include <emio/buffer.hpp>
#include <emio/format.hpp>

#include "./artnet.h"
#include "./control.h"
#include "./network.h"
#include "./version.h"

#ifndef BOOTLOADER

struct ddp_hdr_struct {
    uint8_t flags1;
    uint8_t flags2;
    uint8_t type;
    uint8_t id;
    uint8_t offset1;
    uint8_t offset2;
    uint8_t offset3;
    uint8_t offset4;
    uint8_t len1;
    uint8_t len2;
    uint8_t data[DDPPacket::maxDDPPayloadSize];
} __attribute__((packed));

enum {
    DDP_FLAGS1_PUSH = 0x01,
    DDP_FLAGS1_QUERY = 0x02,
    DDP_FLAGS1_REPLY = 0x04,
    DDP_FLAGS1_STORAGE = 0x08,
    DDP_FLAGS1_TIMECODE = 0x10,
    DDP_FLAGS1_VER1 = 0x40,
};

enum { DDP_ID_DISPLAY = 1, DDP_ID_CONTROL = 246, DDP_ID_CONFIG = 250, DDP_ID_STATUS = 251, DDP_ID_DMX = 254, DDP_ID_ALL = 255 };

class DDPDataPacketSet : public DDPPacket {
   public:
    DDPDataPacketSet(){};
    virtual ~DDPDataPacketSet(){};

    void apply(){};

   private:
    virtual bool verify() const override {
        if (packet[0] != DDP_FLAGS1_VER1) {
            return false;
        }
        if (packet[3] != DDP_ID_DISPLAY) {
            return false;
        }
        return true;
    }
};

class DDPDataPacketQuery : public DDPPacket {
   public:
    DDPDataPacketQuery(){};
    virtual ~DDPDataPacketQuery(){};

    static void sendReply() {
        ddp_hdr_struct reply{.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_REPLY | DDP_FLAGS1_PUSH,
                             .flags2 = 0,
                             .type = 0,
                             .id = DDP_ID_DISPLAY,
                             .offset1 = 0,
                             .offset2 = 0,
                             .offset3 = 0,
                             .offset4 = 0,
                             .len1 = 0,
                             .len2 = 0,
                             .data = {}};
        Network::instance().DDPSend(Network::instance().ipv4Addr(), port, (const uint8_t *)&reply, offsetof(ddp_hdr_struct, data));
    }

   private:
    virtual bool verify() const override { return true; }
};

class DDPStatusQueryPacket : public DDPPacket {
   public:
    DDPStatusQueryPacket(){};
    virtual ~DDPStatusQueryPacket(){};

    static void sendReply() {
        const uint8_t *macaddr = Network::instance().MACAddr();
        emio::static_buffer<maxDDPPayloadSize> buf;
        emio::format_to(buf,
                        "{{\"status\":{{"
                        "\"man\":\"{}\","
                        "\"mod\":\"{}\","
                        "\"ver\":\"{}\","
                        "\"mac\":\"{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}\","
                        "\"push\":{},"
                        "\"ntp\":{}"
                        "}}}}",
                        "obtainium cons", "lightkraken", "2.0." GIT_REV_COUNT, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5], "true",
                        "true")
            .value();
        size_t len = buf.str().length();
        ddp_hdr_struct reply{.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_REPLY | DDP_FLAGS1_PUSH,
                             .flags2 = 0,
                             .type = 0,
                             .id = DDP_ID_STATUS,
                             .offset1 = 0,
                             .offset2 = 0,
                             .offset3 = 0,
                             .offset4 = 0,
                             .len1 = uint8_t((len >> 8) & 0xFF),
                             .len2 = uint8_t((len >> 0) & 0xFF),
                             .data = {}};
        memcpy(reply.data, buf.str().c_str(), len);
        Network::instance().DDPSend(Network::instance().ipv4Addr(), port, (const uint8_t *)&reply, offsetof(ddp_hdr_struct, data) + len);
    }

   private:
    virtual bool verify() const override {
        if (packet[0] != (DDP_FLAGS1_VER1 | DDP_FLAGS1_QUERY)) {
            return false;
        }
        if (packet[3] != DDP_ID_STATUS) {
            return false;
        }
        if (packet[1] != 0 || packet[2] != 0 || packet[4] != 0 || packet[5] != 0 || packet[6] != 0 || packet[7] != 0 || packet[8] != 0 || packet[9] != 0) {
            return false;
        }
        return true;
    }
};

class DDPConfigSetPacket : public DDPPacket {
   public:
    DDPConfigSetPacket(){};
    virtual ~DDPConfigSetPacket(){};

    void apply(){};

   private:
    virtual bool verify() const override {
        if (packet[0] != DDP_FLAGS1_VER1) {
            return false;
        }
        if (packet[3] != DDP_ID_CONFIG) {
            return false;
        }
        return true;
    }
};

class DDPConfigQueryPacket : public DDPPacket {
   public:
    DDPConfigQueryPacket(){};
    virtual ~DDPConfigQueryPacket(){};

    static void sendReply() {
        emio::static_buffer<maxDDPPayloadSize> buf;
        // TODO
        size_t len = buf.str().length();
        ddp_hdr_struct reply{.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_REPLY | DDP_FLAGS1_PUSH,
                             .flags2 = 0,
                             .type = 0,
                             .id = DDP_ID_CONFIG,
                             .offset1 = 0,
                             .offset2 = 0,
                             .offset3 = 0,
                             .offset4 = 0,
                             .len1 = uint8_t((len >> 8) & 0xFF),
                             .len2 = uint8_t((len >> 0) & 0xFF),
                             .data = {}};
        memcpy(reply.data, buf.str().c_str(), len);
        Network::instance().DDPSend(Network::instance().ipv4Addr(), port, (const uint8_t *)&reply, offsetof(ddp_hdr_struct, data) + len);
    }

   private:
    virtual bool verify() const override {
        if (packet[0] != (DDP_FLAGS1_VER1 | DDP_FLAGS1_QUERY)) {
            return false;
        }
        if (packet[3] != DDP_ID_CONFIG) {
            return false;
        }
        if (packet[1] != 0 || packet[2] != 0 || packet[4] != 0 || packet[5] != 0 || packet[6] != 0 || packet[7] != 0 || packet[8] != 0 || packet[9] != 0) {
            return false;
        }
        return true;
    }
};

class DDPControlSetPacket : public DDPPacket {
   public:
    DDPControlSetPacket(){};
    virtual ~DDPControlSetPacket(){};

    void apply(){};

   private:
    virtual bool verify() const override {
        if (packet[0] != DDP_FLAGS1_VER1) {
            return false;
        }
        if (packet[3] != DDP_ID_CONTROL) {
            return false;
        }
        return true;
    }
};

class DDPControlQueryPacket : public DDPPacket {
   public:
    DDPControlQueryPacket(){};
    virtual ~DDPControlQueryPacket(){};

    static void sendReply(){};

   private:
    virtual bool verify() const override {
        if (packet[0] != (DDP_FLAGS1_VER1 | DDP_FLAGS1_QUERY)) {
            return false;
        }
        if (packet[3] != DDP_ID_CONTROL) {
            return false;
        }
        if (packet[1] != 0 || packet[2] != 0 || packet[4] != 0 || packet[5] != 0 || packet[6] != 0 || packet[7] != 0 || packet[8] != 0 || packet[9] != 0) {
            return false;
        }
        return true;
    }
};

class DDPDMXSetPacket : public DDPPacket {
   public:
    DDPDMXSetPacket(){};
    virtual ~DDPDMXSetPacket(){};

    void apply(){};

   private:
    virtual bool verify() const override {
        if (packet[0] != DDP_FLAGS1_VER1) {
            return false;
        }
        if (packet[3] != DDP_ID_DMX) {
            return false;
        }
        return true;
    }
};

class DDPDMXQueryPacket : public DDPPacket {
   public:
    DDPDMXQueryPacket(){};
    virtual ~DDPDMXQueryPacket(){};

    static void sendReply() {
        ddp_hdr_struct reply{.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_REPLY | DDP_FLAGS1_PUSH,
                             .flags2 = 0,
                             .type = 0,
                             .id = DDP_ID_DMX,
                             .offset1 = 0,
                             .offset2 = 0,
                             .offset3 = 0,
                             .offset4 = 0,
                             .len1 = 0,
                             .len2 = 0,
                             .data = {}};
        Network::instance().DDPSend(Network::instance().ipv4Addr(), port, (const uint8_t *)&reply, offsetof(ddp_hdr_struct, data));
    }

   private:
    virtual bool verify() const override {
        if (packet[0] != (DDP_FLAGS1_VER1 | DDP_FLAGS1_QUERY)) {
            return false;
        }
        if (packet[3] != DDP_ID_ALL) {
            return false;
        }
        return true;
    }
};

class DDPAllSetPacket : public DDPPacket {
   public:
    DDPAllSetPacket(){};
    virtual ~DDPAllSetPacket(){};

    void apply(){};

   private:
    virtual bool verify() const override {
        if (packet[0] != DDP_FLAGS1_VER1) {
            return false;
        }
        if (packet[3] != DDP_ID_ALL) {
            return false;
        }
        return true;
    }
};

class DDPAllQueryPacket : public DDPPacket {
   public:
    DDPAllQueryPacket(){};
    virtual ~DDPAllQueryPacket(){};

    static void sendReply() {
        ddp_hdr_struct reply{.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_REPLY | DDP_FLAGS1_PUSH,
                             .flags2 = 0,
                             .type = 0,
                             .id = DDP_ID_ALL,
                             .offset1 = 0,
                             .offset2 = 0,
                             .offset3 = 0,
                             .offset4 = 0,
                             .len1 = 0,
                             .len2 = 0,
                             .data = {}};
        Network::instance().DDPSend(Network::instance().ipv4Addr(), port, (const uint8_t *)&reply, offsetof(ddp_hdr_struct, data));
    }

   private:
    virtual bool verify() const override { return true; }
};

bool DDPPacket::dispatch(const NXD_ADDRESS *from, const uint8_t *buf, size_t len, bool isBroadcast) {
    (void)from;
    PacketType type = DDPPacket::maybeValid(buf, len);
    if (type == PacketInvalid) {
        return false;
    }
    if (!Model::instance().broadcastEnabled && isBroadcast) {
        return false;
    }
    switch (type) {
        case PacketDataQuery: {
            DDPDataPacketQuery dataQueryPacket;
            if (DDPPacket::verify(dataQueryPacket, buf, len)) {
                DDPDataPacketQuery::sendReply();
                return true;
            }
        } break;
        case PacketDataSet: {
            DDPDataPacketSet dataPacketSet;
            if (DDPPacket::verify(dataPacketSet, buf, len)) {
                dataPacketSet.apply();
                return true;
            }
        } break;
        case PacketStatusQuery: {
            DDPStatusQueryPacket statusQueryPacket;
            if (DDPPacket::verify(statusQueryPacket, buf, len)) {
                DDPStatusQueryPacket::sendReply();
                return true;
            }
        } break;
        case PacketConfigQuery: {
            DDPConfigQueryPacket configQueryPacket;
            if (DDPPacket::verify(configQueryPacket, buf, len)) {
                DDPConfigQueryPacket::sendReply();
                return true;
            }
        } break;
        case PacketConfigSet: {
            DDPConfigSetPacket configSetPacket;
            if (DDPPacket::verify(configSetPacket, buf, len)) {
                configSetPacket.apply();
                return true;
            }
        } break;
        case PacketControlQuery: {
            DDPControlQueryPacket controlQueryPacket;
            if (DDPPacket::verify(controlQueryPacket, buf, len)) {
                DDPControlQueryPacket::sendReply();
                return true;
            }
        } break;
        case PacketControlSet: {
            DDPControlSetPacket controlSetPacket;
            if (DDPPacket::verify(controlSetPacket, buf, len)) {
                controlSetPacket.apply();
                return true;
            }
        } break;
        case PacketDMXQuery: {
            DDPDMXQueryPacket dmxQueryPacket;
            if (DDPPacket::verify(dmxQueryPacket, buf, len)) {
                DDPDMXQueryPacket::sendReply();
                return true;
            }
        } break;
        case PacketDMXSet: {
            DDPDMXSetPacket dmxSetPacket;
            if (DDPPacket::verify(dmxSetPacket, buf, len)) {
                dmxSetPacket.apply();
                return true;
            }
        } break;
        case PacketAllQuery: {
            DDPAllQueryPacket allQueryPacket;
            if (DDPPacket::verify(allQueryPacket, buf, len)) {
                DDPAllQueryPacket::sendReply();
                return true;
            }
        } break;
        case PacketAllSet: {
            DDPAllSetPacket allSetPacket;
            if (DDPPacket::verify(allSetPacket, buf, len)) {
                allSetPacket.apply();
                return true;
            }
        } break;
        case PacketInvalid: {
        } break;
    }
    return false;
}

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
        switch (buf[2]) {
            case DDP_ID_DISPLAY:
                return PacketDataQuery;
            case DDP_ID_STATUS:
                return PacketStatusQuery;
            case DDP_ID_CONFIG:
                return PacketConfigQuery;
            case DDP_ID_CONTROL:
                return PacketControlQuery;
            case DDP_ID_DMX:
                return PacketDMXQuery;
            case DDP_ID_ALL:
                return PacketAllQuery;
        }
    } else {
        switch (buf[2]) {
            case DDP_ID_DISPLAY:
                return PacketDataSet;
            case DDP_ID_STATUS:
                return PacketInvalid;
            case DDP_ID_CONFIG:
                return PacketConfigSet;
            case DDP_ID_CONTROL:
                return PacketControlSet;
            case DDP_ID_DMX:
                return PacketDMXSet;
            case DDP_ID_ALL:
                return PacketAllSet;
        }
    }
    return PacketInvalid;
}

bool DDPPacket::verify(DDPPacket &packet, const uint8_t *buf, size_t len) {  // cppcheck-suppress constParameterReference
    PacketType type = DDPPacket::maybeValid(buf, len);
    if (type == PacketInvalid) {
        return false;
    }
    memcpy(packet.packet.data(), buf, std::min(len, packet.packet.size()));
    switch (type) {
        case PacketDataSet:
        case PacketDataQuery:
        case PacketStatusQuery:
        case PacketConfigQuery:
        case PacketConfigSet:
        case PacketControlQuery:
        case PacketControlSet:
        case PacketDMXQuery:
        case PacketDMXSet:
        case PacketAllQuery:
        case PacketAllSet: {
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
