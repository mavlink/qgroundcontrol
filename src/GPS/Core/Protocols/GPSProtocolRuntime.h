#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

#include <QtCore/QString>

#include "GPSClock.h"
#include "GPSCommand.h"
#include "GPSCommandChannel.h"
#include "GPSDeadline.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolEvent.h"
#include "GPSStreamDemux.h"
#include "GPSTransport.h"

/// Blocking transport services. Protocol code never calls these directly: it calls GPSCommandChannel operations,
/// which apply deadlines, evidence and sticky failure around each call. A missing read fails, a missing write or
/// baud change is unsupported.
struct GPSRuntimeIO
{
    std::function<GPSReadResult(std::span<uint8_t>, GPSDeadline)> read{};
    std::function<GPSWriteResult(std::span<const uint8_t>, GPSDeadline)> write{};
    std::function<GPSBaudStatus(unsigned)> setBaudrate{};
    /// Without a wait, the runtime sleeps in short slices until the duration elapses or @a cancelToken is cancelled.
    GPSClock clock{};
    /// Checked before each write and baud change; reads observe it in the transport.
    GPSCancelToken cancelToken{};
};

/// Observers of runtime output, invoked synchronously on the runtime thread.
struct GPSRuntimeObserver
{
    /// Borrowed for the callback; it may decode nested input through GPSProtocolRuntime::consume().
    std::function<void(const GPSEventBatch&)> decoded{};
    /// Every completed command, during configuration and streaming alike.
    std::function<void(const GPSConfigurationEvidence&)> commandFinished{};
};

/// Hosts one receiver family on a blocking transport. It owns the family protocol, the stream demultiplexer, the
/// decode context that collects events, and the command channel with its deadlines, evidence and sticky failure.
/// configure(), probe() and receive() run the family's code to completion on the caller thread; its channel operations
/// block on GPSRuntimeIO.
class GPSProtocolRuntime final : private GPSStreamDemux::Handler
{
public:
    /// Creates the family's protocol with its factory.
    GPSProtocolRuntime(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer = {});
    /// Hosts @a protocol, for families configured by the caller such as test doubles.
    GPSProtocolRuntime(const GPSReceiverFamily& family, std::unique_ptr<GPSFamilyProtocol> protocol, GPSRuntimeIO io,
                       GPSRuntimeObserver observer = {});
    ~GPSProtocolRuntime();

    Q_DISABLE_COPY(GPSProtocolRuntime)

    /// Configures the receiver after clearing any sticky failure. A zero @a baud requests detection and returns the
    /// detected rate. A command still pending at the end is retired as Written, and a successful configuration flushes
    /// once more so a fixed-base or survey-start event is delivered without another read.
    /// @return true when the receiver is ready; otherwise error() reports the failure, Protocol unless the link,
    /// a stop or a needed consent ended it, and errorDetail() describes it.
    [[nodiscard]] bool configure(const GPSConfig& config, unsigned& baud);

    /// Runs the family's identity probe (GPSFamilyProtocol::probe()) within @a budget at the link's current rate, after
    /// clearing any sticky failure. @return whether a receiver of this family answered; false for a family receiver
    /// detection never probes.
    [[nodiscard]] bool probe(std::chrono::milliseconds budget);

    /// One streaming receive of up to @a timeout, including the family's streaming services. Failures are reported
    /// through error(). @return the updates decoded.
    GPSReceiveUpdates receive(std::chrono::milliseconds timeout);

    /// Prepares decode() and consume() for traffic from a receiver configured with @a config, without I/O
    /// (GPSFamilyProtocol::armDecodeOnly()). @a config is valid for the family, as GPSDriver::configure() requires.
    /// @return false when the family needs a live configuration to report base status for its base mode.
    [[nodiscard]] bool armDecodeOnly(const GPSConfig& config);

    /// Prepares decode() and consume() for the navigation output of a receiver configured elsewhere, without I/O
    /// (GPSFamilyProtocol::armNavigationDecode()). @return false when the family decodes it without being armed.
    bool armNavigationDecode(GPSNavigationDecode decode = {});

    /// Decodes @a bytes without I/O or observer callbacks. @return their events; an empty span flushes.
    [[nodiscard]] GPSEventBatch decode(std::span<const uint8_t> bytes);

    /// Decodes @a bytes and delivers their batch to the observer. An empty span flushes.
    GPSReceiveUpdates consume(std::span<const uint8_t> bytes);

    /// Decodes one frame another stream framed and delivers its batch to the observer, without flushing.
    GPSReceiveUpdates consumeFrame(const GPSFrame& frame);

    [[nodiscard]] bool receiverReady() const;

    /// Model and firmware reported during configuration, trimmed; empty when unknown.
    [[nodiscard]] QString identity() const;

    [[nodiscard]] GPSProtocolError error() const { return _channel.error(); }

    [[nodiscard]] const QString& errorDetail() const { return _channel.errorDetail(); }

    [[nodiscard]] const GPSReceiverFamily& family() const { return _family; }

    [[nodiscard]] GPSFamilyProtocol& protocol() { return *_protocol; }

    [[nodiscard]] GPSStreamDemux& stream() { return _demux; }

    /// The family's logging category, or the runtime's when the family has none.
    [[nodiscard]] GPSLogCategory logCategory() const;

private:
    friend class GPSCommandChannel;

    void frame(const GPSFrame& frame) override;

    [[nodiscard]] uint64_t nowUs() const { return _io.clock.nowUs(); }

    GPSReceiveUpdates _deliver(GPSEventBatch batch);

    void _commandFinished(const GPSConfigurationEvidence& result);

    const GPSReceiverFamily& _family;
    GPSRuntimeIO _io;
    GPSRuntimeObserver _observer;
    GPSStreamDemux _demux;
    std::unique_ptr<GPSFamilyProtocol> _protocol;
    GPSCommandChannel _channel;
    GPSDecodeContext _context;
};
