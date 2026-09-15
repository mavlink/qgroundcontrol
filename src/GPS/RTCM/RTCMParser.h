#pragma once

#include <cstddef>
#include <cstdint>

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QVector>

#include "RTCMFramer.h"

/// Legacy parser API; messageLength() remains the payload length.
class RTCMParser
{
public:
    static constexpr uint8_t kPreamble = RTCMFramer::PREAMBLE;

    RTCMParser();
    void reset();

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

    bool isWhitelisted(uint16_t id) const { return _whitelist.isEmpty() || _whitelist.contains(id); }

    bool addByte(uint8_t byte);

    /// Drain recovered candidates before adding more bytes.
    bool nextFrame() { return _framer.nextFrame(); }

    uint8_t* message() { return _framer.message(); }

    uint16_t messageLength() const { return _framer.payloadLength(); }

    uint16_t messageId() const { return _framer.messageId(); }

    /// CRC bytes are available only after a full-length candidate.
    const uint8_t* crcBytes() const { return _framer.message() + kHeaderSize + messageLength(); }

    static constexpr int kCrcSize = RTCMFramer::CRC_SIZE;
    static constexpr int kHeaderSize = RTCMFramer::HEADER_SIZE;

    bool validateCrc() const { return _framer.valid(); }

    static uint32_t crc24q(const uint8_t* data, size_t len);

    /// Complete candidate or rejected header; empty while incomplete or reset.
    QByteArray currentFrame() const;

private:
    RTCMFramer _framer;
    QSet<int> _whitelist;
};
