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
#include "./artnet.h"

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <emio/buffer.hpp>
#include <emio/format.hpp>

#include "./app.h"
#include "./control.h"
#include "./network.h"
#include "./settingsdb.h"
#include "./systick.h"
#include "version.h"

#ifndef BOOTLOADER

static ArtSyncWatchDog syncWatchDog;

void ArtSyncWatchDog::feed() { fedtime = Systick::instance().systemTime(); }

bool ArtSyncWatchDog::starved() {
    double now = Systick::instance().systemTime();
    if (fedtime == 0 || ((now - fedtime) > ArtSyncTimeout)) {
        fedtime = 0;
        return true;
    }
    return false;
}

class OutputPacket : public ArtNetPacket {
   public:
    OutputPacket(){};

    size_t len() const;
    uint8_t sequence() const;
    uint8_t physical() const;
    uint16_t universe() const;

    const uint8_t *data() const { return &packet[18]; }

   private:
    virtual bool verify() const override;
};

class OutputNzsPacket : public ArtNetPacket {
   public:
    OutputNzsPacket(){};

    size_t len() const;
    uint8_t sequence() const;
    uint8_t startCode() const;
    uint16_t universe() const;

    const uint8_t *data() const { return &packet[18]; }

   private:
    virtual bool verify() const override;
};

int ArtNetPacket::version() const { return static_cast<int>((packet[10] << 8) | (packet[11])); }

ArtNetPacket::Opcode ArtNetPacket::opcode() const { return static_cast<Opcode>((packet[8]) | (packet[9] << 8)); }

ArtNetPacket::Opcode ArtNetPacket::maybeValid(const uint8_t *buf, size_t len) {
    bool bufValid = buf ? true : false;

    bool sizeValid = len <= sizeof(packet);

    bool validSignature = memcmp(buf, "Art-Net", 8) == 0;

    bool opcodeValid = false;

    Opcode op = static_cast<Opcode>((buf[8]) | (buf[9] << 8));
    switch (op) {
        case OpPoll:
        case OpPollReply:
        case OpDiagData:
        case OpCommand:
        case OpOutput:
        case OpNzs:
        case OpSync:
        case OpAddress:
        case OpInput:
        case OpTodRequest:
        case OpTodData:
        case OpTodControl:
        case OpRdm:
        case OpRdmSub:
        case OpVideoSetup:
        case OpVideoPalette:
        case OpVideoData:
        case OpMacMaster:
        case OpMacSlave:
        case OpFirmwareMaster:
        case OpFirmwareReply:
        case OpFileTnMaster:
        case OpFileFnMaster:
        case OpFileFnReply:
        case OpIpProg:
        case OpIpProgReply:
        case OpMedia:
        case OpMediaPatch:
        case OpMediaControl:
        case OpMediaContrlReply:
        case OpTimeCode:
        case OpTimeSync:
        case OpTrigger:
        case OpDirectory:
        case OpDirectoryReply:
            opcodeValid = true;
            break;
        default:
            opcodeValid = false;
            break;
    }

    bool versionValid = static_cast<int>((buf[10] << 8) | (buf[11])) >= currentVersion;

    return (bufValid && sizeValid && validSignature && opcodeValid && versionValid) ? op : OpInvalid;
}

bool ArtNetPacket::verify(ArtNetPacket &packet, const uint8_t *buf, size_t len) {  // cppcheck-suppress constParameterReference
    Opcode op = maybeValid(buf, len);
    if (op == OpInvalid) {
        return false;
    }
    memcpy(packet.packet.data(), buf, std::min(len, packet.packet.size()));
    switch (op) {
        case OpPoll:
        case OpSync:
        case OpNzs:
        case OpOutput: {
            return packet.verify();
        } break;
        default: {
            return false;
        } break;
    }
    return false;
}

static constexpr uint32_t syncTimeout = 4;

