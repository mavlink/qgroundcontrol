#pragma once

#include <chrono>
#include <string_view>

#include <QtCore/QString>

#include "GPSCommandTransaction.h"
#include "GPSFrame.h"
#include "GPSProtocolError.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverFamily.h"
#include "GPSTask.h"

class GPSCommandChannel;
class GPSEventSink;
class GPSProtocolRuntime;
class GPSStreamDemux;

/// What a decoder may touch while it handles a frame or flushes: the event sink, the pending command reply, the
/// stream framers and the sticky failure. It never performs I/O.
class GPSDecodeContext
{
public:
    GPSDecodeContext(const GPSDecodeContext&) = delete;
    GPSDecodeContext& operator=(const GPSDecodeContext&) = delete;

    [[nodiscard]] uint64_t nowUs() const;

    [[nodiscard]] GPSEventSink& sink();

    [[nodiscard]] GPSStreamDemux& stream();

    /// Delivers frames the stream queued while the sink was full, such as RTCM3. Families call this in flush() where
    /// their queued frames belong in the event order; the runtime drains again after flush() as a safety net.
    void drainDeferredFrames();

    /// True while a transact() reply is outstanding and unresolved.
    [[nodiscard]] bool replyPending() const;

    /// Resolves the outstanding reply; Pending and later resolutions are ignored.
    void resolveReply(GPSCommandOutcome outcome);

    /// Passes one decoded text reply to the outstanding text matcher, if any.
    void offerReply(std::string_view reply);

    [[nodiscard]] bool failed() const;

    [[nodiscard]] GPSProtocolError error() const;

    [[nodiscard]] const QString& errorDetail() const;

    /// Records a protocol failure unless a failure is already recorded; clears the detail it replaces.
    void failControl();

    void setErrorDetail(const QString& detail);

private:
    friend class GPSProtocolRuntime;

    explicit GPSDecodeContext(GPSProtocolRuntime& runtime)
        : _runtime(runtime)
    {}

    GPSProtocolRuntime& _runtime;
};

/// One receiver family's protocol: its decoder, configurator and streaming services. The runtime owns the I/O,
/// framing, deadlines, command evidence and sticky failure, so implementations hold only receiver state.
class GPSFamilyProtocol
{
public:
    virtual ~GPSFamilyProtocol() = default;

    GPSFamilyProtocol(const GPSFamilyProtocol&) = delete;
    GPSFamilyProtocol& operator=(const GPSFamilyProtocol&) = delete;

    /// Configures the receiver. A zero @a baud requests detection; set it to the rate the link ends at. Any sticky
    /// failure has been cleared before this starts. @return true when the receiver is ready.
    [[nodiscard]] virtual GPSTask<bool> configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud) = 0;

    /// Decodes one frame, publishing events to the context's sink. @return updates beyond the published events, such
    /// as protocol activity for an acknowledgement.
    virtual GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) = 0;

    /// Publishes time-driven output before each decoded chunk, such as expired epochs.
    virtual void flush(GPSDecodeContext& context) { context.drainDeferredFrames(); }

    /// Whether injection (RTCM and moving-baseline data) may be written; false while configuring. @a context reports
    /// the sticky failure; families decide whether a failure ends readiness, as Unicore and Quectel do.
    [[nodiscard]] virtual bool receiverReady(const GPSDecodeContext& context) const
    {
        Q_UNUSED(context)
        return true;
    }

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

    /// One streaming receive. The default runs GPSCommandChannel::receiveCycle() and then the streaming services.
    virtual GPSTask<GPSReceiveUpdates> receive(GPSCommandChannel& channel, std::chrono::milliseconds timeout);

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
    virtual GPSTask<void> serviceStreaming(GPSCommandChannel& channel);

protected:
    GPSFamilyProtocol() = default;
};
