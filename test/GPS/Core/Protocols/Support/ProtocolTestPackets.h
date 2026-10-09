#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <QtCore/QByteArray>

#include "Checksums.h"
#include "LittleEndian.h"
#include "NMEASentence.h"
#include "Protocols/Support/GPSModelViolations.h"

// Wire frames the GPS tests feed to decoders and receiver models answer with. Fixtures encode documented offsets
// independently of decoded object alignment.

namespace GPSTest {

inline std::span<const uint8_t> bytesOf(const QByteArray& bytes)
{
    return {reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())};
}

inline std::span<uint8_t> mutableBytesOf(QByteArray& bytes)
{
    return {reinterpret_cast<uint8_t*>(bytes.data()), static_cast<size_t>(bytes.size())};
}

inline char hexDigit(unsigned value)
{
    value &= 0xf;
    return static_cast<char>(value < 10 ? '0' + value : 'A' + value - 10);
}

/// "$<body>*<checksum>" and @a lineEnding.
inline std::string nmeaSentence(std::string_view body, std::string_view lineEnding = "\r\n")
{
    const uint8_t checksum = QGC::nmeaChecksum({reinterpret_cast<const uint8_t*>(body.data()), body.size()});
    std::string result;
    result.reserve(body.size() + 4 + lineEnding.size());
    result.push_back('$');
    result.append(body);
    result.push_back('*');
    result.push_back(hexDigit(checksum >> 4));
    result.push_back(hexDigit(checksum));
    result.append(lineEnding);
    return result;
}

/// Whether @a sentence is one NMEA frame with a matching checksum.
inline bool checksumValid(const QByteArray& sentence)
{
    const auto frame = NMEA::frame(std::string_view(sentence.constData(), sentence.size()));
    return frame && frame->hasValidChecksum();
}

inline std::vector<uint8_t> nmeaPacket(std::string_view body, std::string_view lineEnding = "\r\n")
{
    const auto text = nmeaSentence(body, lineEnding);
    return {text.begin(), text.end()};
}

/// Recomputes the checksum of a whole UBX @a frame, header to checksum bytes.
inline void ubxChecksum(std::vector<uint8_t>& frame)
{
    if (frame.size() < 8) {
        ModelViolations::record("UBX frame too short for a checksum");
        return;
    }
    const auto checksum = QGC::fletcher8(std::span<const uint8_t>(frame).subspan(2, frame.size() - 4));
    frame[frame.size() - 2] = checksum.a;
    frame.back() = checksum.b;
}

inline std::vector<uint8_t> ubxFrame(uint16_t message, std::span<const uint8_t> payload)
{
    if (payload.size() > UINT16_MAX) {
        ModelViolations::record("UBX payload too large");
        return {};
    }
    std::vector<uint8_t> result{
        0xb5, 0x62, uint8_t(message), uint8_t(message >> 8), uint8_t(payload.size()), uint8_t(payload.size() >> 8)};
    result.insert(result.end(), payload.begin(), payload.end());
    result.insert(result.end(), {0, 0});
    ubxChecksum(result);
    return result;
}

inline std::vector<uint8_t> ubxFrame(uint16_t message, std::initializer_list<uint8_t> payload)
{
    return ubxFrame(message, std::span<const uint8_t>(payload.begin(), payload.size()));
}

/// A NAV-PVT with payload @a pvt at @a tow, followed by the NAV-EOE that publishes its epoch.
inline std::vector<uint8_t> ubxNavigationEpoch(std::vector<uint8_t> pvt, uint32_t tow)
{
    (void) LittleEndian::write(pvt, 0, tow);
    auto frames = ubxFrame(0x0701, pvt);
    const auto end = ubxFrame(0x6101, std::span<const uint8_t>(pvt).first(4));
    frames.insert(frames.end(), end.begin(), end.end());
    return frames;
}

/// A NAV-PVT payload of a valid 3D fix at 47 N 8 E.
inline std::vector<uint8_t> ubxFix3D()
{
    std::vector<uint8_t> pvt(92);
    pvt[20] = 3;
    pvt[21] = 1;
    (void) LittleEndian::write<int32_t>(pvt, 24, 80000000);
    (void) LittleEndian::write<int32_t>(pvt, 28, 470000000);
    return pvt;
}

inline std::vector<uint8_t> rtcmPacket(std::span<const uint8_t> payload)
{
    std::vector<uint8_t> result{0xd3, static_cast<uint8_t>(payload.size() >> 8), static_cast<uint8_t>(payload.size())};
    result.insert(result.end(), payload.begin(), payload.end());
    const auto crc = QGC::crc24q(result);
    result.insert(result.end(), {uint8_t(crc >> 16), uint8_t(crc >> 8), uint8_t(crc)});
    return result;
}

inline std::vector<uint8_t> rtcmPacket(std::string_view payload)
{
    return rtcmPacket({reinterpret_cast<const uint8_t*>(payload.data()), payload.size()});
}

inline std::vector<uint8_t> sbfBlock(uint16_t id, std::span<const uint8_t> payload, uint32_t tow = 0,
                                     uint16_t week = 2435)
{
    if (payload.size() > UINT16_MAX - 14) {
        ModelViolations::record("SBF payload too large");
        return {};
    }
    std::vector<uint8_t> packet(14 + payload.size());
    (void) LittleEndian::write<uint16_t>(packet, 0, 0x4024);
    (void) LittleEndian::write<uint16_t>(packet, 4, id);
    (void) LittleEndian::write<uint16_t>(packet, 6, uint16_t(packet.size()));
    (void) LittleEndian::write<uint32_t>(packet, 8, tow);
    (void) LittleEndian::write<uint16_t>(packet, 12, week);
    std::copy(payload.begin(), payload.end(), packet.begin() + 14);
    (void) LittleEndian::write<uint16_t>(packet, 2, QGC::crc16Xmodem(std::span<const uint8_t>(packet).subspan(4)));
    return packet;
}

inline QByteArray toByteArray(std::span<const uint8_t> bytes)
{
    return QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
}

inline QByteArray toByteArray(std::string_view text)
{
    return QByteArray(text.data(), static_cast<qsizetype>(text.size()));
}

inline QByteArray nmeaBytes(QByteArrayView body)
{
    return QByteArray::fromStdString(nmeaSentence({body.data(), static_cast<size_t>(body.size())}));
}

inline QByteArray ubxBytes(uint16_t message, QByteArrayView payload)
{
    return toByteArray(ubxFrame(message, {reinterpret_cast<const uint8_t*>(payload.data()), size_t(payload.size())}));
}

inline QByteArray ubxBytes(uint16_t message, std::initializer_list<uint8_t> payload)
{
    return toByteArray(ubxFrame(message, payload));
}

inline QByteArray rtcmBytes(QByteArrayView payload)
{
    return toByteArray(rtcmPacket({reinterpret_cast<const uint8_t*>(payload.data()), size_t(payload.size())}));
}

/// An RTCM3 frame of message @a messageId whose payload carries @a extraPayloadBytes counting bytes after the ID.
inline QByteArray rtcmMessage(uint16_t messageId, int extraPayloadBytes = 0)
{
    QByteArray payload;
    payload.append(static_cast<char>((messageId >> 4) & 0xff));
    payload.append(static_cast<char>((messageId & 0x0f) << 4));
    for (int index = 0; index < extraPayloadBytes; ++index) {
        payload.append(static_cast<char>(index & 0xff));
    }
    return rtcmBytes(payload);
}

}  // namespace GPSTest
