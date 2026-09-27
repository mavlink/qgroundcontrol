#pragma once

#include <cstdint>

#include <QtCore/QByteArray>

#include "../Driver/Protocols/ProtocolTestPackets.h"

namespace GpsTestHelpers {

inline QByteArray buildRtcmFrame(uint16_t messageId, int extraPayloadBytes = 0)
{
    const int payloadLength = 2 + extraPayloadBytes;
    QByteArray frame;
    frame.append(static_cast<char>(RTCMFramer::PREAMBLE));
    frame.append(static_cast<char>((payloadLength >> 8) & 0x03));
    frame.append(static_cast<char>(payloadLength & 0xff));
    frame.append(static_cast<char>((messageId >> 4) & 0xff));
    frame.append(static_cast<char>((messageId & 0x0f) << 4));
    for (int index = 0; index < extraPayloadBytes; ++index) {
        frame.append(static_cast<char>(index & 0xff));
    }
    const auto packet =
        rtcmPacket({reinterpret_cast<const uint8_t*>(frame.constData()) + 3, static_cast<size_t>(frame.size() - 3)});
    return QByteArray(reinterpret_cast<const char*>(packet.data()), static_cast<qsizetype>(packet.size()));
}

}  // namespace GpsTestHelpers