void ArtNetPacket::sendArtPollReply(const NXD_ADDRESS *from, uint16_t universe) {
    struct ArtPollReply {
        uint8_t artNet[8];
        uint16_t opCode;
        uint8_t ipAddress[4];
        uint16_t portNumber;
        uint16_t versionInfo;
        uint8_t netSwitch;
        uint8_t subSwitch;
        uint16_t oem;
        uint8_t uebaVersion;
        uint8_t status1;
        uint16_t estaManufactor;
        uint8_t shortName[18];
        uint8_t longName[64];
        uint8_t nodeReport[64];
        uint16_t numPorts;
        uint8_t portTypes[4];
        uint8_t goodInput[4];
        uint8_t goodOutput[4];
        uint8_t swIn[4];
        uint8_t swOut[4];
        uint8_t swVideo;
        uint8_t swMacro;
        uint8_t swRemote;
        uint8_t spare1;
        uint8_t spare2;
        uint8_t spare3;
        uint8_t style;
        uint8_t macAddress[6];
        uint8_t bindIp[4];
        uint8_t bindIndex;
        uint8_t status2;
        uint8_t filler[26];
    } __attribute__((packed)) reply;
    memset(&reply, 0, sizeof(reply));

    reply.opCode = OpPollReply;
    memcpy(reply.artNet, "Art-Net", 8);
    reply.ipAddress[0] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 24) & 0xFF;
    reply.ipAddress[1] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 16) & 0xFF;
    reply.ipAddress[2] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 8) & 0xFF;
    reply.ipAddress[3] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 0) & 0xFF;
    reply.portNumber = 6454;
    reply.versionInfo = GIT_REV_COUNT_INT;
    reply.netSwitch = uint8_t((universe >> 8) & 0xFF);
    reply.subSwitch = uint8_t((universe >> 0) & 0xFF);
    reply.oem = 0x1ed5;
    reply.estaManufactor = 0x1ed5;

    constexpr char short_hostname_base[] = "lk-";
    constexpr std::array hex_table = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    };
    char short_hostname[sizeof(short_hostname_base) + 8];
    memset(short_hostname, 0, sizeof(short_hostname));
    strcpy(short_hostname, short_hostname_base);
    uint32_t short_mac = (Network::instance().MACAddr()[2] << 24) | (Network::instance().MACAddr()[3] << 16) | (Network::instance().MACAddr()[4] << 8) |
                         (Network::instance().MACAddr()[5] << 0);
    for (size_t c = 0; c < 8; c++) {
        short_hostname[c + sizeof(short_hostname_base) - 1] = hex_table[(short_mac >> (32 - ((c + 1) * 4))) & 0xF];
    }

    strncpy((char *)reply.shortName, short_hostname, 17);

    char tag[17] = {};
    if (SettingsDB::instance().getString(SettingsDB::kTag, (char *)&tag, sizeof(tag)) > 0) {
        snprintf((char *)reply.longName, 63, "%.20s - %.16s", Network::instance().hostName(), tag);
    } else {
        snprintf((char *)reply.longName, 63, "%.20s", Network::instance().hostName());
    }

    memcpy(reply.macAddress, Network::instance().MACAddr(), 6);
    reply.bindIp[0] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 24) & 0xFF;
    reply.bindIp[1] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 16) & 0xFF;
    reply.bindIp[2] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 8) & 0xFF;
    reply.bindIp[3] = (Network::instance().ipv4Addr()->nxd_ip_address.v4 >> 0) & 0xFF;

    NXD_ADDRESS ipv4 = *Network::instance().ipv4Addr();
    reply.status2 = 0x01 |                                         // support web browser config
                    0x02 |                                         // supports dhcp
                    (ipv4.nxd_ip_address.v4 == 0 ? 0x04 : 0x00) |  // using dhcp
                    0x08;                                          // ArtNet3

    Network::instance().ArtNetSend(from, port, (const uint8_t *)&reply, sizeof(reply));
}

bool ArtNetPacket::dispatch(const NXD_ADDRESS *from, const uint8_t *buf, size_t len, bool isBroadcast) {
    Opcode op = ArtNetPacket::maybeValid(buf, len);
    if (op == OpInvalid) {
        return false;
    }
    switch (op) {
        case OpPoll: {
            Control::instance().interateAllActiveArtnetUniverses([from](uint16_t universe) { Systick::instance().schedulePollReply(from, universe); });
            return true;
        } break;
        case OpSync: {
            if (!Model::instance().broadcastEnabled && isBroadcast) {
                return false;
            }
            Control::instance().setEnableSyncMode(true);
            Control::instance().sync();
            syncWatchDog.feed();
            return true;
        } break;
        case OpNzs: {
            if (!Model::instance().broadcastEnabled && isBroadcast) {
                return false;
            }
            OutputNzsPacket outputPacket;
            if (ArtNetPacket::verify(outputPacket, buf, len)) {
                Control::instance().setArtnetUniverseOutputData(outputPacket.universe(), outputPacket.data(), outputPacket.len());
                if (Control::instance().syncModeEnabled() && syncWatchDog.starved()) {
                    Control::instance().sync();
                    Control::instance().setEnableSyncMode(false);
                }
                return true;
            }
        } break;
        case OpOutput: {
            if (!Model::instance().broadcastEnabled && isBroadcast) {
                return false;
            }
            OutputPacket outputPacket;
            if (ArtNetPacket::verify(outputPacket, buf, len)) {
                Control::instance().setArtnetUniverseOutputData(outputPacket.universe(), outputPacket.data(), outputPacket.len());
                if (Control::instance().syncModeEnabled() && syncWatchDog.starved()) {
                    Control::instance().sync();
                    Control::instance().setEnableSyncMode(false);
                }
                return true;
            }
        } break;
        default: {
            return false;
        } break;
    }
    return false;
}

size_t OutputPacket::len() const { return (packet[16] << 8) | packet[17]; }

uint16_t OutputPacket::universe() const { return (packet[14]) | (packet[15] << 8); }

uint8_t OutputPacket::sequence() const { return packet[12]; }

uint8_t OutputPacket::physical() const { return packet[13]; }

bool OutputPacket::verify() const {
    if (len() < 2) {
        return false;
    }
    if (len() > 512) {
        return false;
    }
    if ((len() & 1) == 1) {
        return false;
    }
    if (universe() >= 32768) {
        return false;
    }
    return true;
}

size_t OutputNzsPacket::len() const { return (packet[16] << 8) | packet[17]; }

uint16_t OutputNzsPacket::universe() const { return (packet[14]) | (packet[15] << 8); }

uint8_t OutputNzsPacket::sequence() const { return packet[12]; }

uint8_t OutputNzsPacket::startCode() const { return packet[13]; }

bool OutputNzsPacket::verify() const {
    if (len() < 2) {
        return false;
    }
    if (len() > 512) {
        return false;
    }
    if ((len() & 1) == 1) {
        return false;
    }
    if (universe() >= 32768) {
        return false;
    }
    if (startCode() != 0) {
        return false;
    }
    return true;
}

#endif  // #ifndef BOOTLOADER
