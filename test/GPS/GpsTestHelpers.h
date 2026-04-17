#pragma once

#include "RTCMParser.h"

#include <QtCore/QByteArray>
#include <cstdint>

namespace GpsTestHelpers {

// Build a minimal RTCM3 frame with preamble, length, message ID, and CRC-24Q
inline QByteArray buildRtcmFrame(uint16_t messageId, int extraPayloadBytes = 0)
{
    const int payloadLen = 2 + extraPayloadBytes;
    QByteArray frame;

    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>((payloadLen >> 8) & 0x03));
    frame.append(static_cast<char>(payloadLen & 0xFF));

    frame.append(static_cast<char>((messageId >> 4) & 0xFF));
    frame.append(static_cast<char>((messageId & 0x0F) << 4));

    for (int i = 0; i < extraPayloadBytes; i++) {
        frame.append(static_cast<char>(i & 0xFF));
    }

    const uint32_t crc = RTCMParser::crc24q(
        reinterpret_cast<const uint8_t*>(frame.constData()),
        static_cast<size_t>(frame.size()));

    frame.append(static_cast<char>((crc >> 16) & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(crc & 0xFF));

    return frame;
}

// Verify NMEA checksum: XOR of bytes between '$' and '*'
inline bool verifyNmeaChecksum(const QByteArray& sentence)
{
    if (sentence.size() < 6 || sentence.at(0) != '$') {
        return false;
    }

    int star = sentence.lastIndexOf('*');
    if (star < 2 || star + 3 > sentence.size()) {
        return false;
    }

    quint8 calc = 0;
    for (int i = 1; i < star; ++i) {
        calc ^= static_cast<quint8>(sentence.at(i));
    }

    QByteArray expected = QByteArray::number(calc, 16).rightJustified(2, '0').toUpper();
    QByteArray actual = sentence.mid(star + 1, 2).toUpper();
    return actual == expected;
}

} // namespace GpsTestHelpers
