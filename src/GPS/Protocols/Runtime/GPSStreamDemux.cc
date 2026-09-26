#include "GPSStreamDemux.h"

#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

#include "NMEAFramer.h"
#include "RTCMStreamDecoder.h"
#include "UBX/UBXFrameDecoder.h"

class GPSStreamDemux::Framer
{
public:
    enum class Claim : uint8_t
    {
        None,
        Default,
        Watch,
        Owns,
        Locked,
    };

    explicit Framer(GPSFrameKind kind)
        : _kind(kind)
    {}

    virtual ~Framer() = default;

    Framer(const Framer&) = delete;
    Framer& operator=(const Framer&) = delete;

    GPSFrameKind kind() const { return _kind; }

    virtual Claim claim(uint8_t byte) const = 0;
    virtual void feed(uint8_t byte, Handler& handler) = 0;
    virtual void reset() = 0;

    virtual void drain(Handler&) {}

    bool enabled = false;

private:
    GPSFrameKind _kind;
};

namespace {

using Claim = GPSStreamDemux::Framer::Claim;

class RTCM3Framer final : public GPSStreamDemux::Framer
{
public:
    RTCM3Framer()
        : Framer(GPSFrameKind::RTCM3)
    {}

    Claim claim(uint8_t byte) const override
    {
        // Any byte other than the preamble is owned only while a candidate or queued frame exists.
        if (_decoder.ownsByte(0x00)) {
            return Claim::Locked;
        }
        return byte == RTCMFramer::PREAMBLE ? Claim::Owns : Claim::None;
    }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        _decoder.addByte(byte);
        drain(handler);
    }

    void drain(GPSStreamDemux::Handler& handler) override
    {
        _decoder.drain([&handler](std::span<const uint8_t> frame) {
            if (!handler.acceptDeferred()) {
                return false;
            }
            handler.frame({.kind = GPSFrameKind::RTCM3,
                           .bytes = frame,
                           .payload = frame.subspan(RTCMFramer::HEADER_SIZE,
                                                    frame.size() - RTCMFramer::HEADER_SIZE - RTCMFramer::CRC_SIZE),
                           .messageId = RTCMFramer::frameMessageId(frame)});
            return true;
        });
    }

    void reset() override { _decoder.reset(); }

private:
    RTCMStreamDecoder _decoder;
};

class UBXFramer final : public GPSStreamDemux::Framer
{
public:
    UBXFramer()
        : Framer(GPSFrameKind::UBX)
    {}

    Claim claim(uint8_t byte) const override
    {
        if (!_decoder.idle()) {
            return Claim::Locked;
        }
        return byte == SYNC1 ? Claim::Owns : Claim::None;
    }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        const uint8_t checksumA = std::exchange(_previous, byte);
        const auto frame = _decoder.consume(byte);
        if (!frame) {
            return;
        }
        _bytes[0] = SYNC1;
        _bytes[1] = SYNC2;
        _bytes[2] = static_cast<uint8_t>(frame->message);
        _bytes[3] = static_cast<uint8_t>(frame->message >> 8);
        _bytes[4] = static_cast<uint8_t>(frame->length);
        _bytes[5] = static_cast<uint8_t>(frame->length >> 8);
        std::copy_n(frame->payload.begin(), frame->length, _bytes.begin() + HEADER_SIZE);
        _bytes[HEADER_SIZE + frame->length] = checksumA;
        _bytes[HEADER_SIZE + frame->length + 1] = byte;
        const std::span<const uint8_t> bytes(_bytes.data(), HEADER_SIZE + frame->length + 2);
        handler.frame({.kind = GPSFrameKind::UBX,
                       .bytes = bytes,
                       .payload = bytes.subspan(HEADER_SIZE, frame->length),
                       .messageId = frame->message});
    }

    void reset() override { _decoder.reset(); }

private:
    static constexpr uint8_t SYNC1 = 0xb5;
    static constexpr uint8_t SYNC2 = 0x62;
    static constexpr size_t HEADER_SIZE = 6;

    UBX::FrameDecoder _decoder;
    std::array<uint8_t, HEADER_SIZE + std::tuple_size_v<decltype(UBX::Frame::payload)> + 2> _bytes{};
    uint8_t _previous = 0;
};

/// Septentrio block candidates: a candidate ends at its length field or at the 110-byte capture limit, so an oversized
/// block is delivered truncated for the decoder to reject.
class SBFFramer final : public GPSStreamDemux::Framer
{
public:
    explicit SBFFramer(bool shareSync)
        : Framer(GPSFrameKind::SBF)
        , _shareSync(shareSync)
    {}

    Claim claim(uint8_t byte) const override
    {
        switch (_state) {
            case State::Sync1:
                return byte == SYNC1 ? (_shareSync ? Claim::Watch : Claim::Owns) : Claim::None;
            case State::Sync2:
                return _shareSync ? Claim::Watch : Claim::Locked;
            case State::Payload:
                return Claim::Locked;
        }
        return Claim::None;
    }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        switch (_state) {
            case State::Sync1:
                if (byte == SYNC1) {
                    (void) _add(byte);
                    _state = State::Sync2;
                }
                break;
            case State::Sync2:
                if (byte == SYNC2) {
                    (void) _add(byte);
                    _state = State::Payload;
                } else {
                    reset();
                }
                break;
            case State::Payload:
                if (_add(byte)) {
                    const std::span<const uint8_t> bytes(_wire.data(), _size);
                    const uint32_t id = _size >= 6 ? ((uint32_t(_wire[5]) << 8) | _wire[4]) & 0x1fff : 0;
                    handler.frame({.kind = GPSFrameKind::SBF, .bytes = bytes, .payload = bytes, .messageId = id});
                    reset();
                }
                break;
        }
    }

    void reset() override
    {
        _state = State::Sync1;
        _size = 0;
    }

