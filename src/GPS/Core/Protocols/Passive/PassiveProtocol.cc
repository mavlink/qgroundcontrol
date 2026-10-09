#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

#include "GPSCommandChannel.h"
#include "GPSFamilyProtocol.h"
#include "GPSNMEAStream.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "MonotonicClock.h"
#include "NMEASentence.h"
#include "QGCLoggingCategory.h"
#include "SBF/SBFBlocks.h"

QGC_LOGGING_CATEGORY(PassiveProtocolLog, "GPS.Protocols.Passive")

namespace {

/// Standard NMEA, decoded as the ASCII families decode it.
class NMEAProtocol final : public GPSFamilyProtocol
{
public:
    bool configure(GPSCommandChannel&, GPSConfig, unsigned&) override { return false; }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        return _stream.onFrame(frame, context);
    }

    void flush(GPSDecodeContext& context) override { _stream.flush(context); }

    [[nodiscard]] const GPSNMEAStream& stream() const { return _stream; }

private:
    GPSNMEAStream _stream;
};

const GPSReceiverFamily NMEA_FAMILY{
    .type = GPSType::passive,
    .logCategory = &PassiveProtocolLog,
    .stream = {.framers = GPSFrameKind::ASCIILine, .enabled = GPSFrameKind::ASCIILine},
    .create = &gpsCreateProtocol<NMEAProtocol>,
};

bool standardSentence(const GPSFrame& frame)
{
    return NMEA::isStandardSentence(frame.text());
}

bool checkedBlock(const GPSFrame& frame)
{
    return SBF::checkedHeader(frame.bytes).has_value();
}

/// One protocol passive input recognises, decoded by a decode-only runtime of its family.
struct PassiveInput
{
    const GPSReceiverFamily* family;
    GPSFrameKinds frames;
    /// Whether a frame of these kinds belongs to the protocol; null accepts every one.
    bool (*accepts)(const GPSFrame& frame) = nullptr;
};

/// The protocols passive input recognises. Standard NMEA comes first: its stream paces receive().
constexpr std::array INPUTS{
    PassiveInput{&NMEA_FAMILY, GPSFrameKind::ASCIILine, &standardSentence},
    PassiveInput{&UBX::FAMILY, GPSFrameKind::UBX},
    PassiveInput{&SBF::FAMILY, GPSFrameKind::SBF, &checkedBlock},
};
static_assert(INPUTS.front().family == &NMEA_FAMILY);

/// RTCM3, which passive input forwards, and the frames of every input.
constexpr GPSFrameKinds inputFrames()
{
    GPSFrameKinds frames = GPSFrameKind::RTCM3;
    for (const auto& input : INPUTS) {
        frames |= input.frames;
    }
    return frames;
}

/// Decodes whichever protocol of INPUTS the receiver sends, each with a decode-only runtime of its own, and forwards
/// RTCM3. Positions, satellites and integrity come from one protocol only: the first one recognised, until another
/// delivers a position while it has delivered none for SOURCE_TIMEOUT. It never sends anything: the decode-only
/// runtimes have no transport, and configuration only sets the local link's rate.
class PassiveProtocol final : public GPSFamilyProtocol
{
public:
    static constexpr std::chrono::milliseconds SOURCE_TIMEOUT{3000};

    PassiveProtocol() { _reset(); }

    bool configure(GPSCommandChannel& channel, GPSConfig, unsigned& baud) override
    {
        _ready = false;
        _reset();
        channel.stream().reset();
        if (!channel.setBaudrate(baud)) {
            channel.failControl(QStringLiteral("The link cannot run at %1 baud").arg(baud));
            return false;
        }
        _ready = true;
        return true;
    }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        _context = &context;
        if (frame.kind == GPSFrameKind::RTCM3) {
            context.publishRTCM(frame.bytes);
            return {};
        }
        const auto input = _inputOf(frame);
        if (!input) {
            return {};
        }
        if (!_followed) {
            _follow(*input);
        }
        (void) _inputs[*input].runtime->consumeFrame(frame);
        return {};
    }

    void flush(GPSDecodeContext& context) override
    {
        _context = &context;
        for (const auto& input : _inputs) {
            (void) input.runtime->consume({});
        }
    }

    bool receiverReady() const override { return _ready; }

    GPSReceiveUpdates receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout) override
    {
        const auto& nmea = static_cast<const NMEAProtocol&>(_inputs.front().runtime->protocol()).stream();
        return nmea.receive(channel, timeout);
    }

private:
    struct Input
    {
        std::unique_ptr<GPSProtocolRuntime> runtime;
        uint64_t lastPositionUs = 0;
    };

    static std::optional<size_t> _inputOf(const GPSFrame& frame)
    {
        for (size_t index = 0; index < INPUTS.size(); ++index) {
            const auto& input = INPUTS[index];
            if (input.frames.testFlag(frame.kind) && (!input.accepts || input.accepts(frame))) {
                return index;
            }
        }
        return std::nullopt;
    }

    void _reset()
    {
        _followed.reset();
        for (size_t index = 0; index < INPUTS.size(); ++index) {
            GPSRuntimeIO io;
            io.clock.nowUs = [this] { return _context ? _context->nowUs() : 0; };
            GPSRuntimeObserver observer;
            observer.decoded = [this, index](const GPSEventBatch& batch) { _received(index, batch); };
            _inputs[index] = {.runtime = std::make_unique<GPSProtocolRuntime>(*INPUTS[index].family, std::move(io),
                                                                              std::move(observer)),
                              .lastPositionUs = 0};
            _inputs[index].runtime->armNavigationDecode();
        }
    }

    void _received(size_t index, const GPSEventBatch& batch)
    {
        if (batch.updates.testFlag(GPSReceiveUpdate::Position)) {
            const uint64_t now = _context->nowUs();
            _inputs[index].lastPositionUs = now;
            if (!_followed || (*_followed != index &&
                               !MonotonicClock::fresh(_inputs[*_followed].lastPositionUs, now, SOURCE_TIMEOUT))) {
                _follow(index);
            }
        }
        if (_followed != index) {
            return;
        }
        for (const auto& event : batch.events) {
            _context->publishAsReceived(event);
        }
        _context->markUpdates(batch.updates);
    }

    void _follow(size_t index)
    {
        _followed = index;
        // The NMEA decoder's family is passive, which reports standard NMEA.
        const GPSType family = INPUTS[index].family->type;
        qCDebug(PassiveProtocolLog) << "Passive input decodes" << family;
        _context->publishAsReceived(GPSInputProtocol{family});
    }

    std::array<Input, INPUTS.size()> _inputs;
    std::optional<size_t> _followed;
    GPSDecodeContext* _context = nullptr;
    bool _ready = false;
};

}  // namespace

namespace Passive {

const GPSReceiverFamily FAMILY{
    .type = GPSType::passive,
    .logCategory = &PassiveProtocolLog,
    .stream = {.framers = inputFrames(), .enabled = inputFrames(), .shareSBFSync = true},
    .create = &gpsCreateProtocol<PassiveProtocol>,
};

}  // namespace Passive
