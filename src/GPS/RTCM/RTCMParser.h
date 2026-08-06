#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QVector>
#include <cstddef>
#include <cstdint>

class RTCMParser
{
public:
    static constexpr uint8_t kPreamble = 0xD3;

    RTCMParser();
    void reset();

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

    bool isWhitelisted(uint16_t id) const { return _whitelist.isEmpty() || _whitelist.contains(id); }

    bool addByte(uint8_t byte);

    uint8_t* message() { return _buffer; }

    uint16_t messageLength() const { return _messageLength; }

    uint16_t messageId() const;

    const uint8_t* crcBytes() const { return _crcBytes; }

    static constexpr int kCrcSize = 3;
    static constexpr int kHeaderSize = 3;

    bool validateCrc() const;
    static uint32_t crc24q(const uint8_t* data, size_t len);

    /// Bytes of the just-completed frame (header + payload + CRC). Valid only
    /// immediately after addByte() returned true, before the next reset().
    QByteArray currentFrame() const;

    /// Feed a buffer through the parser, carrying state across calls, and return
    /// the concatenation of every complete CRC-valid frame found. Whitelist
    /// filtering is NOT applied. Optionally reports frame counts for caller logging.
    QByteArray extractValidFrames(const QByteArray& in, int* framesFound = nullptr, int* framesDropped = nullptr);

private:
    enum class State
    {
        WaitingForPreamble,
        ReadingLength,
        ReadingMessage,
        ReadingCRC
    };

    static constexpr uint16_t kMaxPayloadLength = 1023;

    QSet<int> _whitelist;
    State _state;
    uint8_t _buffer[kHeaderSize + kMaxPayloadLength];
    uint16_t _messageLength;
    uint16_t _bytesRead;
    uint16_t _lengthBytesRead;
    uint8_t _lengthBytes[2];
    uint16_t _crcBytesRead;
    uint8_t _crcBytes[3];
};