private:
    enum class State : uint8_t
    {
        Sync1,
        Sync2,
        Payload,
    };

    static constexpr uint8_t SYNC1 = 0x24;
    static constexpr uint8_t SYNC2 = 0x40;

    /// @return true when the candidate is complete.
    bool _add(uint8_t byte)
    {
        _wire[_size++] = byte;
        const uint16_t length = static_cast<uint16_t>(_wire[6] | (_wire[7] << 8));
        return (_size > 7 && _size >= length) || _size >= _wire.size();
    }

    std::array<uint8_t, 110> _wire{};
    uint16_t _size = 0;
    State _state = State::Sync1;
    bool _shareSync = false;
};

class NMEASentenceFramer final : public GPSStreamDemux::Framer
{
public:
    explicit NMEASentenceFramer(uint16_t bufferSize)
        : Framer(GPSFrameKind::NMEASentence)
        , _buffer(bufferSize)
        , _framer(_buffer)
    {}

    Claim claim(uint8_t) const override { return Claim::Default; }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        if (const size_t length = _framer.addByte(byte); length > 0) {
            const std::span<const uint8_t> bytes(_buffer.data(), length);
            handler.frame({.kind = GPSFrameKind::NMEASentence, .bytes = bytes, .payload = bytes});
        }
    }

    void reset() override { _framer.reset(); }

private:
    std::vector<uint8_t> _buffer;
    NMEA::Framer _framer;
};

class ASCIILineFramer final : public GPSStreamDemux::Framer
{
public:
    ASCIILineFramer(uint16_t bufferSize, NMEA::LineFramer::Options options)
        : Framer(GPSFrameKind::ASCIILine)
        , _buffer(bufferSize)
        , _framer(_buffer, options)
    {}

    Claim claim(uint8_t) const override { return Claim::Default; }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        const auto update = _framer.addByte(byte);
        if (update.line) {
            const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t*>(update.line->data()),
                                                 update.line->size());
            handler.frame({.kind = GPSFrameKind::ASCIILine, .bytes = bytes, .payload = bytes});
        }
    }

    void reset() override { _framer.reset(); }

private:
    std::vector<char> _buffer;
    NMEA::LineFramer _framer;
};

}  // namespace

GPSStreamDemux::GPSStreamDemux(const GPSStreamConfig& config)
    : _available(config.framers)
{
    if (config.framers & GPSFrameKind::RTCM3) {
        _framers[0] = std::make_unique<RTCM3Framer>();
    }
    if (config.framers & GPSFrameKind::UBX) {
        _framers[1] = std::make_unique<UBXFramer>();
    }
    if (config.framers & GPSFrameKind::SBF) {
        _framers[2] = std::make_unique<SBFFramer>(config.shareSBFSync);
    }
    if (config.framers & GPSFrameKind::NMEASentence) {
        _framers[3] = std::make_unique<NMEASentenceFramer>(config.sentenceBufferSize);
    }
    if (config.framers & GPSFrameKind::ASCIILine) {
        _framers[4] = std::make_unique<ASCIILineFramer>(config.lineBufferSize, config.lineOptions);
    }
    for (const auto& framer : _framers) {
        if (framer) {
            framer->enabled = config.enabled.testFlag(framer->kind());
        }
    }
}

GPSStreamDemux::~GPSStreamDemux() = default;

void GPSStreamDemux::push(uint8_t byte, Handler& handler)
{
    std::array<Claim, FRAMER_COUNT> claims{};
    Framer* exclusive = nullptr;
    for (size_t index = 0; index < _framers.size(); ++index) {
        const auto& framer = _framers[index];
        if (!framer || !framer->enabled) {
            continue;
        }
        claims[index] = framer->claim(byte);
        if (claims[index] == Claim::Locked) {
            exclusive = framer.get();
            break;
        }
        if (claims[index] == Claim::Owns && !exclusive) {
            exclusive = framer.get();
        }
    }
    if (exclusive) {
        for (const auto& framer : _framers) {
            if (framer && framer->enabled && framer.get() != exclusive) {
                framer->reset();
            }
        }
        exclusive->feed(byte, handler);
        return;
    }
    bool defaultFed = false;
    for (size_t index = 0; index < _framers.size(); ++index) {
        if (claims[index] == Claim::Watch || (claims[index] == Claim::Default && !defaultFed)) {
            defaultFed |= claims[index] == Claim::Default;
            _framers[index]->feed(byte, handler);
        }
    }
}

void GPSStreamDemux::drainDeferred(Handler& handler)
{
    for (const auto& framer : _framers) {
        if (framer && framer->enabled) {
            framer->drain(handler);
        }
    }
}

void GPSStreamDemux::reset()
{
    for (const auto& framer : _framers) {
        if (framer) {
            framer->reset();
        }
    }
}

void GPSStreamDemux::reset(GPSFrameKind kind)
{
    for (const auto& framer : _framers) {
        if (framer && framer->kind() == kind) {
            framer->reset();
        }
    }
}

void GPSStreamDemux::setEnabled(GPSFrameKind kind, bool enabled)
{
    for (const auto& framer : _framers) {
        if (framer && framer->kind() == kind) {
            framer->reset();
            framer->enabled = enabled;
        }
    }
}

bool GPSStreamDemux::enabled(GPSFrameKind kind) const
{
    return std::ranges::any_of(
        _framers, [kind](const auto& framer) { return framer && framer->kind() == kind && framer->enabled; });
}
