#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QVector>

#include <cstddef>
#include <cstdint>

#include "RTCMFramer.h"

class RTCMParser
{
public:
    static constexpr uint8_t kPreamble = 0xD3;

    RTCMParser();
    ~RTCMParser();
    void reset();

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

    bool isWhitelisted(uint16_t id) const { return _whitelist.isEmpty() || _whitelist.contains(id); }

    bool addByte(uint8_t byte);

    bool hasPartialFrame() const { return _framer.hasPartialFrame(); }

    uint8_t* message() { return _framer.message(); }

    uint16_t messageLength() const { return _framer.payloadLength(); }

    uint16_t messageId() const;

    const uint8_t* crcBytes() const { return _framer.message() + kHeaderSize + _framer.payloadLength(); }

    static constexpr int kCrcSize = 3;
    static constexpr int kHeaderSize = 3;

    bool validateCrc() const;
    static bool isValidFrame(const QByteArray& frame);
    static uint32_t crc24q(const uint8_t* data, size_t len);

    /// Bytes of the just-completed frame (header + payload + CRC). Valid only
    /// immediately after addByte() returned true, before the next reset().
    QByteArray currentFrame() const;

private:
    RTCMFramer _framer;
    QSet<int> _whitelist;
};
