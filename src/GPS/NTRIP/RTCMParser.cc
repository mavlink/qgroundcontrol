#include "RTCMParser.h"

RTCMParser::RTCMParser()
{
    reset();
}

void RTCMParser::reset()
{
    _state = WaitingForPreamble;
    _messageLength = 0;
    _bytesRead = 0;
    _lengthBytesRead = 0;
    _crcBytesRead = 0;
}

bool RTCMParser::addByte(uint8_t byte)
{
    switch (_state) {
    case WaitingForPreamble:
        if (byte == RTCM3_PREAMBLE) {
            _buffer[0] = byte;
            _bytesRead = 1;
            _state = ReadingLength;
            _lengthBytesRead = 0;
        }
        break;

    case ReadingLength:
        _lengthBytes[_lengthBytesRead++] = byte;
        _buffer[_bytesRead++] = byte;
        if (_lengthBytesRead == 2) {
            _messageLength = ((_lengthBytes[0] & 0x03) << 8) | _lengthBytes[1];
            if (_messageLength > 0 && _messageLength <= kMaxPayloadLength) {
                _state = ReadingMessage;
            } else {
                reset();
            }
        }
        break;

    case ReadingMessage:
        if (_bytesRead < kHeaderSize + kMaxPayloadLength) {
            _buffer[_bytesRead++] = byte;
        }
        if (_bytesRead >= _messageLength + kHeaderSize) {
            _state = ReadingCRC;
            _crcBytesRead = 0;
        }
        break;

    case ReadingCRC:
        _crcBytes[_crcBytesRead++] = byte;
        if (_crcBytesRead == kCrcSize) {
            return true;
        }
        break;
    }
    return false;
}

uint16_t RTCMParser::messageId() const
{
    if (_messageLength >= 2) {
        return ((_buffer[3] << 4) | (_buffer[4] >> 4)) & 0xFFF;
    }
    return 0;
}

uint32_t RTCMParser::crc24q(const uint8_t* data, size_t len)
{
    static constexpr uint32_t kPoly = 0x1864CFB;
    uint32_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= static_cast<uint32_t>(data[i]) << 16;
        for (int j = 0; j < 8; j++) {
            crc <<= 1;
            if (crc & 0x1000000) {
                crc ^= kPoly;
            }
        }
    }
    return crc & 0xFFFFFF;
}

bool RTCMParser::validateCrc() const
{
    if (_messageLength == 0 || _bytesRead < kHeaderSize + _messageLength) {
        return false;
    }

    const uint32_t computed = crc24q(_buffer, kHeaderSize + _messageLength);
    const uint32_t received = (static_cast<uint32_t>(_crcBytes[0]) << 16) |
                              (static_cast<uint32_t>(_crcBytes[1]) << 8) |
                               static_cast<uint32_t>(_crcBytes[2]);
    return computed == received;
}
