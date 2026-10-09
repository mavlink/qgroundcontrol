#include "GPSStreamDemux.h"

#include <vector>

#include "NMEAFramer.h"
#include "RTCMFramer.h"
#include "SBF/SBFBlocks.h"
#include "UBX/UBXFrame.h"

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

    Q_DISABLE_COPY(Framer)

    GPSFrameKind kind() const { return _kind; }

    virtual Claim claim(uint8_t byte) const = 0;
    virtual void feed(uint8_t byte, Handler& handler) = 0;
    virtual void reset() = 0;

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
        if (_framer.hasPartialFrame()) {
            return Claim::Locked;
        }
        return byte == RTCMFramer::PREAMBLE ? Claim::Owns : Claim::None;
    }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        // A completed candidate can leave further frames in its recovery suffix.
        for (bool complete = _framer.addByte(byte); complete; complete = _framer.nextFrame()) {
            if (!_framer.valid()) {
                continue;
            }
            const auto frame = _framer.frame();
            handler.frame({.kind = GPSFrameKind::RTCM3,
                           .bytes = frame,
                           .payload = frame.subspan(RTCMFramer::HEADER_SIZE,
                                                    frame.size() - RTCMFramer::HEADER_SIZE - RTCMFramer::CRC_SIZE),
                           .messageId = RTCMFramer::frameMessageId(frame)});
        }
    }

    void reset() override { _framer.reset(); }

private:
    RTCMFramer _framer;
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
        return byte == UBX::SYNC1 ? Claim::Owns : Claim::None;
    }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        if (const auto frame = _decoder.consume(byte)) {
            handler.frame({.kind = GPSFrameKind::UBX,
                           .bytes = frame->bytes,
                           .payload = frame->payload,
                           .messageId = frame->message});
        }
    }

    void reset() override { _decoder.reset(); }

private:
    UBX::FrameDecoder _decoder;
};

/// Septentrio block candidates: a candidate ends at its length field or at the 110-byte capture limit, so an oversized
/// block is delivered truncated for the decoder to reject. The rest of a block whose length is plausible, a multiple
/// of four up to MAX_SKIPPED_LENGTH, is then discarded, so its binary content never reaches another framer.
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
                return byte == SBF::SYNC1 ? (_shareSync ? Claim::Watch : Claim::Owns) : Claim::None;
            case State::Sync2:
                return _shareSync ? Claim::Watch : Claim::Locked;
            case State::Payload:
            case State::Skip:
                return Claim::Locked;
        }
        return Claim::None;
    }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        switch (_state) {
            case State::Sync1:
                if (byte == SBF::SYNC1) {
                    (void) _add(byte);
                    _state = State::Sync2;
                }
                break;
            case State::Sync2:
                if (byte == SBF::SYNC2) {
                    (void) _add(byte);
                    _state = State::Payload;
                } else {
                    reset();
                }
                break;
            case State::Payload:
                if (_add(byte)) {
                    // A complete candidate holds at least the header.
                    const std::span<const uint8_t> bytes(_wire.data(), _size);
                    const auto header = _header();
                    const uint16_t remaining =
                        header.length > _size && header.length % 4 == 0 && header.length <= MAX_SKIPPED_LENGTH
                            ? header.length - _size
                            : 0;
                    handler.frame(
                        {.kind = GPSFrameKind::SBF, .bytes = bytes, .payload = bytes, .messageId = header.number()});
                    reset();
                    if (remaining > 0) {
                        _state = State::Skip;
                        _remaining = remaining;
                    }
                }
                break;
            case State::Skip:
                if (--_remaining == 0) {
                    reset();
                }
                break;
        }
    }

    void reset() override
    {
        _state = State::Sync1;
        _size = 0;
        _remaining = 0;
    }

private:
    enum class State : uint8_t
    {
        Sync1,
        Sync2,
        Payload,
        /// Discarding the rest of a block beyond the capture limit.
        Skip,
    };

    /// The longest block whose rest is discarded; a longer length is more likely a false sync than a block.
    static constexpr uint16_t MAX_SKIPPED_LENGTH = 4096;

    /// Meaningful once the header is complete.
    SBF::BlockHeader _header() const { return Wire::decode<SBF::BlockHeader>(_wire); }

    /// @return true when the candidate is complete.
    bool _add(uint8_t byte)
    {
        _wire[_size++] = byte;
        return (_size >= Wire::SIZE<SBF::BlockHeader> && _size >= _header().length) || _size >= _wire.size();
    }

    std::array<uint8_t, 110> _wire{};
    uint16_t _size = 0;
    uint16_t _remaining = 0;
    State _state = State::Sync1;
    bool _shareSync = false;
};

class NMEASentenceFramer final : public GPSStreamDemux::Framer
{
public:
    NMEASentenceFramer()
        : Framer(GPSFrameKind::NMEASentence)
        , _buffer(BUFFER_SIZE)
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
    static constexpr size_t BUFFER_SIZE = 600;

    std::vector<uint8_t> _buffer;
    NMEA::Framer _framer;
};

class ASCIILineFramer final : public GPSStreamDemux::Framer
{
public:
    ASCIILineFramer()
        : Framer(GPSFrameKind::ASCIILine)
        , _buffer(BUFFER_SIZE)
        , _framer(_buffer)
    {}

    Claim claim(uint8_t) const override { return Claim::Default; }

    void feed(uint8_t byte, GPSStreamDemux::Handler& handler) override
    {
        if (const auto line = _framer.addByte(byte)) {
            const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t*>(line->data()), line->size());
            handler.frame({.kind = GPSFrameKind::ASCIILine, .bytes = bytes, .payload = bytes});
        }
    }

    void reset() override { _framer.reset(); }

private:
    static constexpr size_t BUFFER_SIZE = 4096;

    std::vector<char> _buffer;
    NMEA::LineFramer _framer;
};

}  // namespace

GPSStreamDemux::GPSStreamDemux(const GPSStreamConfig& config)
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
        _framers[3] = std::make_unique<NMEASentenceFramer>();
    }
    if (config.framers & GPSFrameKind::ASCIILine) {
        _framers[4] = std::make_unique<ASCIILineFramer>();
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
