#pragma once

#include "RTCMParser.h"

#include <QtCore/QByteArray>

namespace RTCMTestHelper {

// Build a valid RTCM3 frame with correct CRC.
// messageId: 12-bit RTCM message type (e.g. 1005, 1077)
// payload:   arbitrary body bytes (excluding the 2 id bytes baked into header+payload)
inline QByteArray buildFrame(uint16_t messageId, const QByteArray &body = {})
{
    // Payload = 2 bytes of message-id bits + body
    const uint16_t payloadLen = static_cast<uint16_t>(2 + body.size());

    QByteArray frame;
    frame.reserve(3 + payloadLen + 3);

    // Header: preamble + 10-bit length (upper 6 bits of byte 1 are reserved/zero)
    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>((payloadLen >> 8) & 0x03));
    frame.append(static_cast<char>(payloadLen & 0xFF));

    // First 2 payload bytes carry the 12-bit message id in the top 12 bits
    frame.append(static_cast<char>((messageId >> 4) & 0xFF));
    frame.append(static_cast<char>((messageId & 0x0F) << 4));

    frame.append(body);

    // CRC-24Q over header + payload
    const uint32_t crc = RTCMParser::crc24q(
        reinterpret_cast<const uint8_t *>(frame.constData()),
        static_cast<size_t>(frame.size()));

    frame.append(static_cast<char>((crc >> 16) & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(crc & 0xFF));

    return frame;
}

// Reassemble the original RTCM payload from captured MAVLink fragments.
// Fragments must be in order and belong to the same sequence id.
inline QByteArray reassembleFromMavlink(const QByteArray &mavlinkPayloads)
{
    return mavlinkPayloads; // simple concatenation for unfragmented
}

} // namespace RTCMTestHelper
