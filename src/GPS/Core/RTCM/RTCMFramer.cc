#include "RTCMFramer.h"

#include <algorithm>

#include <QtCore/QByteArrayView>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMFrameDecoderLog, "GPS.RTCM.RTCMFrameDecoder")

RTCMFrameDecoder::RTCMFrameDecoder()
{
    qCDebug(RTCMFrameDecoderLog) << this;
}

RTCMFrameDecoder::~RTCMFrameDecoder()
{
    qCDebug(RTCMFrameDecoderLog) << this;
}

bool RTCMFramer::addByte(uint8_t byte)
{
    const uint16_t validatedFrameSize = _frameSize ? _discardCandidate() : 0;
    if (!_size && byte != PREAMBLE) {
        return false;
    }
    _bytes[_size++] = byte;
    return _inspect(validatedFrameSize);
}

bool RTCMFramer::nextFrame()
{
    if (!_frameSize) {
        return false;
    }
    return _inspect(_discardCandidate());
}

void RTCMFramer::_discardPrefix(uint16_t count)
{
    if (count) {
        std::move(_bytes.begin() + count, _bytes.begin() + _size, _bytes.begin());
        _size -= count;
        _recoverySize -= (std::min) (_recoverySize, count);
    }
}

uint16_t RTCMFramer::_discardCandidate()
{
    uint16_t validatedFrameSize = 0;
    uint16_t searchOffset = _frameSize;
    if (!valid()) {
        _recoverySize = (std::max) (_recoverySize, _frameSize);
        searchOffset = 1;
    }
    uint16_t discardSize = _frameSize;
    if (searchOffset < _recoverySize) {
        discardSize = _recoverySize;
        // Fresh bytes may belong to an incomplete outer frame.
        const std::span<const uint8_t> bytes{_bytes.data(), _recoverySize};
        for (uint16_t offset = searchOffset; offset < _recoverySize; ++offset) {
            const auto suffix = bytes.subspan(offset);
            if (suffix[0] != PREAMBLE || _reservedBitsSet(suffix)) {
                continue;
            }
            if (suffix.size() < HEADER_SIZE) {
                discardSize = (std::min) (discardSize, offset);
                continue;
            }
            const size_t payloadSize = _headerPayloadSize(suffix);
            if (payloadSize < MIN_PAYLOAD_SIZE) {
                continue;
            }
            const size_t length = HEADER_SIZE + payloadSize + CRC_SIZE;
            if (length > suffix.size()) {
                discardSize = (std::min) (discardSize, offset);
                continue;
            }
            if (isValidFrame(suffix.first(length))) {
                discardSize = offset;
                validatedFrameSize = static_cast<uint16_t>(length);
                break;
            }
        }
    }
    _discardPrefix(discardSize);
    _frameSize = 0;
    _valid = false;
    return validatedFrameSize;
}

bool RTCMFramer::_inspect(uint16_t validatedFrameSize)
{
    _valid = false;
    _frameSize = 0;
    _payloadSize = 0;
    const auto preamble = std::find(_bytes.begin(), _bytes.begin() + _size, PREAMBLE);
    _discardPrefix(static_cast<uint16_t>(preamble - _bytes.begin()));
    const std::span<const uint8_t> buffered{_bytes.data(), _size};
    if (_reservedBitsSet(buffered)) {
        _frameSize = RESERVED_BITS_OFFSET + 1;
        return true;
    }
    if (_size < HEADER_SIZE) {
        return false;
    }
    _payloadSize = _headerPayloadSize(buffered);
    if (_payloadSize < MIN_PAYLOAD_SIZE) {
        _frameSize = HEADER_SIZE;
        return true;
    }
    const uint16_t expectedSize = HEADER_SIZE + _payloadSize + CRC_SIZE;
    if (_size < expectedSize) {
        return false;
    }
    _frameSize = expectedSize;
    _valid = validatedFrameSize == _frameSize || isValidFrame(frame());
    return true;
}

std::optional<RTCMDecodedFrame> RTCMFrameDecoder::addByte(uint8_t byte, qint64 receivedAtMs)
{
    _receiptTimes[_nextReceiptIndex] = receivedAtMs;
    _nextReceiptIndex = (_nextReceiptIndex + 1) % _receiptTimes.size();
    if (!_framer.addByte(byte)) {
        return std::nullopt;
    }
    return _result();
}

std::optional<RTCMDecodedFrame> RTCMFrameDecoder::nextFrame()
{
    if (!_framer.nextFrame()) {
        return std::nullopt;
    }
    return _result();
}

RTCMDecodedFrame RTCMFrameDecoder::_result() const
{
    const auto frame = _framer.frame();
    const size_t firstReceiptIndex =
        (_nextReceiptIndex + _receiptTimes.size() - _framer.bufferedSize()) % _receiptTimes.size();
    RTCMDecodedFrame result{QByteArrayView(frame).toByteArray(), _framer.messageId(), _receiptTimes[firstReceiptIndex]};
    result.valid = _framer.valid();
    return result;
}

void RTCMFrameDecoder::reset()
{
    _framer.reset();
    _nextReceiptIndex = 0;
}
