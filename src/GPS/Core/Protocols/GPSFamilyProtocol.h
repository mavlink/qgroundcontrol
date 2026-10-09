#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QLatin1StringView>
#include <QtCore/QString>

#include "GPSCommand.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverConfig.h"
#include "GPSStreamDemux.h"
#include "GPSType.h"

class GPSCommandChannel;
class GPSFamilyProtocol;
class GPSProtocolRuntime;
class QLoggingCategory;

/// A family's logging category function, as declared by Q_DECLARE_LOGGING_CATEGORY; usable as qCWarning(category).
using GPSLogCategory = const QLoggingCategory& (*)();

/// Recognises one frame, framed with every framer enabled, as traffic only this family's receivers send.
/// @return what identified it, such as "UBX frames", or an empty view.
using GPSSignatureFunction = QLatin1StringView (*)(const GPSFrame& frame);

/// The receiver configuration requested for one configure() attempt.
struct GPSConfig
{
    GPSBaseStationConfig base{};
    bool allowPersistentChanges = false;
    /// A rate receiver detection just found the receiver at. When configure() gets no rate, a baud search
    /// (GPSCommandChannel::detectBaud()) tries this rate first, even one the family does not list, and a family
    /// without one runs the link at it (linkBaud()).
    unsigned detectedBaud = 0;

    /// The rate a family without a baud search runs the link at: @a requested, else detectedBaud, else @a fallback.
    [[nodiscard]] unsigned linkBaud(unsigned requested, unsigned fallback) const
    {
        return requested ? requested : detectedBaud ? detectedBaud : fallback;
    }
};

/// Static description of one receiver family. Each family defines one, its FAMILY, which the family table
/// (GPSReceiverFamilies.cc) lists for type lookup. Its name and base-station capabilities come from the family's
/// GPSReceiverDescriptor.
struct GPSReceiverFamily
{
    GPSType type = GPSType::passive;
    GPSLogCategory logCategory = nullptr;
    GPSStreamConfig stream{};
    /// Baud rates receiver detection probes the family at (GPSFamilyProtocol::probe()), in probing order; empty for a
    /// family it never detects, which then needs an explicit rate.
    std::span<const unsigned> baudCandidates{};
    /// Rate passed to configure() when none is selected; zero requests detection.
    unsigned autoBaudRate = 0;
    std::unique_ptr<GPSFamilyProtocol> (*create)() = nullptr;
    /// Receiver detection (GPSReceiverDetector) recognises the family's traffic with this, and its mismatch hints name
    /// the family; null when its traffic has no signature.
    GPSSignatureFunction signature = nullptr;
};

/// What a decoder may touch while it handles a frame or flushes: it publishes the decoded events, resolves the pending
/// command reply, adjusts the stream framers and reads the sticky failure. It never performs I/O.
class GPSDecodeContext
{
public:
    Q_DISABLE_COPY(GPSDecodeContext)

    [[nodiscard]] uint64_t nowUs() const;

    [[nodiscard]] GPSStreamDemux& stream();

    // Events of the current decode, in publication order. Events are never coalesced: every position epoch is its own
    // event, and consumers decide whether to drop superseded ones.

    void publishPosition(const GPSDecodedPosition& report)
    {
        _batch.updates |= GPSReceiveUpdate::Position;
        _batch.events.emplace_back(report);
    }

    void publishSatellites(const GPSDecodedSatellites& report)
    {
        _batch.updates |= GPSReceiveUpdate::Satellites;
        _batch.events.emplace_back(report);
    }

    void publishSatelliteUsage(std::optional<int> count)
    {
        _batch.updates |= GPSReceiveUpdate::Satellites;
        _batch.events.emplace_back(GPSDecodedSatelliteUsage{nowUs(), count});
    }

    /// Stamps the family's working @a report with the receipt time and publishes a copy.
    void publishIntegrity(GPSIntegrityReport& report)
    {
        report.timestampUs = nowUs();
        _batch.events.emplace_back(report);
        _batch.updates |= GPSReceiveUpdate::Activity;
    }

    /// Stamps @a status with the receipt time and publishes a copy. Survey progress is not an update flag.
    void publishSurvey(GPSSurveyReport& status)
    {
        status.timestampUs = nowUs();
        _batch.events.emplace_back(status);
    }

    /// Publishes survey-in progress; unknown coordinates remain NaN.
    void publishSurvey(bool active, bool valid, std::chrono::seconds duration,
                       const GPSEllipsoidPosition& position = {})
    {
        GPSSurveyReport status{};
        status.position = position;
        status.duration = duration;
        status.valid = valid;
        status.active = active;
        publishSurvey(status);
    }

    void publishRTCM(std::span<const uint8_t> frame)
    {
        _batch.events.emplace_back(GPSRTCMFrame{QByteArrayView(frame).toByteArray()});
        _batch.updates |= GPSReceiveUpdate::Activity;
    }

    /// Publishes @a event exactly as given: its timestamps are kept and no update flag is set. For reports a family
    /// buffered with their original receipt, such as a Quectel survey status held during boot verification, or decoded
    /// elsewhere, as by the protocol decoders of a passive input.
    void publishAsReceived(GPSProtocolEvent event) { _batch.events.push_back(std::move(event)); }

    /// Adds updates a decoder reports for a frame without publishing an event, such as protocol activity.
    void markUpdates(GPSReceiveUpdates updates) { _batch.updates |= updates; }

    /// True while a transact() reply is outstanding and unresolved.
    [[nodiscard]] bool replyPending() const;

    /// Resolves the outstanding reply; Pending and later resolutions are ignored.
    void resolveReply(GPSCommandOutcome outcome);

