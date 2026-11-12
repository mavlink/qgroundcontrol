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

namespace Passive {

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
    .type = GPSType::Passive,
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
/// runtimes have no transport, and configuration only sets the local link's rate. Without a rate to set, it finds the
/// receiver's by listening: the link runs at each of LISTEN_RATES for up to LISTEN_WINDOW, until a frame of one of
/// INPUTS or an RTCM3 frame arrives.
class PassiveProtocol final : public GPSFamilyProtocol
{
public:
    static constexpr std::chrono::milliseconds SOURCE_TIMEOUT{3000};
    /// The rates a receiver is listened for at, the likeliest first: 115200 (most current receivers, and a u-blox one
    /// that QGroundControl configured), 9600 (most NMEA receivers, older u-blox), 38400 (u-blox F9, NMEA 0183-HS) and
    /// 4800 (NMEA 0183), then the other rates in use.
    static constexpr std::array<unsigned, 9> LISTEN_RATES{115200, 9600,   38400,  4800,  57600,
                                                          19200,  230400, 460800, 921600};
    /// How long each rate is listened at: a receiver that reports every two seconds, as older handheld ones do,
    /// completes a sentence within it at 4800 baud, where the longest sentence takes 0.17 s. A receiver that reports
    /// less often may be missed. All rates together take 19.8 s.
    static constexpr std::chrono::milliseconds LISTEN_WINDOW{2200};

    PassiveProtocol()
    {
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

    bool configure(GPSCommandChannel& channel, GPSConfig, unsigned& baud) override
    {
        if (!baud) {
            baud = _listenForRate(channel);
            if (!baud) {
                return false;
            }
        } else if (!channel.setBaudRate(baud)) {
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
            _heard = true;
            context.publishRTCM(frame.bytes);
            return {};
        }
        const auto input = _inputOf(frame);
        if (!input) {
            return {};
        }
        _heard = true;
        if (!_followed) {
            _follow(*input);
        }
        (void) _inputs[*input].runtime->consumeFrame(frame);
        // A frame ends the receive cycle that listens for it, whether or not its decoder reports an update.
        return _listening ? GPSReceiveUpdates(GPSReceiveUpdate::Activity) : GPSReceiveUpdates{};
    }

    void flush(GPSDecodeContext& context) override
    {
        _context = &context;
        for (const auto& input : _inputs) {
            (void) input.runtime->consume({});
        }
    }

    bool receiverReady() const override { return _ready; }

    /// A passive receiver is never asked what it is. What listening learned of it is its rate.
    QString identity() const override
    {
        return _listenedRate ? QStringLiteral("Found at %1 baud").arg(_listenedRate) : QString();
    }

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

    /// Runs the link at each of LISTEN_RATES until the receiver's output arrives, without writing. @return the rate it
    /// arrived at; zero, with the failure recorded, when it arrived at none.
    unsigned _listenForRate(GPSCommandChannel& channel)
    {
        _listening = true;
        const GPSBaudDetection listened = channel.detectBaud(LISTEN_RATES, 0, [this, &channel] {
            // What the receiver sent at another rate is noise, which may have left a framer inside a frame.
            channel.stream().reset();
            _heard = false;
            return channel.receiveUntil([this] { return _heard; }, LISTEN_WINDOW) ? GPSBaudProbe::Found
                                                                                  : GPSBaudProbe::TryNext;
        });
        _listening = false;
        if (!listened.found) {
            // A link failure or a stop is already recorded, and stays.
            channel.failControl(
                listened.probed.empty()
                    ? QStringLiteral("The link cannot run at any rate a passive receiver is listened for at")
                    : QStringLiteral("No NMEA, UBX, SBF or RTCM output arrived at %1 baud. Select the receiver's baud "
                                     "rate if it reports less often than every two seconds")
                          .arg(gpsRateList(listened.probed)));
            return 0;
        }
        _listenedRate = listened.baud;
        qCDebug(PassiveProtocolLog) << "Passive input found at" << _listenedRate << "baud";
        return _listenedRate;
    }

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
    bool _listening = false;
    /// A frame arrived at the rate that is listened at.
    bool _heard = false;
    /// The rate listening found the receiver at; zero for a rate that was given.
    unsigned _listenedRate = 0;
};

}  // namespace

const GPSReceiverFamily FAMILY{
    .type = GPSType::Passive,
    .logCategory = &PassiveProtocolLog,
    .stream = {.framers = inputFrames(), .enabled = inputFrames(), .shareSBFSync = true},
    .create = &gpsCreateProtocol<PassiveProtocol>,
};

}  // namespace Passive
