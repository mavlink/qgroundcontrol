#pragma once

#include <cstddef>
#include <cstdint>

#include <QtCore/QSet>
#include <QtCore/QVector>

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

    bool validateCrc() const;
    static uint32_t crc24q(const uint8_t* data, size_t len);

private:
    enum class State {
        WaitingForPreamble,
        ReadingLength,
        ReadingMessage,
        ReadingCRC
    };

    static constexpr uint16_t kMaxPayloadLength = 1023;
    static constexpr int kHeaderSize = 3;

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