    /// Passes one decoded text reply to the outstanding text matcher, if any.
    void offerReply(std::string_view reply);

    [[nodiscard]] bool failed() const;

    /// Records a protocol failure described by @a detail unless a failure is already recorded, whose detail stays.
    void failControl(const QString& detail);

private:
    friend class GPSProtocolRuntime;

    explicit GPSDecodeContext(GPSCommandChannel& channel)
        : _channel(channel)
    {}

    [[nodiscard]] GPSEventBatch _takeBatch() { return std::exchange(_batch, {}); }

    /// Returns delivered storage so steady-state decoding does not allocate.
    void _recycle(std::vector<GPSProtocolEvent>&& storage)
    {
        storage.clear();
        if (_batch.events.empty() && _batch.events.capacity() < storage.capacity()) {
            _batch.events.swap(storage);
        }
    }

    GPSCommandChannel& _channel;
    GPSEventBatch _batch;
};

/// What a decode-only runtime decodes from a receiver configured elsewhere (GPSFamilyProtocol::armNavigationDecode()).
struct GPSNavigationDecode
{
    /// RTCM3 frames are published too.
    bool corrections = false;
};

/// One receiver family's protocol: its decoding, configuration and streaming services. The runtime owns the I/O,
/// framing, deadlines, command evidence and sticky failure, so implementations hold only receiver state.
class GPSFamilyProtocol
{
public:
    virtual ~GPSFamilyProtocol() = default;

    Q_DISABLE_COPY(GPSFamilyProtocol)

    /// Configures the receiver. A zero @a baud requests detection; set it to the rate the link ends at. Any sticky
    /// failure has been cleared before this starts. A failed configuration records its reason with
    /// GPSCommandChannel::failControl(), failSequence() or failNoAnswer() and returns false; GPSDriver logs it, so the
    /// family does not. @return true when the receiver is ready.
    [[nodiscard]] virtual bool configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) = 0;

    /// Receiver detection's read-only identity query at the link's current rate (GPSReceiverDetector): sends it and
    /// waits for the answer, never changing receiver settings. @return whether a receiver of this family answered, even
    /// one the family cannot configure; false, as by default, for a family without GPSReceiverFamily::baudCandidates,
    /// which detection never probes.
    [[nodiscard]] virtual bool probe(GPSCommandChannel& channel)
    {
        Q_UNUSED(channel)
        return false;
    }

    /// Decodes one frame, publishing events to the context. @return updates beyond the published events, such as
    /// protocol activity for an acknowledgement.
    virtual GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) = 0;

    /// Publishes time-driven output before each decode, such as expired epochs.
    virtual void flush(GPSDecodeContext& context) { Q_UNUSED(context) }

    /// Whether the receiver runs as the latest configure() set it up, such as a base sending corrections; false before
    /// and while configuring. The runtime reports a receiver with a sticky failure as not ready.
    [[nodiscard]] virtual bool receiverReady() const { return true; }

    /// Model and firmware reported during configuration; empty when unknown.
    [[nodiscard]] virtual QString identity() const { return {}; }

    /// Decode-only use, such as replaying a recording: leaves the decoder as a successful configure() with @a config
    /// would, without I/O or events. @return false, as by default, when the family reports base status only after
    /// configuring a receiver, or cannot configure @a config.
    [[nodiscard]] virtual bool armDecodeOnly(const GPSConfig& config, GPSDecodeContext& context)
    {
        Q_UNUSED(config)
        Q_UNUSED(context)
        return false;
    }

    /// Decode-only use without a configuration, such as passive input or fuzzing: decodes the navigation output
    /// (positions, satellites, integrity) of a receiver configured elsewhere, as @a decode asks, without I/O or events.
    /// @return false, as by default, when the family decodes that output without being armed.
    [[nodiscard]] virtual bool armNavigationDecode(GPSNavigationDecode decode, GPSDecodeContext& context)
    {
        Q_UNUSED(decode)
        Q_UNUSED(context)
        return false;
    }

    /// One streaming receive. The default runs GPSCommandChannel::receiveCycle() and then the streaming services.
    virtual GPSReceiveUpdates receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout);

    /// Whether the current receive cycle has what it waits for, given the updates it accumulated so far. Checked
    /// before every read, including the first. The default returns after any update.
    virtual bool completeReceiveCycle(GPSReceiveUpdates handled) { return handled != GPSReceiveUpdates{}; }

    /// The longest single read in a receive cycle of @a timeout; the cycle deadline still bounds it.
    [[nodiscard]] virtual std::chrono::milliseconds nextReadSlice(std::chrono::milliseconds timeout) const
    {
        return timeout;
    }

    /// Commands decoders scheduled while streaming, such as diagnostics polls, RTCM activation or disabling an
    /// unexpected message. Runs after each streaming receive under GPSCommandChannel::SERVICE_TIMEOUT.
    virtual void serviceStreaming(GPSCommandChannel& channel);

protected:
    GPSFamilyProtocol() = default;
};

/// A receiver identity as GPSFamilyProtocol::identity() reports it: "<model> <firmware>", or whichever is known.
[[nodiscard]] inline QString gpsReceiverIdentity(const QByteArray& model, const QByteArray& firmware)
{
    return QString::fromUtf8(model.isEmpty() || firmware.isEmpty() ? model + firmware : model + ' ' + firmware);
}

/// The `create` of a family whose protocol is default-constructed: `.create = &gpsCreateProtocol<Protocol>`.
template <typename Protocol>
std::unique_ptr<GPSFamilyProtocol> gpsCreateProtocol()
{
    return std::make_unique<Protocol>();
}
