#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QSet>
#include <QtCore/QVector>

#include "Checksums.h"

/// Bounded RTCM framing shared by receiver codecs and correction inputs.
/// Drain nextFrame() after completion to recover buffered suffixes.
class RTCMFramer
{
public:
    static constexpr uint8_t PREAMBLE = 0xd3;
    static constexpr uint16_t HEADER_SIZE = 3;
    static constexpr uint16_t CRC_SIZE = 3;
    static constexpr uint16_t MAX_PAYLOAD_SIZE = 1023;
    static constexpr uint16_t MAX_FRAME_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE + CRC_SIZE;

    void reset()
    {
        _size = 0;
        _payloadSize = 0;
        _frameSize = 0;
        _recoverySize = 0;
        _valid = false;
    }

    bool addByte(uint8_t byte);
    bool nextFrame();

    bool hasPartialFrame() const { return _size != 0 && !_frameSize; }

    /// Complete candidate, including malformed headers; empty while incomplete.
    std::span<const uint8_t> frame() const { return {_bytes.data(), _frameSize}; }

    /// Retained suffix length, including bytes beyond the current candidate.
    uint16_t bufferedSize() const { return _size; }

    uint16_t payloadLength() const { return _payloadSize; }

    uint16_t messageId() const
    {
        const uint16_t available = _frameSize ? _frameSize : _size;
        return payloadLength() >= 2 ? frameMessageId(std::span<const uint8_t>(_bytes.data(), available)) : 0;
    }

    /// Message number from a frame header; zero when the header is incomplete.
    [[nodiscard]] static uint16_t frameMessageId(std::span<const uint8_t> bytes)
    {
        return bytes.size() >= HEADER_SIZE + 2 && bytes[0] == PREAMBLE ? (bytes[3] << 4) | (bytes[4] >> 4) : 0;
    }

    bool valid() const { return _valid; }

    [[nodiscard]] static bool isValidFrame(std::span<const uint8_t> bytes)
    {
        if (bytes.size() < HEADER_SIZE + 2 + CRC_SIZE || bytes[0] != PREAMBLE || (bytes[1] & 0xfc)) {
            return false;
        }
        const size_t payloadSize = ((bytes[1] & 3) << 8) | bytes[2];
        return bytes.size() == HEADER_SIZE + payloadSize + CRC_SIZE && QGC::crc24q(bytes) == 0;
    }

    [[nodiscard]] static bool isValidFrame(QByteArrayView bytes)
    {
        return isValidFrame(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(bytes.data()),
                                                     static_cast<size_t>(bytes.size())));
    }

private:
    void _discardPrefix(uint16_t count);
    uint16_t _discardCandidate();
    bool _inspect(uint16_t validatedFrameSize);

    std::array<uint8_t, MAX_FRAME_SIZE> _bytes{};
    uint16_t _size = 0;
    uint16_t _payloadSize = 0;
    uint16_t _frameSize = 0;
    uint16_t _recoverySize = 0;
    bool _valid = false;
};

struct RTCMDecodedFrame
{
    QByteArray data;
    int messageId = 0;
    qint64 receivedAtMs = 0;
    bool valid = false;
    bool filtered = false;
};

/// Frames RTCM bytes while retaining the first fragment's monotonic receipt time.
class RTCMFrameDecoder
{
public:
    RTCMFrameDecoder();
    ~RTCMFrameDecoder();

    std::optional<RTCMDecodedFrame> addByte(uint8_t byte, qint64 receivedAtMs);
    /// Drain after addByte() returns a result, before feeding more bytes.
    std::optional<RTCMDecodedFrame> nextFrame();
    void reset();

    /// Frames @a bytes, delivering each completed frame. A delivery that returns false, because it ended the
    /// stream or retired its owner, stops framing at once; feed() then returns false.
    template <typename Deliver>
    bool feed(QByteArrayView bytes, qint64 receivedAtMs, Deliver&& deliver)
    {
        for (const char byte : bytes) {
            for (auto frame = addByte(static_cast<uint8_t>(byte), receivedAtMs); frame; frame = nextFrame()) {
                if (!deliver(*frame)) {
                    return false;
                }
            }
        }
        return true;
    }

    void setWhitelist(const QVector<int>& ids) { _whitelist = QSet<int>(ids.begin(), ids.end()); }

private:
    RTCMDecodedFrame _result() const;

    RTCMFramer _framer;
    QSet<int> _whitelist;
    std::array<qint64, RTCMFramer::MAX_FRAME_SIZE> _receiptTimes{};
    size_t _nextReceiptIndex = 0;
};
