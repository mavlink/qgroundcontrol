#pragma once

#include <cstdint>

#include <QtCore/QByteArray>

#include "RTCMFramer.h"

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
    const uint32_t crc =
        RTCMFramer::crc24q({reinterpret_cast<const uint8_t*>(frame.constData()), static_cast<size_t>(frame.size())});
    frame.append(static_cast<char>(crc >> 16));
    frame.append(static_cast<char>(crc >> 8));
    frame.append(static_cast<char>(crc));
    return frame;
}

}  // namespace GpsTestHelpers
